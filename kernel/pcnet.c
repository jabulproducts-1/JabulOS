#include "jabulos.h"

#define PCNET_VENDOR_ID 0x1022u
#define PCNET_DEVICE_ID_PCI_II 0x2000u
#define PCNET_DEVICE_ID_PCI_III 0x2001u

#define PCNET_PCI_COMMAND_IO_SPACE 0x0001u
#define PCNET_PCI_COMMAND_BUS_MASTER 0x0004u

#define PCNET_PORT_RDP 0x10u
#define PCNET_PORT_RAP 0x12u
#define PCNET_PORT_RESET 0x14u
#define PCNET_PORT_BDP 0x16u

#define PCNET_CSR0 0u
#define PCNET_CSR1 1u
#define PCNET_CSR2 2u
#define PCNET_CSR3 3u
#define PCNET_CSR4 4u
#define PCNET_CSR58 58u

#define PCNET_BCR2 2u

#define PCNET_CSR0_INIT 0x0001u
#define PCNET_CSR0_START 0x0002u
#define PCNET_CSR0_STOP 0x0004u
#define PCNET_CSR0_TX_POLL 0x0008u
#define PCNET_CSR0_IDON 0x0100u
#define PCNET_CSR0_TX_DONE 0x0200u
#define PCNET_CSR0_RX_DONE 0x0400u
#define PCNET_CSR0_MISS 0x1000u
#define PCNET_CSR0_CERR 0x2000u
#define PCNET_CSR0_BABL 0x4000u
#define PCNET_CSR0_ERROR 0x8000u

#define PCNET_CSR3_MASK_INIT_DONE 0x0100u
#define PCNET_CSR3_MASK_TX_DONE 0x0200u
#define PCNET_CSR3_MASK_RX_DONE 0x0400u
#define PCNET_CSR4_AUTO_PAD_TX 0x0800u

#define PCNET_BCR2_ASEL 0x0002u
#define PCNET_SWSTYLE_32BIT 0x0002u

#define PCNET_RX_RING_COUNT 1u
#define PCNET_TX_RING_COUNT 1u
#define PCNET_BUFFER_SIZE 1520u
#define PCNET_RESET_WAIT_CYCLES 1024u
#define PCNET_INIT_POLL_LIMIT 65536u
#define PCNET_TX_POLL_LIMIT 65536u

#define PCNET_DESCRIPTOR_OWN 0x80u
#define PCNET_DESCRIPTOR_ERROR 0x40u
#define PCNET_DESCRIPTOR_START 0x02u
#define PCNET_DESCRIPTOR_END 0x01u

#define PCNET_CSR0_ACK_MASK (PCNET_CSR0_IDON | PCNET_CSR0_TX_DONE | PCNET_CSR0_RX_DONE | \
                             PCNET_CSR0_MISS | PCNET_CSR0_CERR | PCNET_CSR0_BABL | PCNET_CSR0_ERROR)

typedef struct __attribute__((packed)) {
    u32 buffer_address;
    u16 buffer_length;
    u8 reserved;
    u8 status;
    u32 misc;
    u32 reserved2;
} pcnet_descriptor_t;

typedef struct __attribute__((packed)) {
    u16 mode;
    u8 rlen;
    u8 tlen;
    u8 mac[6];
    u16 reserved;
    u8 logical_address[8];
    u32 rx_ring_address;
    u32 tx_ring_address;
} pcnet_init_block_t;

typedef struct __attribute__((packed, aligned(16))) {
    pcnet_init_block_t init_block;
    u8 init_padding[4];
    pcnet_descriptor_t rx_ring[PCNET_RX_RING_COUNT];
    pcnet_descriptor_t tx_ring[PCNET_TX_RING_COUNT];
    u8 rx_buffer[PCNET_RX_RING_COUNT][PCNET_BUFFER_SIZE];
    u8 tx_buffer[PCNET_TX_RING_COUNT][PCNET_BUFFER_SIZE];
} pcnet_dma_layout_t;

typedef struct {
    bool probed;
    pci_device_info_t pci_device;
    pcnet_status_t status;
    pcnet_dma_layout_t* dma;
    u64 dma_frame;
    u32 rx_read_index;
    u32 tx_write_index;
} pcnet_driver_t;

static pcnet_driver_t g_pcnet;

static void pcnet_write_hex_byte(u8 value) {
    static const char k_hex[] = "0123456789ABCDEF";
    char text[3];

    text[0] = k_hex[(value >> 4) & 0x0Fu];
    text[1] = k_hex[value & 0x0Fu];
    text[2] = '\0';
    serial_write(text);
}

static void pcnet_write_mac(const u8 mac[6]) {
    if (mac == NULL) {
        return;
    }

    for (u32 index = 0u; index < 6u; ++index) {
        pcnet_write_hex_byte(mac[index]);
        if (index + 1u < 6u) {
            serial_write(":");
        }
    }
}

