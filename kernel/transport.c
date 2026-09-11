#include "jabulos.h"

#define NET_UDP_QUEUE_SIZE 8u
#define NET_TCP_CONNECTION_MAX 4u
#define NET_TCP_RECEIVE_BUFFER_SIZE 4096u
#define NET_TCP_FLAG_FIN 0x01u
#define NET_TCP_FLAG_SYN 0x02u
#define NET_TCP_FLAG_RST 0x04u
#define NET_TCP_FLAG_PSH 0x08u
#define NET_TCP_FLAG_ACK 0x10u
#define NET_TCP_HEADER_LENGTH 20u
#define NET_UDP_HEADER_LENGTH 8u
#define NET_DHCP_CLIENT_PORT 68u
#define NET_DHCP_SERVER_PORT 67u
#define NET_DNS_SERVER_PORT 53u
#define NET_DHCP_MAGIC_COOKIE 0x63825363u
#define NET_DHCP_OPTION_PAD 0u
#define NET_DHCP_OPTION_SUBNET_MASK 1u
#define NET_DHCP_OPTION_ROUTER 3u
#define NET_DHCP_OPTION_DNS 6u
#define NET_DHCP_OPTION_REQUESTED_IP 50u
#define NET_DHCP_OPTION_LEASE_TIME 51u
#define NET_DHCP_OPTION_MESSAGE_TYPE 53u
#define NET_DHCP_OPTION_SERVER_ID 54u
#define NET_DHCP_OPTION_PARAMETER_REQUEST_LIST 55u
#define NET_DHCP_OPTION_END 255u
#define NET_DHCP_MESSAGE_DISCOVER 1u
#define NET_DHCP_MESSAGE_OFFER 2u
#define NET_DHCP_MESSAGE_REQUEST 3u
#define NET_DHCP_MESSAGE_ACK 5u
#define NET_DNS_TYPE_A 1u
#define NET_DNS_CLASS_IN 1u
#define NET_DHCP_TIMEOUT_SECONDS 5u
#define NET_DNS_TIMEOUT_SECONDS 5u
#define NET_TCP_TIMEOUT_SECONDS 5u
#define NET_TCP_MAX_RETRIES 3u
#define NET_EPHEMERAL_PORT_START 49152u
#define NET_HTTP_HEADER_BUFFER_SIZE 4096u
#define NET_HTTP_REQUEST_BUFFER_SIZE 1024u
#define NET_HTTP_CHUNK_BUFFER_SIZE 1024u
#define NET_HTTP_MAX_REDIRECTS 4u

typedef struct __attribute__((packed)) {
    u16 source_port;
    u16 destination_port;
    u16 length;
    u16 checksum;
} udp_header_t;

typedef struct __attribute__((packed)) {
    u16 source_port;
    u16 destination_port;
    u32 sequence_number;
    u32 acknowledgement_number;
    u8 data_offset_reserved;
    u8 flags;
    u16 window_size;
    u16 checksum;
    u16 urgent_pointer;
} tcp_header_t;

typedef struct __attribute__((packed)) {
    u16 source_port;
    u16 destination_port;
    u16 tcp_length;
    u16 checksum_placeholder;
    u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 reserved;
    u8 protocol;
} tcp_pseudo_header_t;

typedef struct __attribute__((packed)) {
    u16 source_port;
    u16 destination_port;
    u16 udp_length;
    u16 checksum_placeholder;
    u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 reserved;
    u8 protocol;
} udp_pseudo_header_t;

typedef struct __attribute__((packed)) {
    u8 operation;
    u8 hardware_type;
    u8 hardware_address_length;
    u8 hops;
    u32 transaction_id;
    u16 seconds_elapsed;
    u16 flags;
    u8 client_ip_address[NET_IPV4_ADDRESS_LENGTH];
    u8 your_ip_address[NET_IPV4_ADDRESS_LENGTH];
    u8 server_ip_address[NET_IPV4_ADDRESS_LENGTH];
    u8 gateway_ip_address[NET_IPV4_ADDRESS_LENGTH];
    u8 client_hardware_address[16];
    u8 server_host_name[64];
    u8 boot_file_name[128];
    u32 magic_cookie;
} dhcp_header_t;

typedef struct __attribute__((packed)) {
    u16 transaction_id;
    u16 flags;
    u16 question_count;
    u16 answer_count;
    u16 authority_count;
    u16 additional_count;
} dns_header_t;

typedef enum {
    TCP_STATE_FREE = 0,
    TCP_STATE_SYN_SENT = 1,
    TCP_STATE_ESTABLISHED = 2,
    TCP_STATE_FIN_WAIT_1 = 3,
    TCP_STATE_FIN_WAIT_2 = 4,
    TCP_STATE_CLOSE_WAIT = 5,
    TCP_STATE_LAST_ACK = 6
} tcp_connection_state_t;

typedef struct {
    bool valid;
    net_udp_packet_t packet;
} udp_queue_entry_t;

typedef struct {
    bool used;
    tcp_connection_state_t state;
    bool fin_sent;
    u8 remote_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u16 local_port;
    u16 remote_port;
    u32 initial_sequence;
    u32 send_unacknowledged;
    u32 send_next;
    u32 receive_next;
    u32 receive_window;
    u32 last_activity_second;
    u8 receive_buffer[NET_TCP_RECEIVE_BUFFER_SIZE];
    u16 receive_length;
} tcp_connection_t;

typedef struct {
    udp_queue_entry_t udp_queue[NET_UDP_QUEUE_SIZE];
    u32 udp_head;
    u32 udp_tail;
    u32 udp_count;
    tcp_connection_t tcp_connections[NET_TCP_CONNECTION_MAX];
    net_dhcp_status_t dhcp_status;
    u8 dns_server[NET_IPV4_ADDRESS_LENGTH];
    u16 next_ephemeral_port;
} transport_state_t;

static transport_state_t g_transport = {};

static u16 transport_to_be16(u16 value) {
    return (u16)((value << 8) | (value >> 8));
}

static u32 transport_to_be32(u32 value) {
    return ((value & 0x000000FFu) << 24) |
           ((value & 0x0000FF00u) << 8) |
           ((value & 0x00FF0000u) >> 8) |
           ((value & 0xFF000000u) >> 24);
}

static u16 transport_from_be16(u16 value) {
    return transport_to_be16(value);
}

static u32 transport_from_be32(u32 value) {
    return transport_to_be32(value);
}

static bool transport_ipv4_equal(const u8 left[NET_IPV4_ADDRESS_LENGTH],
                                 const u8 right[NET_IPV4_ADDRESS_LENGTH]) {
    return memcmp(left, right, NET_IPV4_ADDRESS_LENGTH) == 0;
}

