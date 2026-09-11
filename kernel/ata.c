#include "jabulos.h"

#define ATA_IO_BASE 0x1F0
#define ATA_CTRL_BASE 0x3F6
#define ATA_REG_DATA 0
#define ATA_REG_SECTOR_COUNT 2
#define ATA_REG_LBA_LOW 3
#define ATA_REG_LBA_MID 4
#define ATA_REG_LBA_HIGH 5
#define ATA_REG_DRIVE 6
#define ATA_REG_COMMAND 7
#define ATA_REG_STATUS 7

#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF  0x20
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_BSY 0x80

static void ata_delay_400ns(void) {
    inb(ATA_CTRL_BASE);
    inb(ATA_CTRL_BASE);
    inb(ATA_CTRL_BASE);
    inb(ATA_CTRL_BASE);
}

static bool ata_wait(u8 mask, bool set) {
    for (u32 attempt = 0; attempt < 100000; ++attempt) {
        u8 status = inb(ATA_IO_BASE + ATA_REG_STATUS);
        if ((status & ATA_STATUS_ERR) || (status & ATA_STATUS_DF)) {
            return false;
        }
        if (set ? ((status & mask) == mask) : ((status & mask) == 0)) {
            return true;
        }
    }
    return false;
}

static void ata_select_drive(u32 lba) {
    outb(ATA_CTRL_BASE, 0x02);
    outb(ATA_IO_BASE + ATA_REG_DRIVE, (u8)(0xE0 | ((lba >> 24) & 0x0F)));
    ata_delay_400ns();
}

static bool ata_identify_words(u16* identify_data) {
    if (identify_data == NULL) {
        return false;
    }

    ata_select_drive(0);
    outb(ATA_IO_BASE + ATA_REG_SECTOR_COUNT, 0);
    outb(ATA_IO_BASE + ATA_REG_LBA_LOW, 0);
    outb(ATA_IO_BASE + ATA_REG_LBA_MID, 0);
    outb(ATA_IO_BASE + ATA_REG_LBA_HIGH, 0);
    outb(ATA_IO_BASE + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    if (inb(ATA_IO_BASE + ATA_REG_STATUS) == 0) {
        return false;
    }

    if (!ata_wait(ATA_STATUS_BSY, false)) {
        return false;
    }

    if (inb(ATA_IO_BASE + ATA_REG_LBA_MID) != 0 || inb(ATA_IO_BASE + ATA_REG_LBA_HIGH) != 0) {
        return false;
    }

    if (!ata_wait(ATA_STATUS_DRQ, true)) {
        return false;
    }

    insw(ATA_IO_BASE + ATA_REG_DATA, identify_data, 256);
    return true;
}

static void ata_copy_model_string(char* destination, const u16* identify_data) {
    u32 out_index = 0;

    if (destination == NULL || identify_data == NULL) {
        return;
    }

    for (u32 word = 27u; word <= 46u && out_index + 1u < 40u; ++word) {
        char high = (char)(identify_data[word] >> 8);
        char low = (char)(identify_data[word] & 0xFFu);

        destination[out_index++] = high;
        if (out_index + 1u >= 40u) {
            break;
        }
        destination[out_index++] = low;
    }

    while (out_index > 0u && destination[out_index - 1u] == ' ') {
        --out_index;
    }
    destination[out_index] = '\0';
}

static void ata_load_partition_info(ata_device_info_t* out_info) {
    u8 sector[512];

    if (out_info == NULL) {
        return;
    }

    if (!ata_pio_read_sectors(0, 1, sector)) {
        return;
    }

    if (sector[510] != 0x55u || sector[511] != 0xAAu) {
        return;
    }

    for (u32 index = 0; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
        const u8* entry = sector + 446u + index * 16u;
        ata_partition_info_t* partition = &out_info->partitions[index];
        u32 start_lba;
        u32 sector_count;

        start_lba = (u32)entry[8] |
                    ((u32)entry[9] << 8) |
                    ((u32)entry[10] << 16) |
                    ((u32)entry[11] << 24);
        sector_count = (u32)entry[12] |
                       ((u32)entry[13] << 8) |
                       ((u32)entry[14] << 16) |
                       ((u32)entry[15] << 24);

        if (sector_count == 0u) {
            continue;
        }

        partition->present = true;
        partition->bootable = entry[0] == 0x80u;
        partition->partition_type = entry[4];
        partition->start_lba = start_lba;
        partition->sector_count = sector_count;
    }
}

bool ata_identify_primary_master(void) {
    u16 identify_data[256];
    return ata_identify_words(identify_data);
}

bool ata_read_primary_master_info(ata_device_info_t* out_info) {
    u16 identify_data[256];

    if (out_info == NULL) {
        return false;
    }

    memset(out_info, 0, sizeof(*out_info));
    if (!ata_identify_words(identify_data)) {
        return false;
    }

    out_info->present = true;
    out_info->total_sectors = (u32)identify_data[60] | ((u32)identify_data[61] << 16);
    ata_copy_model_string(out_info->model, identify_data);
    ata_load_partition_info(out_info);
    return true;
}

bool ata_pio_read_sectors(u32 lba, u8 count, void* buffer) {
    u16* words = (u16*)buffer;

    if (count == 0 || (lba & 0xF0000000) != 0) {
        return false;
    }

    ata_select_drive(lba);
    outb(ATA_IO_BASE + ATA_REG_SECTOR_COUNT, count);
    outb(ATA_IO_BASE + ATA_REG_LBA_LOW, (u8)(lba & 0xFF));
    outb(ATA_IO_BASE + ATA_REG_LBA_MID, (u8)((lba >> 8) & 0xFF));
    outb(ATA_IO_BASE + ATA_REG_LBA_HIGH, (u8)((lba >> 16) & 0xFF));
    outb(ATA_IO_BASE + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);

    for (u8 sector = 0; sector < count; ++sector) {
        if (!ata_wait(ATA_STATUS_BSY, false) || !ata_wait(ATA_STATUS_DRQ, true)) {
            return false;
        }
        insw(ATA_IO_BASE + ATA_REG_DATA, words + sector * 256, 256);
        ata_delay_400ns();
    }

    return true;
}

bool ata_pio_write_sectors(u32 lba, u8 count, const void* buffer) {
    const u16* words = (const u16*)buffer;

    if (count == 0 || (lba & 0xF0000000) != 0) {
        return false;
    }

    ata_select_drive(lba);
    outb(ATA_IO_BASE + ATA_REG_SECTOR_COUNT, count);
    outb(ATA_IO_BASE + ATA_REG_LBA_LOW, (u8)(lba & 0xFF));
    outb(ATA_IO_BASE + ATA_REG_LBA_MID, (u8)((lba >> 8) & 0xFF));
    outb(ATA_IO_BASE + ATA_REG_LBA_HIGH, (u8)((lba >> 16) & 0xFF));
    outb(ATA_IO_BASE + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);

    for (u8 sector = 0; sector < count; ++sector) {
        if (!ata_wait(ATA_STATUS_BSY, false) || !ata_wait(ATA_STATUS_DRQ, true)) {
            return false;
        }
        outsw(ATA_IO_BASE + ATA_REG_DATA, words + sector * 256, 256);
        ata_delay_400ns();
    }

    outb(ATA_IO_BASE + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    return ata_wait(ATA_STATUS_BSY, false) && ata_wait(ATA_STATUS_DRDY, true);
}