static u16 pcnet_encode_buffer_length(u16 buffer_size) {
    u16 encoded_length = (u16)(0u - buffer_size);
    encoded_length &= 0x0FFFu;
    encoded_length |= 0xF000u;
    return encoded_length;
}

static u16 pcnet_port(pcnet_driver_t* driver, u16 offset) {
    return (u16)(driver->status.io_base + offset);
}

static u16 pcnet_read_csr(pcnet_driver_t* driver, u16 index) {
    outw(pcnet_port(driver, PCNET_PORT_RAP), index);
    return inw(pcnet_port(driver, PCNET_PORT_RDP));
}

static void pcnet_write_csr(pcnet_driver_t* driver, u16 index, u16 value) {
    outw(pcnet_port(driver, PCNET_PORT_RAP), index);
    outw(pcnet_port(driver, PCNET_PORT_RDP), value);
}

static u16 pcnet_read_bcr(pcnet_driver_t* driver, u16 index) {
    outw(pcnet_port(driver, PCNET_PORT_RAP), index);
    return inw(pcnet_port(driver, PCNET_PORT_BDP));
}

static void pcnet_write_bcr(pcnet_driver_t* driver, u16 index, u16 value) {
    outw(pcnet_port(driver, PCNET_PORT_RAP), index);
    outw(pcnet_port(driver, PCNET_PORT_BDP), value);
}

static void pcnet_wait_reset_complete(void) {
    for (u32 wait = 0u; wait < PCNET_RESET_WAIT_CYCLES; ++wait) {
        io_wait();
    }
}

static void pcnet_reset(pcnet_driver_t* driver) {
    (void)inl((u16)(driver->status.io_base + 0x18u));
    (void)inw(pcnet_port(driver, PCNET_PORT_RESET));
    pcnet_wait_reset_complete();
}

static void pcnet_read_mac(pcnet_driver_t* driver) {
    for (u32 index = 0u; index < 6u; ++index) {
        driver->status.mac[index] = inb((u16)(driver->status.io_base + index));
    }
}

static bool pcnet_prepare_dma(pcnet_driver_t* driver) {
    if (driver->dma != NULL) {
        return true;
    }

    driver->dma_frame = pmm_alloc_frame();
    if (driver->dma_frame == 0u || driver->dma_frame > 0xFFFFFFFFu) {
        driver->dma = NULL;
        driver->dma_frame = 0u;
        return false;
    }

    driver->dma = (pcnet_dma_layout_t*)(u64)driver->dma_frame;
    memset(driver->dma, 0, PAGE_SIZE);
    return true;
}

static void pcnet_initialize_descriptor(pcnet_descriptor_t* descriptor,
                                        u32 buffer_address,
                                        u16 buffer_size,
                                        bool card_owns_buffer) {
    if (descriptor == NULL) {
        return;
    }

    memset(descriptor, 0, sizeof(*descriptor));
    descriptor->buffer_address = buffer_address;
    descriptor->buffer_length = pcnet_encode_buffer_length(buffer_size);
    descriptor->status = card_owns_buffer ? PCNET_DESCRIPTOR_OWN : 0x00u;
}

static void pcnet_acknowledge_events(pcnet_driver_t* driver) {
    u16 csr0;
    u16 clear_mask;

    if (driver == NULL || !driver->status.initialized) {
        return;
    }

    csr0 = pcnet_read_csr(driver, PCNET_CSR0);
    clear_mask = csr0 & PCNET_CSR0_ACK_MASK;
    if (clear_mask != 0u) {
        pcnet_write_csr(driver, PCNET_CSR0, clear_mask);
    }
}

static void pcnet_initialize_dma_layout(pcnet_driver_t* driver) {
    pcnet_dma_layout_t* dma = driver->dma;
    u32 index;

    if (dma == NULL) {
        return;
    }

    memset(dma, 0, sizeof(*dma));
    dma->init_block.mode = 0u;
    dma->init_block.rlen = 0u;
    dma->init_block.tlen = 0u;
    memcpy(dma->init_block.mac, driver->status.mac, sizeof(dma->init_block.mac));
    memset(dma->init_block.logical_address, 0, sizeof(dma->init_block.logical_address));

    for (index = 0u; index < PCNET_RX_RING_COUNT; ++index) {
        pcnet_initialize_descriptor(&dma->rx_ring[index],
                                    (u32)(u64)&dma->rx_buffer[index][0],
                                    (u16)PCNET_BUFFER_SIZE,
                                    true);
    }
    for (index = 0u; index < PCNET_TX_RING_COUNT; ++index) {
        pcnet_initialize_descriptor(&dma->tx_ring[index],
                                    (u32)(u64)&dma->tx_buffer[index][0],
                                    (u16)PCNET_BUFFER_SIZE,
                                    false);
    }

    dma->init_block.rx_ring_address = (u32)(u64)&dma->rx_ring[0];
    dma->init_block.tx_ring_address = (u32)(u64)&dma->tx_ring[0];
}

