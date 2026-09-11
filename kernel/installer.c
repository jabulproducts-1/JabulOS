#include "jabulos.h"

struct __attribute__((packed)) installer_header {
    char magic[8];
    u32 version;
    u32 kernel_lba;
    u32 kernel_size;
    u32 kernel_sectors;
    u32 initrd_lba;
    u32 initrd_size;
    u32 initrd_sectors;
    u32 total_sectors;
    u8 reserved[476];
};

static u32 sector_count_for(u32 size) {
    return (size + 511u) / 512u;
}

static bool installer_header_present_at(u32 lba) {
    u8 sector[512];

    if (!storage_read_sectors(lba, 1, sector)) {
        return false;
    }

    return memcmp(sector, "JBAERO1", 7) == 0;
}

static bool write_blob(u32 lba, const void* data, u32 size) {
    u8 sector[512];
    const u8* bytes = (const u8*)data;
    u32 total_sectors = sector_count_for(size);

    for (u32 index = 0; index < total_sectors; ++index) {
        u32 remaining = size - index * 512u;
        u32 chunk = (remaining > 512u) ? 512u : remaining;
        memset(sector, 0, sizeof(sector));
        memcpy(sector, bytes + index * 512u, chunk);

        if (!storage_write_sectors(lba + index, 1, sector)) {
            return false;
        }
    }

    return true;
}

bool installer_install_to_ata0(const void* kernel_image,
                               u32 kernel_size,
                               const void* initrd_image,
                               u32 initrd_size) {
    return installer_install_to_target(kernel_image, kernel_size, initrd_image, initrd_size, 0u, 0u);
}

bool installer_detect_existing_install(void) {
    ata_device_info_t device_info;

    if (installer_header_present_at(0)) {
        return true;
    }

    if (!storage_get_info(&device_info)) {
        return false;
    }

    for (u32 index = 0; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
        const ata_partition_info_t* partition = &device_info.partitions[index];
        if (partition->present && installer_header_present_at(partition->start_lba)) {
            return true;
        }
    }

    return false;
}

bool installer_find_existing_install(u32* out_lba, u32* out_sector_count) {
    ata_device_info_t device_info;

    if (out_lba != NULL) {
        *out_lba = 0u;
    }
    if (out_sector_count != NULL) {
        *out_sector_count = 0u;
    }

    if (!storage_get_info(&device_info) || !device_info.present || device_info.total_sectors == 0u) {
        return false;
    }

    if (installer_header_present_at(0)) {
        if (out_lba != NULL) {
            *out_lba = 0u;
        }
        if (out_sector_count != NULL) {
            *out_sector_count = device_info.total_sectors;
        }
        return true;
    }

    for (u32 index = 0; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
        const ata_partition_info_t* partition = &device_info.partitions[index];
        if (partition->present && installer_header_present_at(partition->start_lba)) {
            if (out_lba != NULL) {
                *out_lba = partition->start_lba;
            }
            if (out_sector_count != NULL) {
                *out_sector_count = partition->sector_count;
            }
            return true;
        }
    }

    return false;
}

bool installer_install_to_target(const void* kernel_image,
                                 u32 kernel_size,
                                 const void* initrd_image,
                                 u32 initrd_size,
                                 u32 target_lba,
                                 u32 target_sector_count) {
    struct installer_header header;
    u32 kernel_sectors;
    u32 initrd_sectors;
    u32 total_required;

    if (!storage_initialize() || kernel_image == NULL || kernel_size == 0) {
        return false;
    }

    kernel_sectors = sector_count_for(kernel_size);
    initrd_sectors = sector_count_for(initrd_size);
    total_required = 1u + kernel_sectors + initrd_sectors;

    if (target_sector_count != 0u && total_required > target_sector_count) {
        return false;
    }

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, "JBAERO1", 7);
    header.version = 1;
    header.kernel_lba = target_lba + 1u;
    header.kernel_size = kernel_size;
    header.kernel_sectors = kernel_sectors;
    header.initrd_lba = header.kernel_lba + kernel_sectors;
    header.initrd_size = initrd_size;
    header.initrd_sectors = initrd_sectors;
    header.total_sectors = total_required;

    if (!storage_write_sectors(target_lba, 1, &header)) {
        return false;
    }

    if (!write_blob(header.kernel_lba, kernel_image, kernel_size)) {
        return false;
    }

    if (initrd_image != NULL && initrd_size != 0 && !write_blob(header.initrd_lba, initrd_image, initrd_size)) {
        return false;
    }

    return true;
}
