#include "jabulos.h"

typedef struct {
    u8 seconds;
    u8 minutes;
    u8 hours;
    u8 day;
    u8 month;
    u8 year;
    u8 day_of_week;
} rtc_time_t;

static u8 cmos_read(u8 reg) {
    outb(0x70, reg);
    io_wait();
    return inb(0x71);
}

static bool rtc_updating(void) {
    return (cmos_read(0x0A) & 0x80) != 0;
}

static u8 bcd_to_binary(u8 value) {
    return (u8)((value & 0x0F) + ((value / 16) * 10));
}

static void rtc_read_full(rtc_time_t* out_time) {
    u8 status_b;

    if (out_time == NULL) {
        return;
    }

    while (rtc_updating()) {
    }

    out_time->seconds = cmos_read(0x00);
    out_time->minutes = cmos_read(0x02);
    out_time->hours = cmos_read(0x04);
    out_time->day = cmos_read(0x07);
    out_time->month = cmos_read(0x08);
    out_time->year = cmos_read(0x09);
    out_time->day_of_week = cmos_read(0x06);
    status_b = cmos_read(0x0B);

    if ((status_b & 0x04) == 0) {
        out_time->seconds = bcd_to_binary(out_time->seconds);
        out_time->minutes = bcd_to_binary(out_time->minutes);
        out_time->hours = bcd_to_binary((u8)(out_time->hours & 0x7F));
        out_time->day = bcd_to_binary(out_time->day);
        out_time->month = bcd_to_binary(out_time->month);
        out_time->year = bcd_to_binary(out_time->year);
        out_time->day_of_week = bcd_to_binary(out_time->day_of_week);
    }

    if ((status_b & 0x02) == 0 && (out_time->hours & 0x80) != 0) {
        out_time->hours = (u8)(((out_time->hours & 0x7F) + 12) % 24);
    }
}

static void rtc_read_time(rtc_time_t* out_time) {
    rtc_read_full(out_time);
}

void rtc_read_time_string(char* out_buffer) {
    rtc_time_t time;
    rtc_read_time(&time);
    out_buffer[0] = (char)('0' + (time.hours / 10));
    out_buffer[1] = (char)('0' + (time.hours % 10));
    out_buffer[2] = ':';
    out_buffer[3] = (char)('0' + (time.minutes / 10));
    out_buffer[4] = (char)('0' + (time.minutes % 10));
    out_buffer[5] = ':';
    out_buffer[6] = (char)('0' + (time.seconds / 10));
    out_buffer[7] = (char)('0' + (time.seconds % 10));
    out_buffer[8] = '\0';
}

void rtc_read_date_string(char* out_buffer) {
    static const char* days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    static const char* months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    
    rtc_time_t time;
    rtc_read_full(&time);
    
    // Day of week (1-7 in CMOS usually, but can vary)
    u8 dow = (time.day_of_week > 0 && time.day_of_week <= 7) ? (time.day_of_week - 1) : 0;
    u8 month = (time.month > 0 && time.month <= 12) ? (time.month - 1) : 0;
    
    strcpy(out_buffer, days[dow]);
    strcat(out_buffer, ", ");
    strcat(out_buffer, months[month]);
    strcat(out_buffer, " ");
    
    char day_str[4];
    day_str[0] = (char)('0' + (time.day / 10));
    day_str[1] = (char)('0' + (time.day % 10));
    day_str[2] = '\0';
    if (day_str[0] == '0') {
        strcat(out_buffer, day_str + 1);
    } else {
        strcat(out_buffer, day_str);
    }
}

u32 rtc_read_seconds_of_day(void) {
    rtc_time_t time;

    rtc_read_time(&time);
    return (u32)time.hours * 3600u + (u32)time.minutes * 60u + (u32)time.seconds;
}