static bool pcnet_wait_for_init_done(pcnet_driver_t* driver) {
    for (u32 poll = 0u; poll < PCNET_INIT_POLL_LIMIT; ++poll) {
        if ((pcnet_read_csr(driver, PCNET_CSR0) & PCNET_CSR0_IDON) != 0u) {
            return true;
        }
        io_wait();
    }

    return false;
}

bool pcnet_initialize(void) {
    pci_device_info_t device;
    u16 pci_command;
    u16 csr4;
    u16 csr0;

    if (g_pcnet.probed) {
        return g_pcnet.status.initialized;
    }

    memset(&g_pcnet, 0, sizeof(g_pcnet));
    g_pcnet.probed = true;

    if (!pci_find_device(PCNET_VENDOR_ID, PCNET_DEVICE_ID_PCI_II, &device) &&
        !pci_find_device(PCNET_VENDOR_ID, PCNET_DEVICE_ID_PCI_III, &device)) {
        serial_write("[net] amd pcnet not detected\n");
        return false;
    }

    g_pcnet.pci_device = device;
    g_pcnet.status.present = true;
    g_pcnet.status.vendor_id = device.vendor_id;
    g_pcnet.status.device_id = device.device_id;
    g_pcnet.status.irq_line = device.irq_line;
    g_pcnet.status.irq_pin = device.irq_pin;

    if ((device.bar[0] & 0x1u) == 0u) {
        serial_write("[net] amd pcnet bar0 is not io space\n");
        return false;
    }

    g_pcnet.status.io_base = (u16)(device.bar[0] & ~0x3u);

    pci_command = pci_read_config_word(device.bus, device.device, device.function, 0x04u);
    pci_command |= (u16)(PCNET_PCI_COMMAND_IO_SPACE | PCNET_PCI_COMMAND_BUS_MASTER);
    pci_write_config_word(device.bus, device.device, device.function, 0x04u, pci_command);
    pci_command = pci_read_config_word(device.bus, device.device, device.function, 0x04u);
    g_pcnet.status.io_space_enabled = (pci_command & PCNET_PCI_COMMAND_IO_SPACE) != 0u;
    g_pcnet.status.bus_master_enabled = (pci_command & PCNET_PCI_COMMAND_BUS_MASTER) != 0u;
    if (!g_pcnet.status.io_space_enabled || !g_pcnet.status.bus_master_enabled) {
        serial_write("[net] amd pcnet pci command bits were not enabled\n");
        return false;
    }

    pcnet_reset(&g_pcnet);
    pcnet_write_csr(&g_pcnet, PCNET_CSR0, PCNET_CSR0_STOP);
    pcnet_write_csr(&g_pcnet, PCNET_CSR58, PCNET_SWSTYLE_32BIT);
    pcnet_write_bcr(&g_pcnet, PCNET_BCR2, (u16)(pcnet_read_bcr(&g_pcnet, PCNET_BCR2) | PCNET_BCR2_ASEL));
    pcnet_read_mac(&g_pcnet);

    if (!pcnet_prepare_dma(&g_pcnet)) {
        serial_write("[net] amd pcnet could not allocate dma memory\n");
        return false;
    }

    pcnet_initialize_dma_layout(&g_pcnet);
    pcnet_write_csr(&g_pcnet, PCNET_CSR1, (u16)((u32)(u64)&g_pcnet.dma->init_block & 0xFFFFu));
    pcnet_write_csr(&g_pcnet, PCNET_CSR2, (u16)(((u32)(u64)&g_pcnet.dma->init_block >> 16) & 0xFFFFu));
    pcnet_write_csr(&g_pcnet,
                    PCNET_CSR3,
                    (u16)(PCNET_CSR3_MASK_INIT_DONE | PCNET_CSR3_MASK_TX_DONE | PCNET_CSR3_MASK_RX_DONE));
    csr4 = pcnet_read_csr(&g_pcnet, PCNET_CSR4);
    pcnet_write_csr(&g_pcnet, PCNET_CSR4, (u16)(csr4 | PCNET_CSR4_AUTO_PAD_TX));
    pcnet_write_csr(&g_pcnet, PCNET_CSR0, PCNET_CSR0_INIT);

    if (!pcnet_wait_for_init_done(&g_pcnet)) {
        serial_write("[net] amd pcnet initialization timed out\n");
        return false;
    }

    pcnet_write_csr(&g_pcnet, PCNET_CSR0, PCNET_CSR0_IDON);
    pcnet_write_csr(&g_pcnet, PCNET_CSR0, PCNET_CSR0_START);
    csr0 = pcnet_read_csr(&g_pcnet, PCNET_CSR0);
    g_pcnet.status.initialized = (csr0 & PCNET_CSR0_STOP) == 0u;
    g_pcnet.rx_read_index = 0u;
    g_pcnet.tx_write_index = 0u;

    serial_write("[net] amd pcnet detected vendor=");
    serial_write_hex64(g_pcnet.status.vendor_id);
    serial_write(" device=");
    serial_write_hex64(g_pcnet.status.device_id);
    serial_write(" io=");
    serial_write_hex64(g_pcnet.status.io_base);
    serial_write(" irq=");
    serial_write_hex64(g_pcnet.status.irq_line);
    serial_write(" mac=");
    pcnet_write_mac(g_pcnet.status.mac);
    serial_write(g_pcnet.status.initialized ? " ready\n" : " not-started\n");

    return g_pcnet.status.initialized;
}

