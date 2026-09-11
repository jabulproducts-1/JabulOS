#include "jabulos.h"

struct __attribute__((packed)) gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access;
    u8 granularity;
    u8 base_high;
};

struct __attribute__((packed)) gdt_descriptor {
    u16 limit;
    u64 base;
};

static struct gdt_entry gdt[5] __attribute__((aligned(16)));
static struct gdt_descriptor gdtr;

static void set_gdt_entry(u32 index, u32 base, u32 limit, u8 access, u8 granularity) {
    gdt[index].limit_low = (u16)(limit & 0xFFFF);
    gdt[index].base_low = (u16)(base & 0xFFFF);
    gdt[index].base_mid = (u8)((base >> 16) & 0xFF);
    gdt[index].access = access;
    gdt[index].granularity = (u8)(((limit >> 16) & 0x0F) | (granularity & 0xF0));
    gdt[index].base_high = (u8)((base >> 24) & 0xFF);
}

void gdt_initialize(void) {
    memset(gdt, 0, sizeof(gdt));

    set_gdt_entry(1, 0, 0, 0x9A, 0x20);
    set_gdt_entry(2, 0, 0, 0x92, 0x00);
    set_gdt_entry(3, 0, 0, 0xFA, 0x20);
    set_gdt_entry(4, 0, 0, 0xF2, 0x00);

    gdtr.limit = (u16)(sizeof(gdt) - 1);
    gdtr.base = (u64)&gdt[0];

    asm volatile ("lgdt %0" : : "m"(gdtr));
}