static bool transport_ipv4_is_zero(const u8 address[NET_IPV4_ADDRESS_LENGTH]) {
    static const u8 k_zero_ipv4[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    return transport_ipv4_equal(address, k_zero_ipv4);
}

static u32 transport_elapsed_seconds(u32 start_second, u32 current_second) {
    if (current_second >= start_second) {
        return current_second - start_second;
    }
    return (86400u - start_second) + current_second;
}

static u16 transport_checksum(const void* data, u16 length) {
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

static void transport_wait_iteration(void) {
    net_service();
    net_transport_service();
    io_wait();
}

static void transport_tcp_refresh_receive_window(tcp_connection_t* connection) {
    if (connection == NULL) {
        return;
    }

    connection->receive_window = NET_TCP_RECEIVE_BUFFER_SIZE - connection->receive_length;
}

static u16 transport_allocate_ephemeral_port(void) {
    if (g_transport.next_ephemeral_port < NET_EPHEMERAL_PORT_START) {
        g_transport.next_ephemeral_port = NET_EPHEMERAL_PORT_START;
    }
    ++g_transport.next_ephemeral_port;
    if (g_transport.next_ephemeral_port == 0u) {
        g_transport.next_ephemeral_port = NET_EPHEMERAL_PORT_START;
    }
    return g_transport.next_ephemeral_port;
}

static tcp_connection_t* transport_get_connection(int connection_id) {
    if (connection_id < 0 || (u32)connection_id >= NET_TCP_CONNECTION_MAX) {
        return NULL;
    }
    return &g_transport.tcp_connections[connection_id];
}

static tcp_connection_t* transport_find_connection(const u8 remote_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                                   u16 local_port,
                                                   u16 remote_port) {
    u32 index;

    for (index = 0u; index < NET_TCP_CONNECTION_MAX; ++index) {
        tcp_connection_t* connection = &g_transport.tcp_connections[index];
        if (connection->used &&
            connection->local_port == local_port &&
            connection->remote_port == remote_port &&
            transport_ipv4_equal(connection->remote_ipv4, remote_ipv4)) {
            return connection;
        }
    }

    return NULL;
}

static u16 transport_udp_checksum(const u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                  const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                  const void* segment,
                                  u16 segment_length) {
    udp_pseudo_header_t pseudo_header;
    u8 buffer[sizeof(pseudo_header) + NET_UDP_HEADER_LENGTH + NET_UDP_PAYLOAD_MAX + 1u];

    memset(&pseudo_header, 0, sizeof(pseudo_header));
    memcpy(pseudo_header.source_ipv4, source_ipv4, NET_IPV4_ADDRESS_LENGTH);
    memcpy(pseudo_header.destination_ipv4, destination_ipv4, NET_IPV4_ADDRESS_LENGTH);
    pseudo_header.protocol = NET_IPV4_PROTOCOL_UDP;
    pseudo_header.udp_length = transport_to_be16(segment_length);

    memcpy(buffer, &pseudo_header, sizeof(pseudo_header));
    memcpy(buffer + sizeof(pseudo_header), segment, segment_length);
    if ((segment_length & 1u) != 0u) {
        buffer[sizeof(pseudo_header) + segment_length] = 0u;
        ++segment_length;
    }

    return transport_checksum(buffer, (u16)(sizeof(pseudo_header) + segment_length));
}

static u16 transport_tcp_checksum(const u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                  const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                  const void* segment,
                                  u16 segment_length) {
    tcp_pseudo_header_t pseudo_header;
    u8 buffer[sizeof(pseudo_header) + NET_TCP_HEADER_LENGTH + NET_TCP_PAYLOAD_MAX + 1u];

    memset(&pseudo_header, 0, sizeof(pseudo_header));
    memcpy(pseudo_header.source_ipv4, source_ipv4, NET_IPV4_ADDRESS_LENGTH);
    memcpy(pseudo_header.destination_ipv4, destination_ipv4, NET_IPV4_ADDRESS_LENGTH);
    pseudo_header.protocol = NET_IPV4_PROTOCOL_TCP;
    pseudo_header.tcp_length = transport_to_be16(segment_length);

    memcpy(buffer, &pseudo_header, sizeof(pseudo_header));
    memcpy(buffer + sizeof(pseudo_header), segment, segment_length);
    if ((segment_length & 1u) != 0u) {
        buffer[sizeof(pseudo_header) + segment_length] = 0u;
        ++segment_length;
    }

    return transport_checksum(buffer, (u16)(sizeof(pseudo_header) + segment_length));
}

static bool transport_send_udp_datagram(const u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                        u16 source_port,
                                        const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                        u16 destination_port,
                                        const void* payload,
                                        u16 payload_length) {
    u8 segment[NET_UDP_HEADER_LENGTH + NET_UDP_PAYLOAD_MAX];
    udp_header_t* header = (udp_header_t*)segment;
    u16 segment_length = (u16)(sizeof(*header) + payload_length);

    if ((payload_length != 0u && payload == NULL) || payload_length > NET_UDP_PAYLOAD_MAX) {
        return false;
    }

    memset(segment, 0, segment_length);
    header->source_port = transport_to_be16(source_port);
    header->destination_port = transport_to_be16(destination_port);
    header->length = transport_to_be16(segment_length);
    if (payload_length != 0u) {
        memcpy(segment + sizeof(*header), payload, payload_length);
    }
    header->checksum = transport_udp_checksum(source_ipv4, destination_ipv4, segment, segment_length);
    if (header->checksum == 0u) {
        header->checksum = 0xFFFFu;
    }

    return net_ipv4_send(NET_IPV4_PROTOCOL_UDP, destination_ipv4, segment, segment_length);
}

static bool transport_send_dhcp_message(u8 message_type,
                                        u32 transaction_id,
                                        const u8 requested_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                        const u8 server_ipv4[NET_IPV4_ADDRESS_LENGTH]) {
    u8 frame[14u + 20u + NET_UDP_HEADER_LENGTH + sizeof(dhcp_header_t) + 64u];
    pcnet_status_t pcnet_status;
    dhcp_header_t* dhcp;
    udp_header_t* udp;
    u8* ipv4;
    u8* ethernet;
    u8* options;
    u16 udp_length;
    u16 ipv4_length;
    u16 frame_length;
    u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH] = {255u, 255u, 255u, 255u};
    u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    static const u8 k_broadcast_mac[NET_MAC_ADDRESS_LENGTH] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};

    pcnet_get_status(&pcnet_status);
    ethernet = frame;
    memcpy(ethernet + 0u, k_broadcast_mac, NET_MAC_ADDRESS_LENGTH);
    memcpy(ethernet + 6u, pcnet_status.mac, NET_MAC_ADDRESS_LENGTH);
    ethernet[12] = 0x08u;
    ethernet[13] = 0x00u;

    ipv4 = ethernet + 14u;
    memset(ipv4, 0, 20u);
    ipv4[0] = 0x45u;
    ipv4[8] = 64u;
    ipv4[9] = NET_IPV4_PROTOCOL_UDP;
    memcpy(ipv4 + 12u, source_ipv4, NET_IPV4_ADDRESS_LENGTH);
    memcpy(ipv4 + 16u, destination_ipv4, NET_IPV4_ADDRESS_LENGTH);

    udp = (udp_header_t*)(ipv4 + 20u);
    memset(udp, 0, sizeof(*udp));
    udp->source_port = transport_to_be16(NET_DHCP_CLIENT_PORT);
    udp->destination_port = transport_to_be16(NET_DHCP_SERVER_PORT);

    dhcp = (dhcp_header_t*)((u8*)udp + sizeof(*udp));
    memset(dhcp, 0, sizeof(*dhcp));
    dhcp->operation = 1u;
    dhcp->hardware_type = 1u;
    dhcp->hardware_address_length = NET_MAC_ADDRESS_LENGTH;
    dhcp->transaction_id = transport_to_be32(transaction_id);
    dhcp->flags = transport_to_be16(0x8000u);
    memcpy(dhcp->client_hardware_address, pcnet_status.mac, NET_MAC_ADDRESS_LENGTH);
    dhcp->magic_cookie = transport_to_be32(NET_DHCP_MAGIC_COOKIE);

    options = (u8*)(dhcp + 1);
    *options++ = NET_DHCP_OPTION_MESSAGE_TYPE;
    *options++ = 1u;
    *options++ = message_type;

    if (message_type == NET_DHCP_MESSAGE_REQUEST && requested_ipv4 != NULL && server_ipv4 != NULL) {
        *options++ = NET_DHCP_OPTION_REQUESTED_IP;
        *options++ = NET_IPV4_ADDRESS_LENGTH;
        memcpy(options, requested_ipv4, NET_IPV4_ADDRESS_LENGTH);
        options += NET_IPV4_ADDRESS_LENGTH;

        *options++ = NET_DHCP_OPTION_SERVER_ID;
        *options++ = NET_IPV4_ADDRESS_LENGTH;
        memcpy(options, server_ipv4, NET_IPV4_ADDRESS_LENGTH);
        options += NET_IPV4_ADDRESS_LENGTH;
    }

    *options++ = NET_DHCP_OPTION_PARAMETER_REQUEST_LIST;
    *options++ = 4u;
    *options++ = NET_DHCP_OPTION_SUBNET_MASK;
    *options++ = NET_DHCP_OPTION_ROUTER;
    *options++ = NET_DHCP_OPTION_DNS;
    *options++ = NET_DHCP_OPTION_LEASE_TIME;
    *options++ = NET_DHCP_OPTION_END;

    udp_length = (u16)(sizeof(*udp) + sizeof(*dhcp) + (u16)(options - (u8*)(dhcp + 1)));
    udp->length = transport_to_be16(udp_length);
    udp->checksum = transport_udp_checksum(source_ipv4, destination_ipv4, udp, udp_length);
    if (udp->checksum == 0u) {
        udp->checksum = 0xFFFFu;
    }

    ipv4_length = (u16)(20u + udp_length);
    ipv4[2] = (u8)(ipv4_length >> 8);
    ipv4[3] = (u8)(ipv4_length & 0xFFu);
    ipv4[6] = 0x40u;
    {
        u16 ip_checksum = transport_checksum(ipv4, 20u);
        ipv4[10] = (u8)(ip_checksum >> 8);
        ipv4[11] = (u8)(ip_checksum & 0xFFu);
    }

    frame_length = (u16)(14u + ipv4_length);
    return pcnet_send(frame, frame_length);
}

