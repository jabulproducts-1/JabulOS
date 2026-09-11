#include "jabulos.h"

// AHCI Registers and Structures
#define SATA_SIG_ATA    0x00000101
#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define AHCI_GHC_AE (1U << 31)

typedef enum {
    FIS_TYPE_REG_H2D = 0x27,
    FIS_TYPE_REG_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1,
} FIS_TYPE;

typedef struct {
    u32 clb;
    u32 clbu;
    u32 fb;
    u32 fbu;
    u32 is;
    u32 ie;
    u32 cmd;
    u32 reserved0;
    u32 tfd;
    u32 sig;
    u32 ssts;
    u32 sctl;
    u32 serr;
    u32 sact;
    u32 ci;
    u32 sntf;
    u32 fbs;
    u32 reserved1[11];
    u32 vendor[4];
} HBA_PORT;

typedef struct {
    u32 cap;
    u32 ghc;
    u32 is;
    u32 pi;
    u32 vs;
    u32 ccc_ctl;
    u32 ccc_pts;
    u32 em_loc;
    u32 em_ctl;
    u32 cap2;
    u32 bohc;
    u8  reserved[0xA0 - 0x2C];
    u8  vendor[0x100 - 0xA0];
    HBA_PORT ports[32];
} HBA_MEM;

typedef struct {
    u8  cfl:5;
    u8  a:1;
    u8  w:1;
    u8  p:1;
    u8  r:1;
    u8  b:1;
    u8  c:1;
    u8  reserved0:1;
    u8  pmp:4;
    u16 prdtl;
    u32 prdbc;
    u32 ctba;
    u32 ctbau;
    u32 reserved1[4];
} HBA_CMD_HEADER;

typedef struct {
    u32 dba;
    u32 dbau;
    u32 reserved0;
    u32 dbc:22;
    u32 reserved1:9;
    u32 i:1;
} HBA_PRDT_ENTRY;

typedef struct {
    u8  cfis[64];
    u8  acmd[16];
    u8  reserved[48];
    HBA_PRDT_ENTRY prdt_entry[1];
} HBA_CMD_TBL;

static HBA_MEM* hba_mem = NULL;
static int active_port = -1;
static u32 ahci_total_sectors = 0;

static bool ahci_port_stop(HBA_PORT* port) {
    port->cmd &= ~0x0001; // ST
    port->cmd &= ~0x0010; // FRE
    
    u32 spin = 0;
    while (spin < 1000000) {
        if (!(port->cmd & 0x4000) && !(port->cmd & 0x8000)) return true;
        spin++;
        io_wait();
    }
    return false;
}

static bool ahci_port_start(HBA_PORT* port) {
    u32 spin = 0;
    while (port->cmd & 0x8000 && spin < 1000000) {
        spin++;
        io_wait();
    }
    if (spin == 1000000) return false;

    port->cmd |= 0x0010; // FRE
    port->cmd |= 0x0001; // ST
    return true;
}

static int find_cmd_slot(HBA_PORT* port) {
    u32 slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0) return i;
        slots >>= 1;
    }
    return -1;
}

bool ahci_identify(int port_no, u32* out_sectors) {
    HBA_PORT* port = &hba_mem->ports[port_no];
    int slot = find_cmd_slot(port);
    if (slot == -1) return false;

    u64 clb_phys = ((u64)port->clbu << 32) | port->clb;
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)clb_phys;
    cmdheader += slot;
    cmdheader->cfl = 5;
    cmdheader->w = 0;
    cmdheader->prdtl = 1;

    u64 ctba_phys = ((u64)cmdheader->ctbau << 32) | cmdheader->ctba;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)ctba_phys;
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));

    u16* identify_data = (u16*)pmm_alloc_frame();
    memset(identify_data, 0, 4096);

    cmdtbl->prdt_entry[0].dba = (u32)(u64)identify_data;
    cmdtbl->prdt_entry[0].dbau = (u32)((u64)identify_data >> 32);
    cmdtbl->prdt_entry[0].dbc = 511; // 512 bytes
    cmdtbl->prdt_entry[0].i = 1;

    u8* fis = cmdtbl->cfis;
    fis[0] = FIS_TYPE_REG_H2D;
    fis[1] = 0x80;
    fis[2] = 0xEC; // IDENTIFY

    u32 spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
        spin++;
        io_wait();
    }
    if (spin == 1000000) return false;

    port->ci = (1 << slot);

    spin = 0;
    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) return false;
        spin++;
        if (spin > 1000000) return false;
        io_wait();
    }

    if (out_sectors) {
        *out_sectors = (u32)identify_data[60] | ((u32)identify_data[61] << 16);
    }

    pmm_free_frame((u64)identify_data);
    return true;
}

