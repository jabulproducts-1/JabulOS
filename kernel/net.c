#include "jabulos.h"

#define NET_ETHERTYPE_IPV4 0x0800u
#define NET_ETHERTYPE_ARP 0x0806u

#define NET_ARP_HARDWARE_ETHERNET 0x0001u
#define NET_ARP_OPERATION_REQUEST 0x0001u
#define NET_ARP_OPERATION_REPLY 0x0002u

#define NET_IPV4_HEADER_MIN_LENGTH 20u
#define NET_IPV4_VERSION_AND_IHL 0x45u
#define NET_IPV4_TTL_DEFAULT 64u
#define NET_IPV4_FLAG_DONT_FRAGMENT 0x4000u

#define NET_ARP_CACHE_SIZE 8u
#define NET_IPV4_QUEUE_SIZE 4u
#define NET_ETHERNET_FRAME_MAX 1514u
#define NET_RX_BUDGET 16u
#define NET_ARP_WAIT_SPINS 32768u

typedef struct __attribute__((packed)) {
    u8 destination[NET_MAC_ADDRESS_LENGTH];
    u8 source[NET_MAC_ADDRESS_LENGTH];
    u16 ether_type;
} ethernet_header_t;

typedef struct __attribute__((packed)) {
    u16 hardware_type;
    u16 protocol_type;
    u8 hardware_length;
    u8 protocol_length;
    u16 operation;
    u8 sender_hardware_address[NET_MAC_ADDRESS_LENGTH];
    u8 sender_protocol_address[NET_IPV4_ADDRESS_LENGTH];
    u8 target_hardware_address[NET_MAC_ADDRESS_LENGTH];
    u8 target_protocol_address[NET_IPV4_ADDRESS_LENGTH];
} arp_packet_t;

typedef struct __attribute__((packed)) {
    u8 version_and_ihl;
    u8 differentiated_services;
    u16 total_length;
    u16 identification;
    u16 flags_and_fragment_offset;
    u8 ttl;
    u8 protocol;
    u16 header_checksum;
    u8 source_address[NET_IPV4_ADDRESS_LENGTH];
    u8 destination_address[NET_IPV4_ADDRESS_LENGTH];
} ipv4_header_t;

typedef struct {
    bool valid;
    u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH];
    u8 mac_address[NET_MAC_ADDRESS_LENGTH];
    u32 age_stamp;
} arp_cache_entry_t;

typedef struct {
    bool initialized;
    bool ipv4_configured;
    u8 mac_address[NET_MAC_ADDRESS_LENGTH];
    u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH];
    u8 subnet_mask[NET_IPV4_ADDRESS_LENGTH];
    u8 gateway[NET_IPV4_ADDRESS_LENGTH];
    arp_cache_entry_t arp_cache[NET_ARP_CACHE_SIZE];
    net_ipv4_packet_t ipv4_queue[NET_IPV4_QUEUE_SIZE];
    u32 ipv4_queue_head;
    u32 ipv4_queue_tail;
    u32 ipv4_queue_count;
    u16 next_ipv4_id;
    u32 arp_age_counter;
    net_status_t status;
} net_state_t;

static net_state_t g_net;

static const u8 k_broadcast_mac[NET_MAC_ADDRESS_LENGTH] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
static const u8 k_zero_mac[NET_MAC_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u, 0u, 0u};

static u16 net_to_be16(u16 value) {
    return (u16)((value << 8) | (value >> 8));
}

static u16 net_from_be16(u16 value) {
    return net_to_be16(value);
}

static bool net_mac_equal(const u8 left[NET_MAC_ADDRESS_LENGTH], const u8 right[NET_MAC_ADDRESS_LENGTH]) {
    return memcmp(left, right, NET_MAC_ADDRESS_LENGTH) == 0;
}

static bool net_ipv4_equal(const u8 left[NET_IPV4_ADDRESS_LENGTH], const u8 right[NET_IPV4_ADDRESS_LENGTH]) {
    return memcmp(left, right, NET_IPV4_ADDRESS_LENGTH) == 0;
}

