#include "jabulos.h"

struct __attribute__((packed)) idt_entry {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
};

struct __attribute__((packed)) idt_descriptor {
    u16 limit;
    u64 base;
};

extern void isr_stub_0(void);

struct interrupt_frame {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;
    u64 vector;
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};

static struct idt_entry idt[256] __attribute__((aligned(16)));
static struct idt_descriptor idtr;

static void pic_mask_all(void) {
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

static void set_idt_entry(u32 vector, u64 handler) {
    idt[vector].offset_low = (u16)(handler & 0xFFFF);
    idt[vector].selector = 0x08;
    idt[vector].ist = 0;
    idt[vector].type_attr = 0x8E;
    idt[vector].offset_mid = (u16)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (u32)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].zero = 0;
}

void idt_initialize(void) {
    serial_write("[boot] idt fill begin\n");
    for (u32 vector = 0; vector < 256; ++vector) {
        set_idt_entry(vector, (u64)&isr_stub_0);
    }
    serial_write("[boot] idt fill done\n");

    idtr.limit = (u16)(sizeof(idt) - 1);
    idtr.base = (u64)&idt[0];

    serial_write("[boot] pic masked\n");
    pic_mask_all();
    serial_write("[boot] loading idtr\n");
    asm volatile ("lidt %0" : : "m"(idtr));
    serial_write("[boot] idtr loaded\n");
}

void isr_default_handler(u64* interrupt_frame) {
    struct interrupt_frame* frame = (struct interrupt_frame*)interrupt_frame;

    serial_write("[fault] unhandled interrupt vector=");
    serial_write_hex64(frame->vector);
    serial_write(" error=");
    serial_write_hex64(frame->error_code);
    serial_write(" rip=");
    serial_write_hex64(frame->rip);
    serial_write("\n");
    halt_forever();
}