bool ahci_initialize(void) {
    pci_device_info_t ahci_device;
    if (!pci_find_class_device(0x01, 0x06, &ahci_device)) {
        return false;
    }

    // Enable PCI Bus Mastering and Memory Space
    u16 command = pci_read_config_word(ahci_device.bus, ahci_device.device, ahci_device.function, 0x04);
    command |= (1 << 2) | (1 << 1); // Bus Master, Memory Space
    pci_write_config_word(ahci_device.bus, ahci_device.device, ahci_device.function, 0x04, command);

    hba_mem = (HBA_MEM*)((u64)ahci_device.bar[5] & 0xFFFFFFF0ULL);
    hba_mem->ghc |= AHCI_GHC_AE; // Enable AHCI mode

    u32 pi = hba_mem->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            HBA_PORT* port = &hba_mem->ports[i];
            u32 ssts = port->ssts;
            u8 det = ssts & 0x0F;
            u8 ipm = (ssts >> 8) & 0x0F;

            if (det == HBA_PORT_DET_PRESENT && ipm == HBA_PORT_IPM_ACTIVE) {
                if (port->sig == SATA_SIG_ATA) {
                    if (!ahci_port_stop(port)) {
                        continue;
                    }

                    // Allocate memory for Command List, FIS, and one Command Table
                    u64 clb_phys = pmm_alloc_frame();
                    u64 fb_phys = pmm_alloc_frame();
                    u64 ctba_phys = pmm_alloc_frame();

                    if (!clb_phys || !fb_phys || !ctba_phys) {
                        serial_write("[ahci] failed to allocate frames for port ");
                        serial_write_hex64(i);
                        serial_write("\n");
                        continue;
                    }

                    port->clb = (u32)clb_phys;
                    port->clbu = (u32)(clb_phys >> 32);
                    port->fb = (u32)fb_phys;
                    port->fbu = (u32)(fb_phys >> 32);

                    HBA_CMD_HEADER* headers = (HBA_CMD_HEADER*)clb_phys;
                    memset(headers, 0, 1024); // 32 slots * 32 bytes

                    for (int j = 0; j < 32; j++) {
                        headers[j].prdtl = 8; // Max 8 entries per table
                        u64 table_phys = ctba_phys + (j * 256); // Simple mapping
                        headers[j].ctba = (u32)table_phys;
                        headers[j].ctbau = (u32)(table_phys >> 32);
                    }

                    port->serr = 0xFFFFFFFF; // Clear errors
                    if (!ahci_port_start(port)) {
                        continue;
                    }
                    
                    if (ahci_identify(i, &ahci_total_sectors)) {
                        active_port = i;
                        serial_write("[ahci] initialized port ");
                        serial_write_hex64(i);
                        serial_write(" capacity: ");
                        serial_write_hex64(ahci_total_sectors);
                        serial_write(" sectors\n");
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

u32 ahci_get_total_sectors(void) {
    return ahci_total_sectors;
}

bool ahci_read_sectors(u32 lba, u32 count, void* buffer) {
    if (active_port == -1) return false;

    HBA_PORT* port = &hba_mem->ports[active_port];
    int slot = find_cmd_slot(port);
    if (slot == -1) return false;

    u64 clb_phys = ((u64)port->clbu << 32) | port->clb;
    
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)clb_phys;
    cmdheader += slot;
    cmdheader->cfl = 5; 
    cmdheader->w = 0;
    cmdheader->prdtl = 1;

    u64 ctba_phys = ((u64)cmdheader->ctbau << 32) | cmdheader->ctba;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)ctba_phys;
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));

    cmdtbl->prdt_entry[0].dba = (u32)(u64)buffer;
    cmdtbl->prdt_entry[0].dbau = (u32)((u64)buffer >> 32);
    cmdtbl->prdt_entry[0].dbc = (count << 9) - 1; 
    cmdtbl->prdt_entry[0].i = 1;

    u8* fis = cmdtbl->cfis;
    fis[0] = FIS_TYPE_REG_H2D;
    fis[1] = 0x80; 
    fis[2] = 0x25; // READ DMA EXT
    
    fis[4] = (u8)lba;
    fis[5] = (u8)(lba >> 8);
    fis[6] = (u8)(lba >> 16);
    fis[7] = 0x40;

    fis[8] = (u8)(lba >> 24);
    fis[9] = 0;
    fis[10] = 0;
    
    fis[12] = (u8)count;
    fis[13] = (u8)(count >> 8);

    u32 spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
        spin++;
        io_wait();
    }
    if (spin == 1000000) return false;

    port->ci = (1 << slot);

    spin = 0;
    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) return false; 
        spin++;
        if (spin > 1000000) return false;
        io_wait();
    }

    return true;
}

bool ahci_write_sectors(u32 lba, u32 count, const void* buffer) {
    if (active_port == -1) return false;

    HBA_PORT* port = &hba_mem->ports[active_port];
    int slot = find_cmd_slot(port);
    if (slot == -1) return false;

    u64 clb_phys = ((u64)port->clbu << 32) | port->clb;
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)clb_phys;
    cmdheader += slot;
    cmdheader->cfl = 5;
    cmdheader->w = 1; 
    cmdheader->prdtl = 1;

    u64 ctba_phys = ((u64)cmdheader->ctbau << 32) | cmdheader->ctba;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)ctba_phys;
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));

    cmdtbl->prdt_entry[0].dba = (u32)(u64)buffer;
    cmdtbl->prdt_entry[0].dbau = (u32)((u64)buffer >> 32);
    cmdtbl->prdt_entry[0].dbc = (count << 9) - 1;
    cmdtbl->prdt_entry[0].i = 1;

    u8* fis = cmdtbl->cfis;
    fis[0] = FIS_TYPE_REG_H2D;
    fis[1] = 0x80;
    fis[2] = 0x35; // WRITE DMA EXT
    
    fis[4] = (u8)lba;
    fis[5] = (u8)(lba >> 8);
    fis[6] = (u8)(lba >> 16);
    fis[7] = 0x40;

    fis[8] = (u8)(lba >> 24);
    fis[12] = (u8)count;
    fis[13] = (u8)(count >> 8);

    u32 spin = 0;
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) {
        spin++;
        io_wait();
    }
    if (spin == 1000000) return false;

    port->ci = (1 << slot);

    spin = 0;
    while (1) {
        if ((port->ci & (1 << slot)) == 0) break;
        if (port->is & (1 << 30)) return false;
        spin++;
        if (spin > 1000000) return false;
        io_wait();
    }

    return true;
}