static bool net_ipv4_is_zero(const u8 address[NET_IPV4_ADDRESS_LENGTH]) {
    static const u8 k_zero_ipv4[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    return net_ipv4_equal(address, k_zero_ipv4);
}

static bool net_ipv4_is_broadcast(const u8 address[NET_IPV4_ADDRESS_LENGTH]) {
    static const u8 k_broadcast_ipv4[NET_IPV4_ADDRESS_LENGTH] = {255u, 255u, 255u, 255u};
    return net_ipv4_equal(address, k_broadcast_ipv4);
}

static bool net_ipv4_same_subnet(const u8 left[NET_IPV4_ADDRESS_LENGTH],
                                 const u8 right[NET_IPV4_ADDRESS_LENGTH],
                                 const u8 mask[NET_IPV4_ADDRESS_LENGTH]) {
    u32 index;

    for (index = 0u; index < NET_IPV4_ADDRESS_LENGTH; ++index) {
        if ((left[index] & mask[index]) != (right[index] & mask[index])) {
            return false;
        }
    }

    return true;
}

static void net_status_sync(void) {
    u32 arp_entries = 0u;
    u32 index;

    g_net.status.initialized = g_net.initialized;
    g_net.status.ipv4_configured = g_net.ipv4_configured;
    memcpy(g_net.status.mac, g_net.mac_address, sizeof(g_net.status.mac));
    memcpy(g_net.status.ipv4_address, g_net.ipv4_address, sizeof(g_net.status.ipv4_address));
    memcpy(g_net.status.subnet_mask, g_net.subnet_mask, sizeof(g_net.status.subnet_mask));
    memcpy(g_net.status.gateway, g_net.gateway, sizeof(g_net.status.gateway));
    g_net.status.queued_ipv4_packets = g_net.ipv4_queue_count;

    for (index = 0u; index < NET_ARP_CACHE_SIZE; ++index) {
        if (g_net.arp_cache[index].valid) {
            ++arp_entries;
        }
    }
    g_net.status.arp_cache_entries = arp_entries;
}

static void net_store_arp_entry(const u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH],
                                const u8 mac_address[NET_MAC_ADDRESS_LENGTH]) {
    s32 free_index = -1;
    u32 oldest_index = 0u;
    u32 oldest_age = 0xFFFFFFFFu;
    u32 index;

    if (net_ipv4_is_zero(ipv4_address) || net_mac_equal(mac_address, k_zero_mac)) {
        return;
    }

    for (index = 0u; index < NET_ARP_CACHE_SIZE; ++index) {
        if (g_net.arp_cache[index].valid) {
            if (net_ipv4_equal(g_net.arp_cache[index].ipv4_address, ipv4_address)) {
                memcpy(g_net.arp_cache[index].mac_address, mac_address, NET_MAC_ADDRESS_LENGTH);
                g_net.arp_cache[index].age_stamp = ++g_net.arp_age_counter;
                net_status_sync();
                return;
            }
            if (g_net.arp_cache[index].age_stamp < oldest_age) {
                oldest_age = g_net.arp_cache[index].age_stamp;
                oldest_index = index;
            }
        } else if (free_index < 0) {
            free_index = (s32)index;
        }
    }

    index = free_index >= 0 ? (u32)free_index : oldest_index;
    g_net.arp_cache[index].valid = true;
    memcpy(g_net.arp_cache[index].ipv4_address, ipv4_address, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_net.arp_cache[index].mac_address, mac_address, NET_MAC_ADDRESS_LENGTH);
    g_net.arp_cache[index].age_stamp = ++g_net.arp_age_counter;
    net_status_sync();
}

static bool net_lookup_arp_entry(const u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH],
                                 u8 out_mac[NET_MAC_ADDRESS_LENGTH]) {
    u32 index;

    for (index = 0u; index < NET_ARP_CACHE_SIZE; ++index) {
        if (g_net.arp_cache[index].valid &&
            net_ipv4_equal(g_net.arp_cache[index].ipv4_address, ipv4_address)) {
            memcpy(out_mac, g_net.arp_cache[index].mac_address, NET_MAC_ADDRESS_LENGTH);
            g_net.arp_cache[index].age_stamp = ++g_net.arp_age_counter;
            net_status_sync();
            return true;
        }
    }

    return false;
}

static u16 net_ipv4_checksum(const void* data, u16 length) {
    const u8* bytes = (const u8*)data;
    u32 sum = 0u;
    u16 index;

    for (index = 0u; index + 1u < length; index = (u16)(index + 2u)) {
        sum += ((u32)bytes[index] << 8) | (u32)bytes[index + 1u];
    }
    if ((length & 1u) != 0u) {
        sum += (u32)bytes[length - 1u] << 8;
    }

    while ((sum >> 16) != 0u) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }

    return (u16)(~sum);
}