static bool transport_parse_dhcp_options(const u8* options,
                                         u16 options_length,
                                         u8* out_message_type,
                                         u8 out_subnet_mask[NET_IPV4_ADDRESS_LENGTH],
                                         u8 out_gateway[NET_IPV4_ADDRESS_LENGTH],
                                         u8 out_dns[NET_IPV4_ADDRESS_LENGTH],
                                         u8 out_server[NET_IPV4_ADDRESS_LENGTH],
                                         u32* out_lease_seconds) {
    u16 offset = 0u;

    if (out_message_type != NULL) {
        *out_message_type = 0u;
    }
    if (out_lease_seconds != NULL) {
        *out_lease_seconds = 0u;
    }
    while (offset < options_length) {
        u8 option = options[offset++];
        u8 option_length;

        if (option == NET_DHCP_OPTION_END) {
            return true;
        }
        if (option == NET_DHCP_OPTION_PAD) {
            continue;
        }
        if (offset >= options_length) {
            return false;
        }

        option_length = options[offset++];
        if (option_length > options_length - offset) {
            return false;
        }

        if (option == NET_DHCP_OPTION_MESSAGE_TYPE && option_length >= 1u && out_message_type != NULL) {
            *out_message_type = options[offset];
        } else if (option == NET_DHCP_OPTION_SUBNET_MASK && option_length >= NET_IPV4_ADDRESS_LENGTH) {
            memcpy(out_subnet_mask, options + offset, NET_IPV4_ADDRESS_LENGTH);
        } else if (option == NET_DHCP_OPTION_ROUTER && option_length >= NET_IPV4_ADDRESS_LENGTH) {
            memcpy(out_gateway, options + offset, NET_IPV4_ADDRESS_LENGTH);
        } else if (option == NET_DHCP_OPTION_DNS && option_length >= NET_IPV4_ADDRESS_LENGTH) {
            memcpy(out_dns, options + offset, NET_IPV4_ADDRESS_LENGTH);
        } else if (option == NET_DHCP_OPTION_SERVER_ID && option_length >= NET_IPV4_ADDRESS_LENGTH) {
            memcpy(out_server, options + offset, NET_IPV4_ADDRESS_LENGTH);
        } else if (option == NET_DHCP_OPTION_LEASE_TIME && option_length >= 4u && out_lease_seconds != NULL) {
            *out_lease_seconds = ((u32)options[offset] << 24) |
                                 ((u32)options[offset + 1u] << 16) |
                                 ((u32)options[offset + 2u] << 8) |
                                 (u32)options[offset + 3u];
        }

        offset = (u16)(offset + option_length);
    }

    return true;
}

static bool transport_wait_for_dhcp_message(u32 transaction_id,
                                            u8 expected_message_type,
                                            u8 out_your_ipv4[NET_IPV4_ADDRESS_LENGTH],
                                            u8 out_subnet_mask[NET_IPV4_ADDRESS_LENGTH],
                                            u8 out_gateway[NET_IPV4_ADDRESS_LENGTH],
                                            u8 out_dns[NET_IPV4_ADDRESS_LENGTH],
                                            u8 out_server[NET_IPV4_ADDRESS_LENGTH],
                                            u32* out_lease_seconds) {
    u32 start_second = rtc_read_seconds_of_day();

    for (;;) {
        net_udp_packet_t packet;
        u8 message_type = 0u;

        transport_wait_iteration();
        while (net_udp_poll(NET_DHCP_CLIENT_PORT, &packet)) {
            dhcp_header_t* dhcp;
            u16 options_length;
            pcnet_status_t pcnet_status;

            if (packet.source_port != NET_DHCP_SERVER_PORT || packet.payload_length < sizeof(dhcp_header_t)) {
                continue;
            }

            dhcp = (dhcp_header_t*)packet.payload;
            pcnet_get_status(&pcnet_status);
            if (dhcp->operation != 2u ||
                dhcp->hardware_type != 1u ||
                dhcp->hardware_address_length != NET_MAC_ADDRESS_LENGTH ||
                transport_from_be32(dhcp->transaction_id) != transaction_id ||
                memcmp(dhcp->client_hardware_address, pcnet_status.mac, NET_MAC_ADDRESS_LENGTH) != 0 ||
                transport_from_be32(dhcp->magic_cookie) != NET_DHCP_MAGIC_COOKIE) {
                continue;
            }

            options_length = (u16)(packet.payload_length - sizeof(dhcp_header_t));
            if (!transport_parse_dhcp_options(packet.payload + sizeof(dhcp_header_t),
                                              options_length,
                                              &message_type,
                                              out_subnet_mask,
                                              out_gateway,
                                              out_dns,
                                              out_server,
                                              out_lease_seconds)) {
                continue;
            }
            if (message_type != expected_message_type) {
                continue;
            }

            memcpy(out_your_ipv4, dhcp->your_ip_address, NET_IPV4_ADDRESS_LENGTH);
            return true;
        }

        if (transport_elapsed_seconds(start_second, rtc_read_seconds_of_day()) >= NET_DHCP_TIMEOUT_SECONDS) {
            return false;
        }
    }
}

static bool transport_tcp_send_segment(tcp_connection_t* connection,
                                       u8 flags,
                                       const void* payload,
                                       u16 payload_length) {
    net_status_t net_status;
    u8 segment[NET_TCP_HEADER_LENGTH + NET_TCP_PAYLOAD_MAX];
    tcp_header_t* header = (tcp_header_t*)segment;
    u16 segment_length = (u16)(sizeof(*header) + payload_length);

    if (connection == NULL || payload_length > NET_TCP_PAYLOAD_MAX) {
        return false;
    }
    net_get_status(&net_status);
    if (!net_status.ipv4_configured) {
        return false;
    }

    memset(segment, 0, segment_length);
    header->source_port = transport_to_be16(connection->local_port);
    header->destination_port = transport_to_be16(connection->remote_port);
    header->sequence_number = transport_to_be32(connection->send_next);
    header->acknowledgement_number = transport_to_be32(connection->receive_next);
    header->data_offset_reserved = (u8)((NET_TCP_HEADER_LENGTH / 4u) << 4);
    header->flags = flags;
    header->window_size = transport_to_be16((u16)connection->receive_window);
    if (payload_length != 0u) {
        memcpy(segment + sizeof(*header), payload, payload_length);
    }
    header->checksum = transport_tcp_checksum(net_status.ipv4_address, connection->remote_ipv4, segment, segment_length);

    if (!net_ipv4_send(NET_IPV4_PROTOCOL_TCP, connection->remote_ipv4, segment, segment_length)) {
        return false;
    }

    if ((flags & NET_TCP_FLAG_SYN) != 0u || (flags & NET_TCP_FLAG_FIN) != 0u) {
        ++connection->send_next;
    }
    connection->send_next += payload_length;
    connection->last_activity_second = rtc_read_seconds_of_day();
    return true;
}