bool pcnet_send(const void* frame, u16 length) {
    pcnet_descriptor_t* descriptor;
    u32 descriptor_index;
    u32 poll;

    if (!g_pcnet.status.initialized || g_pcnet.dma == NULL || frame == NULL) {
        return false;
    }
    if (length == 0u || length > PCNET_BUFFER_SIZE) {
        return false;
    }

    descriptor_index = g_pcnet.tx_write_index % PCNET_TX_RING_COUNT;
    descriptor = &g_pcnet.dma->tx_ring[descriptor_index];
    if ((descriptor->status & PCNET_DESCRIPTOR_OWN) != 0u) {
        return false;
    }

    memcpy(&g_pcnet.dma->tx_buffer[descriptor_index][0], frame, length);
    descriptor->misc = 0u;
    descriptor->buffer_length = pcnet_encode_buffer_length(length);
    descriptor->status = (u8)(PCNET_DESCRIPTOR_OWN | PCNET_DESCRIPTOR_START | PCNET_DESCRIPTOR_END);

    pcnet_write_csr(&g_pcnet, PCNET_CSR0, PCNET_CSR0_TX_POLL);
    for (poll = 0u; poll < PCNET_TX_POLL_LIMIT; ++poll) {
        if ((descriptor->status & PCNET_DESCRIPTOR_OWN) == 0u) {
            pcnet_acknowledge_events(&g_pcnet);
            g_pcnet.tx_write_index = (g_pcnet.tx_write_index + 1u) % PCNET_TX_RING_COUNT;
            return (descriptor->status & PCNET_DESCRIPTOR_ERROR) == 0u;
        }
        io_wait();
    }

    return false;
}

bool pcnet_poll_receive(void* frame_buffer, u16 buffer_capacity, u16* out_length) {
    pcnet_descriptor_t* descriptor;
    u32 descriptor_index;
    u16 frame_length;

    if (!g_pcnet.status.initialized || g_pcnet.dma == NULL || frame_buffer == NULL || out_length == NULL) {
        return false;
    }

    descriptor_index = g_pcnet.rx_read_index % PCNET_RX_RING_COUNT;
    descriptor = &g_pcnet.dma->rx_ring[descriptor_index];
    if ((descriptor->status & PCNET_DESCRIPTOR_OWN) != 0u) {
        return false;
    }

    *out_length = 0u;
    frame_length = (u16)(descriptor->misc & 0x0FFFu);
    if (frame_length >= 4u) {
        frame_length = (u16)(frame_length - 4u);
    } else {
        frame_length = 0u;
    }

    if ((descriptor->status & PCNET_DESCRIPTOR_ERROR) == 0u &&
        (descriptor->status & (PCNET_DESCRIPTOR_START | PCNET_DESCRIPTOR_END)) ==
            (PCNET_DESCRIPTOR_START | PCNET_DESCRIPTOR_END) &&
        frame_length != 0u &&
        frame_length <= buffer_capacity) {
        memcpy(frame_buffer, &g_pcnet.dma->rx_buffer[descriptor_index][0], frame_length);
        *out_length = frame_length;
    }

    pcnet_initialize_descriptor(descriptor,
                                (u32)(u64)&g_pcnet.dma->rx_buffer[descriptor_index][0],
                                (u16)PCNET_BUFFER_SIZE,
                                true);
    pcnet_acknowledge_events(&g_pcnet);
    g_pcnet.rx_read_index = (g_pcnet.rx_read_index + 1u) % PCNET_RX_RING_COUNT;
    return *out_length != 0u;
}

void pcnet_get_status(pcnet_status_t* out_status) {
    if (out_status == NULL) {
        return;
    }

    *out_status = g_pcnet.status;
}