static bool net_send_ethernet_frame(const u8 destination_mac[NET_MAC_ADDRESS_LENGTH],
                                    u16 ether_type,
                                    const void* payload,
                                    u16 payload_length) {
    u8 frame[NET_ETHERNET_FRAME_MAX];
    ethernet_header_t* header = (ethernet_header_t*)frame;
    u16 frame_length = (u16)(sizeof(*header) + payload_length);

    if (!g_net.initialized || payload == NULL || frame_length > sizeof(frame)) {
        return false;
    }

    memcpy(header->destination, destination_mac, NET_MAC_ADDRESS_LENGTH);
    memcpy(header->source, g_net.mac_address, NET_MAC_ADDRESS_LENGTH);
    header->ether_type = net_to_be16(ether_type);
    memcpy(frame + sizeof(*header), payload, payload_length);

    if (!pcnet_send(frame, frame_length)) {
        ++g_net.status.dropped_frames;
        return false;
    }

    ++g_net.status.tx_frames;
    return true;
}

static bool net_send_arp_packet(u16 operation,
                                const u8 target_mac[NET_MAC_ADDRESS_LENGTH],
                                const u8 target_ipv4[NET_IPV4_ADDRESS_LENGTH]) {
    arp_packet_t packet;
    const u8* destination_mac = operation == NET_ARP_OPERATION_REQUEST ? k_broadcast_mac : target_mac;

    if (!g_net.initialized || !g_net.ipv4_configured || target_ipv4 == NULL) {
        return false;
    }

    memset(&packet, 0, sizeof(packet));
    packet.hardware_type = net_to_be16(NET_ARP_HARDWARE_ETHERNET);
    packet.protocol_type = net_to_be16(NET_ETHERTYPE_IPV4);
    packet.hardware_length = NET_MAC_ADDRESS_LENGTH;
    packet.protocol_length = NET_IPV4_ADDRESS_LENGTH;
    packet.operation = net_to_be16(operation);
    memcpy(packet.sender_hardware_address, g_net.mac_address, NET_MAC_ADDRESS_LENGTH);
    memcpy(packet.sender_protocol_address, g_net.ipv4_address, NET_IPV4_ADDRESS_LENGTH);
    memcpy(packet.target_hardware_address,
           operation == NET_ARP_OPERATION_REQUEST ? k_zero_mac : target_mac,
           NET_MAC_ADDRESS_LENGTH);
    memcpy(packet.target_protocol_address, target_ipv4, NET_IPV4_ADDRESS_LENGTH);

    return net_send_ethernet_frame(destination_mac, NET_ETHERTYPE_ARP, &packet, sizeof(packet));
}

static void net_queue_ipv4_packet(const ethernet_header_t* ethernet_header,
                                  const ipv4_header_t* ipv4_header,
                                  const u8* payload,
                                  u16 payload_length) {
    net_ipv4_packet_t* packet;

    if (g_net.ipv4_queue_count >= NET_IPV4_QUEUE_SIZE) {
        ++g_net.status.dropped_frames;
        return;
    }

    packet = &g_net.ipv4_queue[g_net.ipv4_queue_tail];
    memset(packet, 0, sizeof(*packet));
    memcpy(packet->source_mac, ethernet_header->source, NET_MAC_ADDRESS_LENGTH);
    memcpy(packet->source_ipv4, ipv4_header->source_address, NET_IPV4_ADDRESS_LENGTH);
    memcpy(packet->destination_ipv4, ipv4_header->destination_address, NET_IPV4_ADDRESS_LENGTH);
    packet->protocol = ipv4_header->protocol;
    packet->payload_length = payload_length;
    memcpy(packet->payload, payload, payload_length);

    g_net.ipv4_queue_tail = (g_net.ipv4_queue_tail + 1u) % NET_IPV4_QUEUE_SIZE;
    ++g_net.ipv4_queue_count;
    net_status_sync();
}

static void net_handle_arp(const ethernet_header_t* ethernet_header, const u8* payload, u16 payload_length) {
    const arp_packet_t* packet = (const arp_packet_t*)payload;

    (void)ethernet_header;
    if (payload_length < sizeof(*packet)) {
        ++g_net.status.dropped_frames;
        return;
    }
    if (net_from_be16(packet->hardware_type) != NET_ARP_HARDWARE_ETHERNET ||
        net_from_be16(packet->protocol_type) != NET_ETHERTYPE_IPV4 ||
        packet->hardware_length != NET_MAC_ADDRESS_LENGTH ||
        packet->protocol_length != NET_IPV4_ADDRESS_LENGTH) {
        ++g_net.status.dropped_frames;
        return;
    }

    net_store_arp_entry(packet->sender_protocol_address, packet->sender_hardware_address);
    if (!g_net.ipv4_configured) {
        return;
    }

    if (net_from_be16(packet->operation) == NET_ARP_OPERATION_REQUEST &&
        net_ipv4_equal(packet->target_protocol_address, g_net.ipv4_address)) {
        (void)net_send_arp_packet(NET_ARP_OPERATION_REPLY,
                                  packet->sender_hardware_address,
                                  packet->sender_protocol_address);
    }
}

