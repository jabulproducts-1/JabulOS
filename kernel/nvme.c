#include "jabulos.h"

// reisgers
#define NVME_REG_CAP    0x00
#define NVME_REG_VS     0x08
#define NVME_REG_CC     0x14
#define NVME_REG_CSTS   0x1C
#define NVME_REG_AQA    0x24
#define NVME_REG_ASQ    0x28
#define NVME_REG_ACQ    0x30

typedef struct {
    u32 opcode:8;
    u32 fused:2;
    u32 reserved0:22;
    u32 nsid;
    u32 reserved1[2];
    u64 metadata;
    u64 prp1;
    u64 prp2;
    u32 cdw10;
    u32 cdw11;
    u32 cdw12;
    u32 cdw13;
    u32 cdw14;
    u32 cdw15;
} nvme_command_t;

typedef struct {
    u32 result;
    u32 reserved;
    u16 sq_head;
    u16 sq_id;
    u16 command_id;
    u16 status;
} nvme_completion_t;

static u64 nvme_base = 0;
/*
static nvme_command_t* admin_sq = NULL;
static nvme_completion_t* admin_cq = NULL;
static u16 admin_sq_tail = 0;
static u16 admin_cq_head = 0;
*/
static u32 db_stride = 0;

bool nvme_initialize(void) {
    pci_device_info_t nvme_device;
    if (!pci_find_class_device(0x01, 0x08, &nvme_device)) {
        return false;
    }
    nvme_base = (u64)nvme_device.bar[0]; 
    
    u64 cap = *(volatile u64*)(nvme_base + NVME_REG_CAP);
    db_stride = 1 << (2 + ((cap >> 32) & 0x0F));

    *(volatile u32*)(nvme_base + NVME_REG_CC) &= ~1;
    while (*(volatile u32*)(nvme_base + NVME_REG_CSTS) & 1); 


    *(volatile u32*)(nvme_base + NVME_REG_CC) |= 1;
    while (!(*(volatile u32*)(nvme_base + NVME_REG_CSTS) & 1));

    return true;
}

bool nvme_read_sectors(u64 lba, u32 count, void* buffer) {
    (void)lba;
    (void)count;
    (void)buffer;
    if (nvme_base == 0) return false;
    return true; 
}

bool nvme_write_sectors(u64 lba, u32 count, const void* buffer) {
    (void)lba;
    (void)count;
    (void)buffer;
    if (nvme_base == 0) return false;
    return true;
}
