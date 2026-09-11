#include "jabulos.h"

#define COM1_PORT 0x3F8

static bool g_serial_ready = false;

static bool serial_is_transmit_empty(void) {
    return (inb(COM1_PORT + 5) & 0x20) != 0;
}

static bool serial_received(void) {
    return (inb(COM1_PORT + 5) & 0x01) != 0;
}

static void serial_write_byte(char value) {
    if (!g_serial_ready) {
        return;
    }

    while (!serial_is_transmit_empty()) {
    }

    outb(COM1_PORT, (u8)value);
}

void serial_initialize(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
    g_serial_ready = true;
}

void serial_write(const char* text) {
    if (text == NULL) {
        return;
    }

    while (*text != '\0') {
        if (*text == '\n') {
            serial_write_byte('\r');
        }
        serial_write_byte(*text++);
    }
}

void serial_write_hex64(u64 value) {
    static const char digits[] = "0123456789ABCDEF";

    serial_write("0x");
    for (s32 shift = 60; shift >= 0; shift -= 4) {
        serial_write_byte(digits[(value >> shift) & 0x0F]);
    }
}

void serial_write_hex32(u32 value) {
    static const char digits[] = "0123456789ABCDEF";

    serial_write("0x");
    for (s32 shift = 28; shift >= 0; shift -= 4) {
        serial_write_byte(digits[(value >> shift) & 0x0F]);
    }
}

bool serial_read_byte(u8* out_value) {
    if (out_value == NULL) {
        return false;
    }
    if (!g_serial_ready) {
        return false;
    }
    if (!serial_received()) {
        return false;
    }
    *out_value = inb(COM1_PORT);
    return true;
}