static void net_handle_ipv4(const ethernet_header_t* ethernet_header, const u8* payload, u16 payload_length) {
    const ipv4_header_t* header = (const ipv4_header_t*)payload;
    u16 header_length;
    u16 total_length;
    u16 queued_payload_length;

    if (!g_net.ipv4_configured || payload_length < sizeof(*header)) {
        ++g_net.status.dropped_frames;
        return;
    }
    if ((header->version_and_ihl >> 4) != 4u) {
        ++g_net.status.dropped_frames;
        return;
    }

    header_length = (u16)((header->version_and_ihl & 0x0Fu) * 4u);
    if (header_length < NET_IPV4_HEADER_MIN_LENGTH || header_length > payload_length) {
        ++g_net.status.dropped_frames;
        return;
    }

    total_length = net_from_be16(header->total_length);
    if (total_length < header_length || total_length > payload_length) {
        ++g_net.status.dropped_frames;
        return;
    }
    if (net_ipv4_checksum(header, header_length) != 0u) {
        ++g_net.status.dropped_frames;
        return;
    }
    if (!net_ipv4_equal(header->destination_address, g_net.ipv4_address) &&
        !net_ipv4_is_broadcast(header->destination_address)) {
        return;
    }

    net_store_arp_entry(header->source_address, ethernet_header->source);
    queued_payload_length = (u16)(total_length - header_length);
    if (queued_payload_length > NET_IPV4_PAYLOAD_MAX) {
        ++g_net.status.dropped_frames;
        return;
    }

    net_queue_ipv4_packet(ethernet_header,
                          header,
                          payload + header_length,
                          queued_payload_length);
}

static void net_process_frame(const u8* frame, u16 frame_length) {
    const ethernet_header_t* header;
    u16 ether_type;
    const u8* payload;
    u16 payload_length;

    if (frame == NULL || frame_length < sizeof(ethernet_header_t)) {
        ++g_net.status.dropped_frames;
        return;
    }

    header = (const ethernet_header_t*)frame;
    if (!net_mac_equal(header->destination, g_net.mac_address) &&
        !net_mac_equal(header->destination, k_broadcast_mac)) {
        return;
    }

    ether_type = net_from_be16(header->ether_type);
    payload = frame + sizeof(*header);
    payload_length = (u16)(frame_length - sizeof(*header));
    if (ether_type == NET_ETHERTYPE_ARP) {
        net_handle_arp(header, payload, payload_length);
    } else if (ether_type == NET_ETHERTYPE_IPV4) {
        net_handle_ipv4(header, payload, payload_length);
    }
}

bool net_initialize(void) {
    pcnet_status_t pcnet_status;

    if (g_net.initialized) {
        return true;
    }

    memset(&g_net, 0, sizeof(g_net));
    if (!pcnet_initialize()) {
        serial_write("[net] ethernet stack unavailable because pcnet failed\n");
        return false;
    }

    pcnet_get_status(&pcnet_status);
    if (!pcnet_status.initialized) {
        serial_write("[net] ethernet stack unavailable because pcnet is not ready\n");
        return false;
    }

    g_net.initialized = true;
    memcpy(g_net.mac_address, pcnet_status.mac, NET_MAC_ADDRESS_LENGTH);
    net_status_sync();
    serial_write("[net] ethernet/arp/ipv4 stack initialized\n");
    return true;
}

bool net_configure_ipv4_static(const u8 address[NET_IPV4_ADDRESS_LENGTH],
                               const u8 subnet_mask[NET_IPV4_ADDRESS_LENGTH],
                               const u8 gateway[NET_IPV4_ADDRESS_LENGTH]) {
    if (!g_net.initialized || address == NULL || subnet_mask == NULL || gateway == NULL) {
        return false;
    }

    memcpy(g_net.ipv4_address, address, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_net.subnet_mask, subnet_mask, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_net.gateway, gateway, NET_IPV4_ADDRESS_LENGTH);
    g_net.ipv4_configured = true;
    net_store_arp_entry(g_net.ipv4_address, g_net.mac_address);
    net_status_sync();

    serial_write("[net] static ipv4 configured\n");
    (void)net_send_arp_packet(NET_ARP_OPERATION_REQUEST, k_zero_mac, g_net.ipv4_address);
    return true;
}

