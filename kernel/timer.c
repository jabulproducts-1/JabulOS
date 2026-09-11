#include "jabulos.h"

#define PIT_INPUT_HZ 1193182u
#define PIT_CYCLES_PER_MS 1193u
#define PIT_CHANNEL0_RELOAD 65535u

static bool g_timer_initialized = false;
static u32 g_timer_last_counter = PIT_CHANNEL0_RELOAD;
static u32 g_timer_cycle_remainder = 0u;
static u64 g_timer_ms = 0u;

static u32 pit_read_channel0_counter(void) {
    u8 low;
    u8 high;
    u32 value;

    outb(0x43, 0x00);
    low = inb(0x40);
    high = inb(0x40);
    value = (u32)low | ((u32)high << 8);
    return value == 0u ? PIT_CHANNEL0_RELOAD : value;
}

static void timer_update_locked(void) {
    u32 current_counter;
    u32 elapsed_cycles;
    u32 total_cycles;

    if (!g_timer_initialized) {
        return;
    }

    current_counter = pit_read_channel0_counter();
    if (current_counter <= g_timer_last_counter) {
        elapsed_cycles = g_timer_last_counter - current_counter;
    } else {
        elapsed_cycles = g_timer_last_counter + (PIT_CHANNEL0_RELOAD - current_counter);
    }

    g_timer_last_counter = current_counter;
    total_cycles = g_timer_cycle_remainder + elapsed_cycles;
    g_timer_ms += (u64)(total_cycles / PIT_CYCLES_PER_MS);
    g_timer_cycle_remainder = total_cycles % PIT_CYCLES_PER_MS;
}

void timer_initialize(void) {
    outb(0x43, 0x34);
    outb(0x40, (u8)(PIT_CHANNEL0_RELOAD & 0xFFu));
    outb(0x40, (u8)((PIT_CHANNEL0_RELOAD >> 8) & 0xFFu));

    g_timer_ms = 0u;
    g_timer_cycle_remainder = 0u;
    g_timer_last_counter = pit_read_channel0_counter();
    g_timer_initialized = true;

    serial_write("[boot] timer initialized\n");
}

u64 timer_ticks_ms(void) {
    timer_update_locked();
    return g_timer_ms;
}

void timer_sleep_ms(u32 milliseconds) {
    u64 target_ms;

    if (!g_timer_initialized || milliseconds == 0u) {
        return;
    }

    target_ms = timer_ticks_ms() + (u64)milliseconds;
    while (timer_ticks_ms() < target_ms) {
        asm volatile ("pause");
    }
}
