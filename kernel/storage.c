#include "jabulos.h"

typedef enum {
    STORAGE_TYPE_NONE,
    STORAGE_TYPE_IDE,
    STORAGE_TYPE_AHCI,
    STORAGE_TYPE_NVME
} storage_type_t;

static storage_type_t current_storage_type = STORAGE_TYPE_NONE;

bool storage_initialize(void) {
    if (nvme_initialize()) {
        current_storage_type = STORAGE_TYPE_NVME;
        serial_write("[storage] NVMe controller initialized\n");
        return true;
    }

    if (ahci_initialize()) {
        current_storage_type = STORAGE_TYPE_AHCI;
        serial_write("[storage] AHCI controller initialized\n");
        return true;
    }

    if (ata_identify_primary_master()) {
        current_storage_type = STORAGE_TYPE_IDE;
        serial_write("[storage] Legacy IDE controller detected\n");
        return true;
    }

    serial_write("[storage] No supported storage controller found\n");
    return false;
}

bool storage_read_sectors(u64 lba, u32 count, void* buffer) {
    switch (current_storage_type) {
        case STORAGE_TYPE_NVME:
            return nvme_read_sectors(lba, count, buffer);
        case STORAGE_TYPE_AHCI:
            return ahci_read_sectors((u32)lba, count, buffer);
        case STORAGE_TYPE_IDE:
            return ata_pio_read_sectors((u32)lba, (u8)count, buffer);
        default:
            return false;
    }
}

bool storage_write_sectors(u64 lba, u32 count, const void* buffer) {
    switch (current_storage_type) {
        case STORAGE_TYPE_NVME:
            return nvme_write_sectors(lba, count, buffer);
        case STORAGE_TYPE_AHCI:
            return ahci_write_sectors((u32)lba, count, buffer);
        case STORAGE_TYPE_IDE:
            return ata_pio_write_sectors((u32)lba, (u8)count, buffer);
        default:
            return false;
    }
}

bool storage_get_info(ata_device_info_t* out_info) {
    if (out_info == NULL) return false;

    if (current_storage_type == STORAGE_TYPE_IDE) {
        return ata_read_primary_master_info(out_info);
    }

    // For AHCI and NVMe, we should ideally implement identify commands.
    // For now, let's fill in some basic info if they are present.
    memset(out_info, 0, sizeof(*out_info));
    out_info->present = (current_storage_type != STORAGE_TYPE_NONE);
    
    if (current_storage_type == STORAGE_TYPE_AHCI) {
        strcpy(out_info->model, "SATA AHCI Drive");
        out_info->total_sectors = ahci_get_total_sectors();
    } else if (current_storage_type == STORAGE_TYPE_NVME) {
        strcpy(out_info->model, "NVMe Express Drive");
    }

    // Attempt to load partition info using the generic read function
    u8 sector[512];
    if (storage_read_sectors(0, 1, sector)) {
        if (sector[510] == 0x55 && sector[511] == 0xAA) {
            for (u32 i = 0; i < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++i) {
                const u8* entry = sector + 446 + i * 16;
                u32 sector_count = (u32)entry[12] | ((u32)entry[13] << 8) | ((u32)entry[14] << 16) | ((u32)entry[15] << 24);
                if (sector_count == 0) continue;
                
                out_info->partitions[i].present = true;
                out_info->partitions[i].bootable = (entry[0] == 0x80);
                out_info->partitions[i].partition_type = entry[4];
                out_info->partitions[i].start_lba = (u32)entry[8] | ((u32)entry[9] << 8) | ((u32)entry[10] << 16) | ((u32)entry[11] << 24);
                out_info->partitions[i].sector_count = sector_count;
            }
        }
    }

    return out_info->present;
}
