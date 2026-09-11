#include "jabulos.h"

#define PCI_CONFIG_ADDRESS_PORT 0x0CF8u
#define PCI_CONFIG_DATA_PORT 0x0CFCu

static u32 pci_make_config_address(u8 bus, u8 device, u8 function, u8 offset) {
    return 0x80000000u |
           ((u32)bus << 16) |
           ((u32)device << 11) |
           ((u32)function << 8) |
           (offset & 0xFCu);
}

u32 pci_read_config_dword(u8 bus, u8 device, u8 function, u8 offset) {
    outl(PCI_CONFIG_ADDRESS_PORT, pci_make_config_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA_PORT);
}

u16 pci_read_config_word(u8 bus, u8 device, u8 function, u8 offset) {
    u32 value = pci_read_config_dword(bus, device, function, offset);
    u32 shift = (u32)(offset & 2u) * 8u;
    return (u16)((value >> shift) & 0xFFFFu);
}

u8 pci_read_config_byte(u8 bus, u8 device, u8 function, u8 offset) {
    u32 value = pci_read_config_dword(bus, device, function, offset);
    u32 shift = (u32)(offset & 3u) * 8u;
    return (u8)((value >> shift) & 0xFFu);
}

void pci_write_config_dword(u8 bus, u8 device, u8 function, u8 offset, u32 value) {
    outl(PCI_CONFIG_ADDRESS_PORT, pci_make_config_address(bus, device, function, offset));
    outl(PCI_CONFIG_DATA_PORT, value);
}

void pci_write_config_word(u8 bus, u8 device, u8 function, u8 offset, u16 value) {
    u8 aligned_offset = offset & 0xFCu;
    u32 shift = (u32)(offset & 2u) * 8u;
    u32 current = pci_read_config_dword(bus, device, function, aligned_offset);
    u32 mask = 0xFFFFu << shift;
    u32 updated = (current & ~mask) | ((u32)value << shift);
    pci_write_config_dword(bus, device, function, aligned_offset, updated);
}

static void pci_fill_device_info(u8 bus, u8 device, u8 function, pci_device_info_t* out_device) {
    u32 class_reg;
    u32 irq_reg;
    u32 bar_index;

    if (out_device == NULL) {
        return;
    }

    memset(out_device, 0, sizeof(*out_device));
    out_device->present = true;
    out_device->bus = bus;
    out_device->device = device;
    out_device->function = function;
    out_device->vendor_id = pci_read_config_word(bus, device, function, 0x00u);
    out_device->device_id = pci_read_config_word(bus, device, function, 0x02u);

    class_reg = pci_read_config_dword(bus, device, function, 0x08u);
    out_device->revision_id = (u8)(class_reg & 0xFFu);
    out_device->prog_if = (u8)((class_reg >> 8) & 0xFFu);
    out_device->subclass = (u8)((class_reg >> 16) & 0xFFu);
    out_device->class_code = (u8)((class_reg >> 24) & 0xFFu);

    for (bar_index = 0u; bar_index < 6u; ++bar_index) {
        out_device->bar[bar_index] = pci_read_config_dword(bus, device, function, (u8)(0x10u + bar_index * 4u));
    }

    irq_reg = pci_read_config_dword(bus, device, function, 0x3Cu);
    out_device->irq_line = (u8)(irq_reg & 0xFFu);
    out_device->irq_pin = (u8)((irq_reg >> 8) & 0xFFu);
}

bool pci_find_class_device(u8 class_code, u8 subclass, pci_device_info_t* out_device) {
    u16 vendor_id;

    if (out_device == NULL) {
        return false;
    }

    memset(out_device, 0, sizeof(*out_device));
    for (u32 bus = 0u; bus < 256u; ++bus) {
        for (u32 device = 0u; device < 32u; ++device) {
            for (u32 function = 0u; function < 8u; ++function) {
                u32 class_reg;

                vendor_id = pci_read_config_word((u8)bus, (u8)device, (u8)function, 0x00u);
                if (vendor_id == 0xFFFFu) {
                    if (function == 0u) {
                        break;
                    }
                    continue;
                }

                class_reg = pci_read_config_dword((u8)bus, (u8)device, (u8)function, 0x08u);
                if (((class_reg >> 24) & 0xFFu) == class_code &&
                    ((class_reg >> 16) & 0xFFu) == subclass) {
                    pci_fill_device_info((u8)bus, (u8)device, (u8)function, out_device);
                    return true;
                }
            }
        }
    }

    return false;
}

bool pci_find_device(u16 vendor_id, u16 device_id, pci_device_info_t* out_device) {
    if (out_device == NULL) {
        return false;
    }

    memset(out_device, 0, sizeof(*out_device));
    for (u32 bus = 0u; bus < 256u; ++bus) {
        for (u32 device = 0u; device < 32u; ++device) {
            for (u32 function = 0u; function < 8u; ++function) {
                u16 current_vendor_id;

                current_vendor_id = pci_read_config_word((u8)bus, (u8)device, (u8)function, 0x00u);
                if (current_vendor_id == 0xFFFFu) {
                    if (function == 0u) {
                        break;
                    }
                    continue;
                }

                if (current_vendor_id == vendor_id &&
                    pci_read_config_word((u8)bus, (u8)device, (u8)function, 0x02u) == device_id) {
                    pci_fill_device_info((u8)bus, (u8)device, (u8)function, out_device);
                    return true;
                }
            }
        }
    }

    return false;
}
