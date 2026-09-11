#include "xhci.h"
#include "jabulos.h"

static xhci_controller_t g_xhci = {0};

// xHCI Register offsets
#define XHCI_CAPLENGTH  0x00
#define XHCI_HCSPARAMS1 0x04
#define XHCI_HCCPARAMS1 0x10
#define XHCI_DBOFF      0x14
#define XHCI_RTSOFF      0x18

#define XHCI_USBCMD     0x00 // Relative to Operational Base
#define XHCI_USBSTS     0x04
#define XHCI_PAGESIZE   0x08
#define XHCI_CONFIG     0x38

void xhci_initialize(void) {
    pci_device_info_t pci_info;
    
    // Find USB 3.0 xHCI controller (Class 0C, Subclass 03, ProgIF 30)
    if (!pci_find_class_device(0x0C, 0x03, &pci_info)) {
        serial_write("[xhci] No xHCI controller found\n");
        return;
    }

    if (pci_info.prog_if != 0x30) {
        serial_write("[xhci] Found USB controller but not xHCI\n");
        return;
    }

    g_xhci.present = true;
    g_xhci.bus = pci_info.bus;
    g_xhci.device = pci_info.device;
    g_xhci.function = pci_info.function;

    // BAR0 is the MMIO base
    u32 bar0 = pci_info.bar[0];
    u32 bar1 = pci_info.bar[1];
    g_xhci.mmio_base = (u64)(bar0 & 0xFFFFFFF0);
    if ((bar0 & 0x06) == 0x04) { // 64-bit address
        g_xhci.mmio_base |= ((u64)bar1 << 32);
    }

    serial_write("[xhci] Found controller at ");
    serial_write_hex64(g_xhci.mmio_base);
    serial_write("\n");

    // Enable MMIO and Bus Mastering
    u16 command = pci_read_config_word(g_xhci.bus, g_xhci.device, g_xhci.function, 0x04);
    pci_write_config_word(g_xhci.bus, g_xhci.device, g_xhci.function, 0x04, command | 0x06);

    u8 cap_length = mmio_read8(g_xhci.mmio_base + XHCI_CAPLENGTH);
    u64 op_base = g_xhci.mmio_base + cap_length;

    // Reset controller
    mmio_write32(op_base + XHCI_USBCMD, 0x02); // HCRST
    while (mmio_read32(op_base + XHCI_USBCMD) & 0x02);
    while (mmio_read32(op_base + XHCI_USBSTS) & 0x800); // CNR (Controller Not Ready)

    serial_write("[xhci] Controller reset complete\n");

    u32 hcs1 = mmio_read32(g_xhci.mmio_base + XHCI_HCSPARAMS1);
    g_xhci.max_slots = (hcs1 >> 24) & 0xFF;
    
    u32 pagesize_reg = mmio_read32(op_base + XHCI_PAGESIZE);
    g_xhci.page_size = pagesize_reg << 12;

    serial_write("[xhci] Max slots: ");
    serial_write_hex32(g_xhci.max_slots);
    serial_write("\n");
}

void xhci_poll(void) {
    if (!g_xhci.present) return;
    // Poll for events if we had rings set up.
}