static bool transport_tcp_wait_for_ack(tcp_connection_t* connection, u32 target_ack) {
    u32 start_second = rtc_read_seconds_of_day();

    while (connection != NULL && connection->used) {
        if (connection->send_unacknowledged >= target_ack) {
            return true;
        }
        transport_wait_iteration();
        if (transport_elapsed_seconds(start_second, rtc_read_seconds_of_day()) >= NET_TCP_TIMEOUT_SECONDS) {
            return false;
        }
    }

    return false;
}

static void transport_handle_udp_packet(const net_ipv4_packet_t* packet) {
    const udp_header_t* header;
    u16 total_length;
    u16 payload_length;
    net_udp_packet_t* queued_packet;

    if (packet == NULL || packet->payload_length < sizeof(udp_header_t) || g_transport.udp_count >= NET_UDP_QUEUE_SIZE) {
        return;
    }

    header = (const udp_header_t*)packet->payload;
    total_length = transport_from_be16(header->length);
    if (total_length < sizeof(*header) || total_length > packet->payload_length) {
        return;
    }

    payload_length = (u16)(total_length - sizeof(*header));
    if (payload_length > NET_UDP_PAYLOAD_MAX) {
        return;
    }

    queued_packet = &g_transport.udp_queue[g_transport.udp_tail].packet;
    memset(queued_packet, 0, sizeof(*queued_packet));
    memcpy(queued_packet->source_ipv4, packet->source_ipv4, NET_IPV4_ADDRESS_LENGTH);
    memcpy(queued_packet->destination_ipv4, packet->destination_ipv4, NET_IPV4_ADDRESS_LENGTH);
    queued_packet->source_port = transport_from_be16(header->source_port);
    queued_packet->destination_port = transport_from_be16(header->destination_port);
    queued_packet->payload_length = payload_length;
    memcpy(queued_packet->payload, packet->payload + sizeof(*header), payload_length);
    g_transport.udp_queue[g_transport.udp_tail].valid = true;
    g_transport.udp_tail = (g_transport.udp_tail + 1u) % NET_UDP_QUEUE_SIZE;
    ++g_transport.udp_count;
}

static void transport_handle_tcp_packet(const net_ipv4_packet_t* packet) {
    const tcp_header_t* header;
    tcp_connection_t* connection;
    u16 header_length;
    u16 payload_length;
    u32 sequence_number;
    u32 acknowledgement_number;

    if (packet == NULL || packet->payload_length < sizeof(tcp_header_t)) {
        return;
    }

    header = (const tcp_header_t*)packet->payload;
    header_length = (u16)(((header->data_offset_reserved >> 4) & 0x0Fu) * 4u);
    if (header_length < sizeof(*header) || header_length > packet->payload_length) {
        return;
    }

    connection = transport_find_connection(packet->source_ipv4,
                                           transport_from_be16(header->destination_port),
                                           transport_from_be16(header->source_port));
    if (connection == NULL) {
        return;
    }

    sequence_number = transport_from_be32(header->sequence_number);
    acknowledgement_number = transport_from_be32(header->acknowledgement_number);
    payload_length = (u16)(packet->payload_length - header_length);

    if ((header->flags & NET_TCP_FLAG_RST) != 0u) {
        memset(connection, 0, sizeof(*connection));
        return;
    }

    if ((header->flags & NET_TCP_FLAG_ACK) != 0u && acknowledgement_number > connection->send_unacknowledged) {
        connection->send_unacknowledged = acknowledgement_number;
    }

    if (connection->state == TCP_STATE_SYN_SENT &&
        (header->flags & (NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK)) == (NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK) &&
        acknowledgement_number == connection->initial_sequence + 1u) {
        connection->receive_next = sequence_number + 1u;
        connection->send_unacknowledged = acknowledgement_number;
        connection->state = TCP_STATE_ESTABLISHED;
        (void)transport_tcp_send_segment(connection, NET_TCP_FLAG_ACK, NULL, 0u);
        return;
    }

    if (payload_length != 0u && sequence_number == connection->receive_next) {
        u16 copy_length = payload_length;
        if (copy_length > (u16)(NET_TCP_RECEIVE_BUFFER_SIZE - connection->receive_length)) {
            copy_length = (u16)(NET_TCP_RECEIVE_BUFFER_SIZE - connection->receive_length);
        }
        if (copy_length != 0u) {
            memcpy(connection->receive_buffer + connection->receive_length,
                   packet->payload + header_length,
                   copy_length);
            connection->receive_length = (u16)(connection->receive_length + copy_length);
            connection->receive_next += copy_length;
        }
        transport_tcp_refresh_receive_window(connection);
        (void)transport_tcp_send_segment(connection, NET_TCP_FLAG_ACK, NULL, 0u);
    }

    if ((header->flags & NET_TCP_FLAG_FIN) != 0u && sequence_number + payload_length == connection->receive_next) {
        ++connection->receive_next;
        (void)transport_tcp_send_segment(connection, NET_TCP_FLAG_ACK, NULL, 0u);
        if (connection->state == TCP_STATE_FIN_WAIT_1 || connection->state == TCP_STATE_FIN_WAIT_2) {
            memset(connection, 0, sizeof(*connection));
        } else {
            connection->state = TCP_STATE_CLOSE_WAIT;
        }
    }

    if (connection->state == TCP_STATE_FIN_WAIT_1 && connection->send_unacknowledged >= connection->send_next) {
        connection->state = TCP_STATE_FIN_WAIT_2;
    } else if (connection->state == TCP_STATE_LAST_ACK && connection->send_unacknowledged >= connection->send_next) {
        memset(connection, 0, sizeof(*connection));
    }
}

void net_transport_service(void) {
    net_ipv4_packet_t packet;

    while (net_poll_ipv4_packet(&packet)) {
        if (packet.protocol == NET_IPV4_PROTOCOL_UDP) {
            transport_handle_udp_packet(&packet);
        } else if (packet.protocol == NET_IPV4_PROTOCOL_TCP) {
            transport_handle_tcp_packet(&packet);
        }
    }
}

bool net_udp_send(u16 source_port,
                  const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                  u16 destination_port,
                  const void* payload,
                  u16 payload_length) {
    net_status_t status;

    net_get_status(&status);
    if (!status.ipv4_configured) {
        return false;
    }

    return transport_send_udp_datagram(status.ipv4_address,
                                       source_port,
                                       destination_ipv4,
                                       destination_port,
                                       payload,
                                       payload_length);
}

