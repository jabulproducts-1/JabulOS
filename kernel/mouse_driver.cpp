#include "jabulos.h"

static bool ps2_wait_read_ready(void) {
    for (u32 attempt = 0; attempt < 100000u; ++attempt) {
        if ((inb(0x64) & 0x01u) != 0) {
            return true;
        }
    }
    return false;
}

static bool ps2_wait_write_ready(void) {
    for (u32 attempt = 0; attempt < 100000u; ++attempt) {
        if ((inb(0x64) & 0x02u) == 0) {
            return true;
        }
    }
    return false;
}

static bool mouse_read_byte(u8* out_byte) {
    for (u32 attempt = 0; attempt < 100000u; ++attempt) {
        u8 status = inb(0x64);

        if ((status & 0x01u) == 0) {
            continue;
        }

        if ((status & 0x20u) == 0) {
            (void)inb(0x60);
            continue;
        }

        if (out_byte != NULL) {
            *out_byte = inb(0x60);
        } else {
            (void)inb(0x60);
        }
        return true;
    }

    return false;
}

static bool mouse_write_command(u8 value) {
    if (!ps2_wait_write_ready()) {
        return false;
    }
    outb(0x64, 0xD4);

    if (!ps2_wait_write_ready()) {
        return false;
    }
    outb(0x60, value);
    return true;
}

static bool mouse_write_and_ack(u8 value) {
    u8 response;

    if (!mouse_write_command(value)) {
        return false;
    }

    if (!mouse_read_byte(&response)) {
        return false;
    }

    return response == 0xFAu;
}

static bool mouse_apply_packet(mouse_driver_state_t* mouse) {
    s32 new_x;
    s32 new_y;
    bool left_now;
    bool right_now;
    bool changed = false;

    if (mouse == NULL) {
        return false;
    }

    left_now = (mouse->packet[0] & 0x01u) != 0;
    if (left_now != mouse->left_down) {
        changed = true;
    }
    mouse->left_down = left_now;

    right_now = (mouse->packet[0] & 0x02u) != 0;
    if (right_now != mouse->right_down) {
        changed = true;
    }
    mouse->right_down = right_now;

    if ((mouse->packet[0] & 0xC0u) != 0) {
        return changed;
    }

    new_x = (s32)mouse->x + (s32)((s8)mouse->packet[1]);
    new_y = (s32)mouse->y - (s32)((s8)mouse->packet[2]);

    if (new_x < 0) {
        new_x = 0;
    } else if ((u32)new_x >= display_driver_width()) {
        new_x = (s32)display_driver_width() - 1;
    }

    if (new_y < 0) {
        new_y = 0;
    } else if ((u32)new_y >= display_driver_height()) {
        new_y = (s32)display_driver_height() - 1;
    }

    if ((u32)new_x != mouse->x || (u32)new_y != mouse->y) {
        changed = true;
    }

    mouse->x = (u32)new_x;
    mouse->y = (u32)new_y;
    return changed;
}

static bool mouse_handle_byte(mouse_driver_state_t* mouse, u8 value) {
    if (mouse == NULL || !mouse->initialized) {
        return false;
    }

    if (mouse->packet_index == 0u && (value & 0x08u) == 0) {
        return false;
    }

    mouse->packet[mouse->packet_index++] = value;
    if (mouse->packet_index < 3u) {
        return false;
    }

    mouse->packet_index = 0;
    return mouse_apply_packet(mouse);
}

void mouse_driver_initialize(mouse_driver_state_t* mouse) {
    u8 config;

    if (mouse == NULL) {
        return;
    }

    memset(mouse, 0, sizeof(*mouse));
    mouse->x = display_driver_width() / 2u;
    mouse->y = display_driver_height() / 2u;

    if (!ps2_wait_write_ready()) {
        return;
    }
    outb(0x64, 0xA8);

    if (!ps2_wait_write_ready()) {
        return;
    }
    outb(0x64, 0x20);

    if (!ps2_wait_read_ready()) {
        return;
    }
    config = inb(0x60);
    config = (u8)((config | 0x02u) & (u8)~0x20u);

    if (!ps2_wait_write_ready()) {
        return;
    }
    outb(0x64, 0x60);

    if (!ps2_wait_write_ready()) {
        return;
    }
    outb(0x60, config);

    if (!mouse_write_and_ack(0xF6u)) {
        return;
    }
    if (!mouse_write_and_ack(0xF4u)) {
        return;
    }

    mouse->initialized = true;
}

bool mouse_driver_poll(mouse_driver_state_t* mouse) {
    bool changed = false;

    while ((inb(0x64) & 0x01u) != 0) {
        u8 status = inb(0x64);

        if ((status & 0x20u) == 0) {
            break;
        }

        if (mouse_handle_byte(mouse, inb(0x60))) {
            changed = true;
        }
    }

    return changed;
}