void net_service(void) {
    u8 frame[NET_ETHERNET_FRAME_MAX];
    u16 frame_length = 0u;
    u32 budget;

    if (!g_net.initialized) {
        return;
    }

    for (budget = 0u; budget < NET_RX_BUDGET; ++budget) {
        if (!pcnet_poll_receive(frame, sizeof(frame), &frame_length)) {
            break;
        }

        ++g_net.status.rx_frames;
        net_process_frame(frame, frame_length);
    }

    net_status_sync();
}

bool net_arp_resolve(const u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH], u8 out_mac[NET_MAC_ADDRESS_LENGTH]) {
    u32 spin;
    u8 next_hop[NET_IPV4_ADDRESS_LENGTH];

    if (!g_net.initialized || !g_net.ipv4_configured || ipv4_address == NULL || out_mac == NULL) {
        return false;
    }
    if (net_ipv4_is_broadcast(ipv4_address)) {
        memcpy(out_mac, k_broadcast_mac, NET_MAC_ADDRESS_LENGTH);
        return true;
    }

    memcpy(next_hop, ipv4_address, sizeof(next_hop));
    if (!net_ipv4_same_subnet(ipv4_address, g_net.ipv4_address, g_net.subnet_mask) &&
        !net_ipv4_is_zero(g_net.gateway)) {
        memcpy(next_hop, g_net.gateway, sizeof(next_hop));
    }

    if (net_lookup_arp_entry(next_hop, out_mac)) {
        return true;
    }
    if (!net_send_arp_packet(NET_ARP_OPERATION_REQUEST, k_zero_mac, next_hop)) {
        return false;
    }

    for (spin = 0u; spin < NET_ARP_WAIT_SPINS; ++spin) {
        net_service();
        if (net_lookup_arp_entry(next_hop, out_mac)) {
            return true;
        }
        io_wait();
    }

    return false;
}

bool net_ipv4_send(u8 protocol,
                   const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                   const void* payload,
                   u16 payload_length) {
    u8 destination_mac[NET_MAC_ADDRESS_LENGTH];
    u8 packet[sizeof(ipv4_header_t) + NET_IPV4_PAYLOAD_MAX];
    ipv4_header_t* header = (ipv4_header_t*)packet;
    u16 total_length = (u16)(sizeof(*header) + payload_length);

    if (!g_net.initialized || !g_net.ipv4_configured || destination_ipv4 == NULL) {
        return false;
    }
    if ((payload_length != 0u && payload == NULL) || payload_length > NET_IPV4_PAYLOAD_MAX) {
        return false;
    }
    if (!net_arp_resolve(destination_ipv4, destination_mac)) {
        return false;
    }

    memset(packet, 0, total_length);
    header->version_and_ihl = NET_IPV4_VERSION_AND_IHL;
    header->differentiated_services = 0u;
    header->total_length = net_to_be16(total_length);
    header->identification = net_to_be16(++g_net.next_ipv4_id);
    header->flags_and_fragment_offset = net_to_be16(NET_IPV4_FLAG_DONT_FRAGMENT);
    header->ttl = NET_IPV4_TTL_DEFAULT;
    header->protocol = protocol;
    memcpy(header->source_address, g_net.ipv4_address, NET_IPV4_ADDRESS_LENGTH);
    memcpy(header->destination_address, destination_ipv4, NET_IPV4_ADDRESS_LENGTH);
    if (payload_length != 0u) {
        memcpy(packet + sizeof(*header), payload, payload_length);
    }
    header->header_checksum = net_to_be16(net_ipv4_checksum(header, sizeof(*header)));

    return net_send_ethernet_frame(destination_mac, NET_ETHERTYPE_IPV4, packet, total_length);
}

bool net_poll_ipv4_packet(net_ipv4_packet_t* out_packet) {
    if (out_packet == NULL || g_net.ipv4_queue_count == 0u) {
        return false;
    }

    *out_packet = g_net.ipv4_queue[g_net.ipv4_queue_head];
    g_net.ipv4_queue_head = (g_net.ipv4_queue_head + 1u) % NET_IPV4_QUEUE_SIZE;
    --g_net.ipv4_queue_count;
    net_status_sync();
    return true;
}

void net_get_status(net_status_t* out_status) {
    if (out_status == NULL) {
        return;
    }

    net_status_sync();
    *out_status = g_net.status;
}