bool net_udp_poll(u16 local_port, net_udp_packet_t* out_packet) {
    u32 examined = 0u;
    u32 index = g_transport.udp_head;

    if (out_packet == NULL) {
        return false;
    }

    while (examined < g_transport.udp_count) {
        udp_queue_entry_t* entry = &g_transport.udp_queue[index];
        if (entry->valid && entry->packet.destination_port == local_port) {
            *out_packet = entry->packet;
            entry->valid = false;
            if (index == g_transport.udp_head) {
                do {
                    g_transport.udp_head = (g_transport.udp_head + 1u) % NET_UDP_QUEUE_SIZE;
                } while (g_transport.udp_count > 1u &&
                         !g_transport.udp_queue[g_transport.udp_head].valid &&
                         g_transport.udp_head != g_transport.udp_tail);
            }
            --g_transport.udp_count;
            return true;
        }
        index = (index + 1u) % NET_UDP_QUEUE_SIZE;
        ++examined;
    }

    return false;
}

bool net_dhcp_negotiate(void) {
    u32 transaction_id;
    u8 offered_ipv4[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    u8 subnet_mask[NET_IPV4_ADDRESS_LENGTH] = {255u, 255u, 255u, 0u};
    u8 gateway[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    u8 dns_server[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    u8 dhcp_server[NET_IPV4_ADDRESS_LENGTH] = {0u, 0u, 0u, 0u};
    u32 lease_seconds = 0u;

    g_transport.dhcp_status.attempted = true;
    transaction_id = 0x4A420000u | (rtc_read_seconds_of_day() & 0xFFFFu);

    if (!transport_send_dhcp_message(NET_DHCP_MESSAGE_DISCOVER, transaction_id, NULL, NULL)) {
        serial_write("[net] dhcp discover send failed\n");
        return false;
    }
    if (!transport_wait_for_dhcp_message(transaction_id,
                                         NET_DHCP_MESSAGE_OFFER,
                                         offered_ipv4,
                                         subnet_mask,
                                         gateway,
                                         dns_server,
                                         dhcp_server,
                                         &lease_seconds)) {
        serial_write("[net] dhcp offer timed out\n");
        return false;
    }
    if (!transport_send_dhcp_message(NET_DHCP_MESSAGE_REQUEST,
                                     transaction_id,
                                     offered_ipv4,
                                     dhcp_server)) {
        serial_write("[net] dhcp request send failed\n");
        return false;
    }
    if (!transport_wait_for_dhcp_message(transaction_id,
                                         NET_DHCP_MESSAGE_ACK,
                                         offered_ipv4,
                                         subnet_mask,
                                         gateway,
                                         dns_server,
                                         dhcp_server,
                                         &lease_seconds)) {
        serial_write("[net] dhcp ack timed out\n");
        return false;
    }

    if (!net_configure_ipv4_static(offered_ipv4, subnet_mask, gateway)) {
        serial_write("[net] dhcp could not apply ipv4 settings\n");
        return false;
    }

    g_transport.dhcp_status.configured = true;
    memcpy(g_transport.dhcp_status.ipv4_address, offered_ipv4, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_transport.dhcp_status.subnet_mask, subnet_mask, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_transport.dhcp_status.gateway, gateway, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_transport.dhcp_status.dns_server, dns_server, NET_IPV4_ADDRESS_LENGTH);
    memcpy(g_transport.dhcp_status.dhcp_server, dhcp_server, NET_IPV4_ADDRESS_LENGTH);
    g_transport.dhcp_status.lease_time_seconds = lease_seconds;
    memcpy(g_transport.dns_server, dns_server, NET_IPV4_ADDRESS_LENGTH);

    serial_write("[net] dhcp lease acquired\n");
    return true;
}

void net_dhcp_get_status(net_dhcp_status_t* out_status) {
    if (out_status == NULL) {
        return;
    }
    *out_status = g_transport.dhcp_status;
}

void net_dns_set_server(const u8 dns_server[NET_IPV4_ADDRESS_LENGTH]) {
    if (dns_server == NULL) {
        return;
    }
    memcpy(g_transport.dns_server, dns_server, NET_IPV4_ADDRESS_LENGTH);
}

static bool transport_dns_skip_name(const u8* message, u16 message_length, u16* in_out_offset) {
    u16 offset = *in_out_offset;

    while (offset < message_length) {
        u8 label_length = message[offset++];
        if (label_length == 0u) {
            *in_out_offset = offset;
            return true;
        }
        if ((label_length & 0xC0u) == 0xC0u) {
            if (offset >= message_length) {
                return false;
            }
            *in_out_offset = (u16)(offset + 1u);
            return true;
        }
        if (label_length > message_length - offset) {
            return false;
        }
        offset = (u16)(offset + label_length);
    }

    return false;
}

bool net_dns_resolve_a(const char* host, u8 out_ipv4[NET_IPV4_ADDRESS_LENGTH]) {
    u8 query[512];
    dns_header_t* header = (dns_header_t*)query;
    u16 offset = sizeof(*header);
    u16 local_port = transport_allocate_ephemeral_port();
    u16 transaction_id = (u16)(rtc_read_seconds_of_day() ^ local_port);
    u32 start_second;

    if (host == NULL || out_ipv4 == NULL || transport_ipv4_is_zero(g_transport.dns_server)) {
        return false;
    }

    memset(query, 0, sizeof(query));
    header->transaction_id = transport_to_be16(transaction_id);
    header->flags = transport_to_be16(0x0100u);
    header->question_count = transport_to_be16(1u);

    {
        u32 label_start = 0u;
        u32 index = 0u;
        for (;;) {
            if (host[index] == '.' || host[index] == '\0') {
                u32 label_length = index - label_start;
                if (label_length == 0u || label_length > 63u || offset + 1u + label_length + 5u > sizeof(query)) {
                    return false;
                }
                query[offset++] = (u8)label_length;
                memcpy(query + offset, host + label_start, label_length);
                offset = (u16)(offset + label_length);
                label_start = index + 1u;
                if (host[index] == '\0') {
                    break;
                }
            }
            ++index;
        }
    }

    query[offset++] = 0u;
    query[offset++] = 0u;
    query[offset++] = NET_DNS_TYPE_A;
    query[offset++] = 0u;
    query[offset++] = NET_DNS_CLASS_IN;

    if (!net_udp_send(local_port, g_transport.dns_server, NET_DNS_SERVER_PORT, query, offset)) {
        return false;
    }

    start_second = rtc_read_seconds_of_day();
    for (;;) {
        net_udp_packet_t packet;

        transport_wait_iteration();
        while (net_udp_poll(local_port, &packet)) {
            u16 question_count;
            u16 answer_count;
            u16 parse_offset;
            u16 answer_index;

            if (packet.source_port != NET_DNS_SERVER_PORT || packet.payload_length < sizeof(dns_header_t)) {
                continue;
            }

            header = (dns_header_t*)packet.payload;
            if (transport_from_be16(header->transaction_id) != transaction_id) {
                continue;
            }

            question_count = transport_from_be16(header->question_count);
            answer_count = transport_from_be16(header->answer_count);
            parse_offset = sizeof(*header);

            while (question_count-- > 0u) {
                if (!transport_dns_skip_name(packet.payload, packet.payload_length, &parse_offset) ||
                    parse_offset + 4u > packet.payload_length) {
                    break;
                }
                parse_offset = (u16)(parse_offset + 4u);
            }
            if (question_count != 0xFFFFu) {
                continue;
            }

            for (answer_index = 0u; answer_index < answer_count; ++answer_index) {
                u16 type;
                u16 class_code;
                u16 rdlength;

                if (!transport_dns_skip_name(packet.payload, packet.payload_length, &parse_offset) ||
                    parse_offset + 10u > packet.payload_length) {
                    break;
                }

                type = ((u16)packet.payload[parse_offset] << 8) | packet.payload[parse_offset + 1u];
                class_code = ((u16)packet.payload[parse_offset + 2u] << 8) | packet.payload[parse_offset + 3u];
                rdlength = ((u16)packet.payload[parse_offset + 8u] << 8) | packet.payload[parse_offset + 9u];
                parse_offset = (u16)(parse_offset + 10u);
                if (parse_offset + rdlength > packet.payload_length) {
                    break;
                }

                if (type == NET_DNS_TYPE_A && class_code == NET_DNS_CLASS_IN && rdlength == NET_IPV4_ADDRESS_LENGTH) {
                    memcpy(out_ipv4, packet.payload + parse_offset, NET_IPV4_ADDRESS_LENGTH);
                    return true;
                }
                parse_offset = (u16)(parse_offset + rdlength);
            }
        }

        if (transport_elapsed_seconds(start_second, rtc_read_seconds_of_day()) >= NET_DNS_TIMEOUT_SECONDS) {
            return false;
        }
    }
}

int net_tcp_connect(const u8 remote_ipv4[NET_IPV4_ADDRESS_LENGTH], u16 remote_port) {
    u32 index;

    for (index = 0u; index < NET_TCP_CONNECTION_MAX; ++index) {
        tcp_connection_t* connection = &g_transport.tcp_connections[index];
        u32 retry;

        if (connection->used) {
            continue;
        }

        memset(connection, 0, sizeof(*connection));
        connection->used = true;
        connection->state = TCP_STATE_SYN_SENT;
        memcpy(connection->remote_ipv4, remote_ipv4, NET_IPV4_ADDRESS_LENGTH);
        connection->local_port = transport_allocate_ephemeral_port();
        connection->remote_port = remote_port;
        connection->initial_sequence = 0x10000000u | ((u32)rtc_read_seconds_of_day() << 8);
        connection->send_unacknowledged = connection->initial_sequence;
        connection->send_next = connection->initial_sequence;
        transport_tcp_refresh_receive_window(connection);

        for (retry = 0u; retry < NET_TCP_MAX_RETRIES; ++retry) {
            connection->send_unacknowledged = connection->initial_sequence;
            connection->send_next = connection->initial_sequence;
            if (!transport_tcp_send_segment(connection, NET_TCP_FLAG_SYN, NULL, 0u)) {
                continue;
            }
            if (connection->state == TCP_STATE_ESTABLISHED) {
                return (int)index;
            }
            if (transport_tcp_wait_for_ack(connection, connection->initial_sequence + 1u) &&
                connection->state == TCP_STATE_ESTABLISHED) {
                return (int)index;
            }
        }

        memset(connection, 0, sizeof(*connection));
        return -1;
    }

    return -1;
}

bool net_tcp_is_connected(int connection_id) {
    tcp_connection_t* connection = transport_get_connection(connection_id);

    if (connection == NULL || !connection->used) {
        return false;
    }
    return connection->state == TCP_STATE_ESTABLISHED || connection->state == TCP_STATE_CLOSE_WAIT;
}

bool net_tcp_send(int connection_id, const void* data, u16 length) {
    tcp_connection_t* connection = transport_get_connection(connection_id);
    const u8* cursor = (const u8*)data;
    u16 remaining = length;

    if (connection == NULL || !connection->used || data == NULL) {
        return false;
    }
    if (connection->state != TCP_STATE_ESTABLISHED && connection->state != TCP_STATE_CLOSE_WAIT) {
        return false;
    }

    while (remaining != 0u) {
        u16 chunk = remaining > NET_TCP_PAYLOAD_MAX ? NET_TCP_PAYLOAD_MAX : remaining;
        u32 target_ack = connection->send_next + chunk;

        if (!transport_tcp_send_segment(connection, (u8)(NET_TCP_FLAG_ACK | NET_TCP_FLAG_PSH), cursor, chunk)) {
            return false;
        }
        if (!transport_tcp_wait_for_ack(connection, target_ack)) {
            return false;
        }

        cursor += chunk;
        remaining = (u16)(remaining - chunk);
    }

    return true;
}

bool net_tcp_wait_for_data(int connection_id, u32 timeout_seconds) {
    tcp_connection_t* connection = transport_get_connection(connection_id);
    u32 start_second = rtc_read_seconds_of_day();

    if (connection == NULL || !connection->used) {
        return false;
    }

    while (connection->used) {
        if (connection->receive_length != 0u || connection->state == TCP_STATE_CLOSE_WAIT) {
            return true;
        }
        transport_wait_iteration();
        if (transport_elapsed_seconds(start_second, rtc_read_seconds_of_day()) >= timeout_seconds) {
            return false;
        }
    }

    return false;
}

u16 net_tcp_receive(int connection_id, void* buffer, u16 buffer_capacity) {
    tcp_connection_t* connection = transport_get_connection(connection_id);
    u16 copy_length;

    if (connection == NULL || !connection->used || buffer == NULL || buffer_capacity == 0u) {
        return 0u;
    }

    copy_length = connection->receive_length;
    if (copy_length > buffer_capacity) {
        copy_length = buffer_capacity;
    }
    memcpy(buffer, connection->receive_buffer, copy_length);
    memmove(connection->receive_buffer,
            connection->receive_buffer + copy_length,
            connection->receive_length - copy_length);
    connection->receive_length = (u16)(connection->receive_length - copy_length);
    transport_tcp_refresh_receive_window(connection);
    return copy_length;
}

void net_tcp_close(int connection_id) {
    tcp_connection_t* connection = transport_get_connection(connection_id);

    if (connection == NULL || !connection->used) {
        return;
    }

    if (connection->state == TCP_STATE_ESTABLISHED) {
        if (transport_tcp_send_segment(connection, (u8)(NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK), NULL, 0u)) {
            connection->state = TCP_STATE_FIN_WAIT_1;
            connection->fin_sent = true;
            if (transport_tcp_wait_for_ack(connection, connection->send_next)) {
                return;
            }
        }
    } else if (connection->state == TCP_STATE_CLOSE_WAIT) {
        if (transport_tcp_send_segment(connection, (u8)(NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK), NULL, 0u)) {
            connection->state = TCP_STATE_LAST_ACK;
            connection->fin_sent = true;
            if (transport_tcp_wait_for_ack(connection, connection->send_next)) {
                return;
            }
        }
    }

    memset(connection, 0, sizeof(*connection));
}

static char transport_ascii_lower(char value) {
    if (value >= 'A' && value <= 'Z') {
        return (char)(value - 'A' + 'a');
    }
    return value;
}

static void transport_copy_string(char* destination, u32 capacity, const char* source) {
    u32 index = 0u;

    if (destination == NULL || capacity == 0u) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    while (index + 1u < capacity && source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static void transport_copy_string_n(char* destination, u32 capacity, const char* source, u32 length) {
    u32 copy_length = length;

    if (destination == NULL || capacity == 0u) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    if (copy_length + 1u > capacity) {
        copy_length = capacity - 1u;
    }
    memcpy(destination, source, copy_length);
    destination[copy_length] = '\0';
}

static bool transport_string_starts_with_ci(const char* text, const char* prefix) {
    u32 index = 0u;

    if (text == NULL || prefix == NULL) {
        return false;
    }

    while (prefix[index] != '\0') {
        if (text[index] == '\0' || transport_ascii_lower(text[index]) != transport_ascii_lower(prefix[index])) {
            return false;
        }
        ++index;
    }
    return true;
}

static bool transport_string_equals_ci_n(const char* left, const char* right, u32 length) {
    for (u32 index = 0u; index < length; ++index) {
        if (transport_ascii_lower(left[index]) != transport_ascii_lower(right[index])) {
            return false;
        }
    }
    return true;
}

static bool transport_parse_decimal_u32(const char* text, u32* out_value) {
    u32 value = 0u;
    u32 index = 0u;

    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    while (text[index] != '\0') {
        char value_char = text[index];
        if (value_char < '0' || value_char > '9') {
            return false;
        }
        value = value * 10u + (u32)(value_char - '0');
        ++index;
    }

    *out_value = value;
    return true;
}

static bool transport_parse_ipv4_literal(const char* text, u8 out_ipv4[NET_IPV4_ADDRESS_LENGTH]) {
    u32 part_index = 0u;
    u32 current_value = 0u;
    bool have_digit = false;

    if (text == NULL || out_ipv4 == NULL) {
        return false;
    }

    while (*text != '\0') {
        if (*text >= '0' && *text <= '9') {
            current_value = current_value * 10u + (u32)(*text - '0');
            if (current_value > 255u) {
                return false;
            }
            have_digit = true;
        } else if (*text == '.') {
            if (!have_digit || part_index >= NET_IPV4_ADDRESS_LENGTH - 1u) {
                return false;
            }
            out_ipv4[part_index++] = (u8)current_value;
            current_value = 0u;
            have_digit = false;
        } else {
            return false;
        }
        ++text;
    }

    if (!have_digit || part_index != NET_IPV4_ADDRESS_LENGTH - 1u) {
        return false;
    }

    out_ipv4[part_index] = (u8)current_value;
    return true;
}

static bool transport_append_string(char* buffer, u32 capacity, const char* text) {
    u32 length;
    u32 index = 0u;

    if (buffer == NULL || capacity == 0u || text == NULL) {
        return false;
    }

    length = (u32)strlen(buffer);
    while (text[index] != '\0') {
        if (length + index + 1u >= capacity) {
            return false;
        }
        buffer[length + index] = text[index];
        ++index;
    }
    buffer[length + index] = '\0';
    return true;
}

static bool transport_append_uint(char* buffer, u32 capacity, u32 value) {
    char digits[11];
    u32 digit_count = 0u;

    if (buffer == NULL || capacity == 0u) {
        return false;
    }

    if (value == 0u) {
        return transport_append_string(buffer, capacity, "0");
    }

    while (value != 0u) {
        digits[digit_count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (digit_count != 0u) {
        char single_char[2];
        --digit_count;
        single_char[0] = digits[digit_count];
        single_char[1] = '\0';
        if (!transport_append_string(buffer, capacity, single_char)) {
            return false;
        }
    }

    return true;
}

static void transport_http_init_result(net_http_result_t* out_result, const char* url) {
    if (out_result == NULL) {
        return;
    }

    memset(out_result, 0, sizeof(*out_result));
    transport_copy_string(out_result->final_url, sizeof(out_result->final_url), url);
    transport_copy_string(out_result->message, sizeof(out_result->message), "HTTP request not started");
}

static void transport_http_set_message(net_http_result_t* out_result, const char* message) {
    if (out_result == NULL) {
        return;
    }
    transport_copy_string(out_result->message, sizeof(out_result->message), message);
}

static bool transport_http_parse_url(const char* url,
                                     char* out_host,
                                     u32 host_capacity,
                                     char* out_path,
                                     u32 path_capacity,
                                     u16* out_port,
                                     bool* out_https) {
    const char* cursor = url;
    const char* path_start;
    u32 authority_length = 0u;
    u32 host_length = 0u;
    u32 port_index = 0u;
    bool https = false;
    u16 port = 80u;

    if (url == NULL || out_host == NULL || out_path == NULL || out_port == NULL || out_https == NULL) {
        return false;
    }

    if (transport_string_starts_with_ci(url, "http://")) {
        cursor += 7;
    } else if (transport_string_starts_with_ci(url, "https://")) {
        cursor += 8;
        https = true;
        port = 443u;
    }

    path_start = cursor;
    while (path_start[authority_length] != '\0' && path_start[authority_length] != '/') {
        ++authority_length;
    }
    if (authority_length == 0u) {
        return false;
    }

    while (host_length < authority_length && cursor[host_length] != ':') {
        ++host_length;
    }
    if (host_length == 0u || host_length >= host_capacity) {
        return false;
    }

    transport_copy_string_n(out_host, host_capacity, cursor, host_length);
    if (host_length < authority_length) {
        char port_text[8];
        u32 port_length = authority_length - host_length - 1u;
        u32 parsed_port = 0u;

        if (port_length == 0u || port_length >= sizeof(port_text)) {
            return false;
        }

        for (port_index = 0u; port_index < port_length; ++port_index) {
            port_text[port_index] = cursor[host_length + 1u + port_index];
        }
        port_text[port_length] = '\0';
        if (!transport_parse_decimal_u32(port_text, &parsed_port) || parsed_port == 0u || parsed_port > 65535u) {
            return false;
        }
        port = (u16)parsed_port;
    }

    if (cursor[authority_length] == '\0') {
        transport_copy_string(out_path, path_capacity, "/");
    } else {
        transport_copy_string(out_path, path_capacity, cursor + authority_length);
        if (out_path[0] == '\0') {
            transport_copy_string(out_path, path_capacity, "/");
        }
    }

    *out_port = port;
    *out_https = https;
    return true;
}

static bool transport_http_build_request(char* request,
                                         u32 capacity,
                                         const char* host,
                                         u16 port,
                                         const char* path) {
    if (request == NULL || capacity == 0u || host == NULL || path == NULL) {
        return false;
    }

    request[0] = '\0';
    if (!transport_append_string(request, capacity, "GET ") ||
        !transport_append_string(request, capacity, path) ||
        !transport_append_string(request, capacity, " HTTP/1.0\r\nHost: ") ||
        !transport_append_string(request, capacity, host)) {
        return false;
    }

    if (port != 80u) {
        if (!transport_append_string(request, capacity, ":") ||
            !transport_append_uint(request, capacity, (u32)port)) {
            return false;
        }
    }

    return transport_append_string(request, capacity,
                                   "\r\nUser-Agent: JabulOSAero/0.1\r\n"
                                   "Connection: close\r\n"
                                   "Accept: text/plain, text/html, */*\r\n\r\n");
}

static u16 transport_http_parse_status_code(const char* headers) {
    u32 index = 0u;
    u16 status_code = 0u;

    if (headers == NULL) {
        return 0u;
    }

    while (headers[index] != '\0' && headers[index] != ' ') {
        ++index;
    }
    if (headers[index] != ' ') {
        return 0u;
    }

    ++index;
    while (headers[index] == ' ') {
        ++index;
    }

    while (headers[index] >= '0' && headers[index] <= '9') {
        status_code = (u16)(status_code * 10u + (u16)(headers[index] - '0'));
        ++index;
    }
    return status_code;
}

static bool transport_http_get_header_value(const char* headers,
                                            const char* header_name,
                                            char* out_value,
                                            u32 value_capacity) {
    u32 name_length;
    const char* cursor;

    if (headers == NULL || header_name == NULL || out_value == NULL || value_capacity == 0u) {
        return false;
    }

    out_value[0] = '\0';
    name_length = (u32)strlen(header_name);
    cursor = headers;
    while (cursor[0] != '\0' && !(cursor[0] == '\r' && cursor[1] == '\n')) {
        ++cursor;
    }
    if (cursor[0] == '\r' && cursor[1] == '\n') {
        cursor += 2;
    }

    while (cursor[0] != '\0') {
        u32 line_length = 0u;
        u32 value_length = 0u;
        const char* value_start;

        if (cursor[0] == '\r' && cursor[1] == '\n') {
            return false;
        }

        while (cursor[line_length] != '\0' &&
               !(cursor[line_length] == '\r' && cursor[line_length + 1u] == '\n')) {
            ++line_length;
        }
        if (line_length <= name_length || cursor[name_length] != ':') {
            cursor += line_length;
            if (cursor[0] == '\r' && cursor[1] == '\n') {
                cursor += 2;
            }
            continue;
        }

        if (!transport_string_equals_ci_n(cursor, header_name, name_length)) {
            cursor += line_length;
            if (cursor[0] == '\r' && cursor[1] == '\n') {
                cursor += 2;
            }
            continue;
        }

        value_start = cursor + name_length + 1u;
        while (*value_start == ' ' || *value_start == '\t') {
            ++value_start;
        }
        while (value_start[value_length] != '\0' &&
               !(value_start[value_length] == '\r' && value_start[value_length + 1u] == '\n')) {
            ++value_length;
        }
        while (value_length > 0u &&
               (value_start[value_length - 1u] == ' ' || value_start[value_length - 1u] == '\t')) {
            --value_length;
        }
        transport_copy_string_n(out_value, value_capacity, value_start, value_length);
        return true;
    }

    return false;
}

static bool transport_http_parse_content_length(const char* headers, u32* out_length) {
    char value[16];

    if (!transport_http_get_header_value(headers, "Content-Length", value, sizeof(value))) {
        return false;
    }

    return transport_parse_decimal_u32(value, out_length);
}

static bool transport_http_build_redirect_url(const char* location,
                                              const char* host,
                                              u16 port,
                                              bool https,
                                              char* out_url,
                                              u32 url_capacity) {
    if (location == NULL || host == NULL || out_url == NULL || url_capacity == 0u) {
        return false;
    }

    if (transport_string_starts_with_ci(location, "http://") ||
        transport_string_starts_with_ci(location, "https://")) {
        transport_copy_string(out_url, url_capacity, location);
        return true;
    }

    out_url[0] = '\0';
    if (!transport_append_string(out_url, url_capacity, https ? "https://" : "http://") ||
        !transport_append_string(out_url, url_capacity, host)) {
        return false;
    }

    if ((https && port != 443u) || (!https && port != 80u)) {
        if (!transport_append_string(out_url, url_capacity, ":") ||
            !transport_append_uint(out_url, url_capacity, (u32)port)) {
            return false;
        }
    }

    if (location[0] != '/') {
        if (!transport_append_string(out_url, url_capacity, "/")) {
            return false;
        }
    }

    return transport_append_string(out_url, url_capacity, location);
}

bool net_http_get(const char* url, char* body, u32 body_capacity, net_http_result_t* out_result) {
    char current_url[NET_HTTP_URL_MAX];
    char host[80];
    char path[NET_HTTP_URL_MAX];
    char request[NET_HTTP_REQUEST_BUFFER_SIZE];
    char headers[NET_HTTP_HEADER_BUFFER_SIZE];
    char redirect_location[NET_HTTP_URL_MAX];
    u8 chunk[NET_HTTP_CHUNK_BUFFER_SIZE];
    u8 remote_ipv4[NET_IPV4_ADDRESS_LENGTH];

    if (url == NULL || body == NULL || body_capacity == 0u || out_result == NULL) {
        return false;
    }

    body[0] = '\0';
    transport_http_init_result(out_result, url);
    transport_copy_string(current_url, sizeof(current_url), url);

    for (u32 redirect_count = 0u; redirect_count <= NET_HTTP_MAX_REDIRECTS; ++redirect_count) {
        bool https = false;
        bool header_complete = false;
        bool have_content_length = false;
        u16 port = 80u;
        u32 header_length = 0u;
        u32 body_length = 0u;
        u32 expected_body_length = 0u;
        int connection_id = -1;

        body[0] = '\0';
        headers[0] = '\0';
        redirect_location[0] = '\0';
        memset(remote_ipv4, 0, sizeof(remote_ipv4));
        out_result->success = false;
        out_result->truncated = false;
        out_result->status_code = 0u;
        out_result->body_length = 0u;
        out_result->content_type[0] = '\0';
        transport_copy_string(out_result->final_url, sizeof(out_result->final_url), current_url);

        if (!transport_http_parse_url(current_url, host, sizeof(host), path, sizeof(path), &port, &https)) {
            transport_http_set_message(out_result, "Invalid URL");
            return false;
        }
        
        if (!transport_http_build_request(request, sizeof(request), host, port, path)) {
            transport_http_set_message(out_result, "HTTP request is too large");
            return false;
        }
        if (!transport_parse_ipv4_literal(host, remote_ipv4) &&
            !net_dns_resolve_a(host, remote_ipv4)) {
            transport_http_set_message(out_result, "DNS lookup failed");
            return false;
        }

        connection_id = net_tcp_connect(remote_ipv4, port);
        if (connection_id < 0) {
            transport_http_set_message(out_result, "TCP connect failed");
            return false;
        }
        if (!net_tcp_send(connection_id, request, (u16)strlen(request))) {
            net_tcp_close(connection_id);
            transport_http_set_message(out_result, "HTTP request send failed");
            return false;
        }

        for (;;) {
            u16 received_length;

            if (!net_tcp_wait_for_data(connection_id, NET_TCP_TIMEOUT_SECONDS)) {
                if (header_complete) {
                    break;
                }
                net_tcp_close(connection_id);
                transport_http_set_message(out_result, "HTTP response timed out");
                return false;
            }

            received_length = net_tcp_receive(connection_id, chunk, sizeof(chunk));
            if (received_length == 0u) {
                break;
            }

            for (u32 index = 0u; index < received_length; ++index) {
                char byte = (char)chunk[index];

                if (!header_complete) {
                    if (header_length + 1u >= sizeof(headers)) {
                        net_tcp_close(connection_id);
                        transport_http_set_message(out_result, "HTTP headers are too large");
                        return false;
                    }

                    headers[header_length++] = byte;
                    headers[header_length] = '\0';
                    if (header_length >= 4u &&
                        headers[header_length - 4u] == '\r' &&
                        headers[header_length - 3u] == '\n' &&
                        headers[header_length - 2u] == '\r' &&
                        headers[header_length - 1u] == '\n') {
                        header_complete = true;
                        out_result->status_code = transport_http_parse_status_code(headers);
                        (void)transport_http_get_header_value(headers,
                                                              "Content-Type",
                                                              out_result->content_type,
                                                              sizeof(out_result->content_type));
                        have_content_length = transport_http_parse_content_length(headers, &expected_body_length);
                        (void)transport_http_get_header_value(headers,
                                                              "Location",
                                                              redirect_location,
                                                              sizeof(redirect_location));
                    }
                    continue;
                }

                if (body_length + 1u < body_capacity) {
                    body[body_length++] = byte;
                    body[body_length] = '\0';
                } else {
                    out_result->truncated = true;
                }
            }

            if (header_complete && have_content_length && body_length >= expected_body_length) {
                break;
            }
        }

        net_tcp_close(connection_id);
        out_result->body_length = body_length;

        if (!header_complete) {
            transport_http_set_message(out_result, "HTTP response was incomplete");
            return false;
        }

        if (out_result->status_code >= 300u &&
            out_result->status_code < 400u &&
            redirect_location[0] != '\0') {
            char next_url[NET_HTTP_URL_MAX];

            if (redirect_count == NET_HTTP_MAX_REDIRECTS) {
                transport_http_set_message(out_result, "Too many HTTP redirects");
                return false;
            }
            if (!transport_http_build_redirect_url(redirect_location, host, port, https, next_url, sizeof(next_url))) {
                transport_http_set_message(out_result, "HTTP redirect is invalid");
                return false;
            }

            out_result->redirected = true;
            transport_copy_string(current_url, sizeof(current_url), next_url);
            continue;
        }

        out_result->success = out_result->status_code >= 200u && out_result->status_code < 300u;
        transport_copy_string(out_result->final_url, sizeof(out_result->final_url), current_url);
        transport_http_set_message(out_result,
                                   out_result->success ? "HTTP request completed"
                                                       : "HTTP server returned a non-success status");
        return true;
    }

    transport_http_set_message(out_result, "HTTP request failed");
    return false;
}
