#include "jabulos.h"

typedef enum {
    APP_START = 0,
    APP_SETTINGS = 1,
    APP_TERMINAL = 2,
    APP_FILES = 3,
    APP_TASKS = 4
} dock_app_t;

typedef enum {
    WINDOW_NONE = 0,
    WINDOW_SEARCH = 1,
    WINDOW_SETTINGS = 2,
    WINDOW_TERMINAL = 3,
    WINDOW_FILES = 4,
    WINDOW_TASKS = 5,
    WINDOW_WELCOME = 6
} window_id_t;

typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} window_rect_t;

typedef struct {
    bool show_search;
    bool show_settings;
    bool show_terminal;
    bool show_files;
    bool show_tasks;
    bool show_welcome;
    bool minimized_search;
    bool minimized_settings;
    bool minimized_terminal;
    bool minimized_files;
    bool minimized_tasks;
    bool minimized_welcome;
    bool shaded_search;
    bool shaded_settings;
    bool shaded_terminal;
    bool shaded_files;
    bool shaded_tasks;
    bool shaded_welcome;
    window_rect_t search_rect;
    window_rect_t settings_rect;
    window_rect_t terminal_rect;
    window_rect_t files_rect;
    window_rect_t tasks_rect;
    window_rect_t welcome_rect;
    window_id_t dragging_window;
    s32 drag_offset_x;
    s32 drag_offset_y;
    char launcher_query[32];
    bool oobe_storage_ready;
    bool oobe_disk_has_install;
    bool oobe_install_failed;
    bool oobe_install_complete;
    u32 oobe_stage;
    u32 desktop_theme;
    u32 window_theme;
    bool transparency_enabled;
    bool live_media_boot;
    u32 setup_phase;
    u32 setup_phase_started_second;
    u32 settings_page;
    ata_device_info_t storage_info;
    u32 install_target_index;
} desktop_state_t;

typedef struct {
    u32 x;
    u32 y;
    bool initialized;
    bool left_down;
    u8 packet[3];
    u8 packet_index;
} mouse_state_t;

typedef struct {
    u32 dock_x;
    u32 dock_y;
    u32 dock_width;
    u32 dock_height;
    u32 icon_size;
} dock_layout_t;

#define BOOT_LOG_MAX_LINES 24u
#define BOOT_LOG_LINE_LENGTH 72u
#define CURSOR_WIDTH 12u
#define CURSOR_HEIGHT 23u
#define TOP_BAR_HEIGHT 40u
#define WINDOW_CORNER_RADIUS 20u
#define WINDOW_BUTTON_SIZE 10u
#define WINDOW_TITLEBAR_HEIGHT 38u
#define LAUNCHER_APP_COUNT 4u
#define OOBE_BUTTON_WIDTH 144u
#define OOBE_BUTTON_HEIGHT 34u
#define SETUP_INSTALL_SECONDS (15u * 60u)
#define SETUP_CUSTOMIZE_SECONDS 10u

static const char* PREVIEW_LABEL_TEXT = " JabulOS Aero shell ";
static const char* g_launcher_app_names[LAUNCHER_APP_COUNT] = {
    "Settings",
    "Terminal",
    "File Manager",
    "File Manager",
    "Task Manager"
};
static const window_id_t g_launcher_window_ids[LAUNCHER_APP_COUNT] = {
    WINDOW_SETTINGS,
    WINDOW_TERMINAL,
    WINDOW_FILES,
    WINDOW_TASKS
};
static const dock_app_t g_launcher_dock_apps[LAUNCHER_APP_COUNT] = {
    APP_SETTINGS,
    APP_TERMINAL,
    APP_FILES,
    APP_TASKS
};

static u32 window_title_height(window_id_t window);
static void draw_oobe_button(u32 x, u32 y, u32 width, const char* label, bool primary);
static void launcher_tile_rect(const window_rect_t* rect, u32 index, u32* out_x, u32* out_y, u32* out_width, u32* out_height);
static void launcher_recommended_rect(const window_rect_t* rect,
                                      u32 index,
                                      u32* out_x,
                                      u32* out_y,
                                      u32* out_width,
                                      u32* out_height);

static char g_boot_log_lines[BOOT_LOG_MAX_LINES][BOOT_LOG_LINE_LENGTH];
static u32 g_boot_log_count = 0;

typedef enum {
    WINDOW_BUTTON_NONE = 0,
    WINDOW_BUTTON_CLOSE = 1,
    WINDOW_BUTTON_MINIMIZE = 2,
    WINDOW_BUTTON_HIDE = 3
} window_button_t;

typedef enum {
    OOBE_STAGE_INTRO = 0,
    OOBE_STAGE_CONFIRM = 1,
    OOBE_STAGE_COMPLETE = 2,
    OOBE_STAGE_ERROR = 3,
    OOBE_STAGE_STORAGE_MISSING = 4
} oobe_stage_t;

typedef enum {
    OOBE_BUTTON_NONE = 0,
    OOBE_BUTTON_PRIMARY = 1,
    OOBE_BUTTON_SECONDARY = 2
} oobe_button_t;

typedef enum {
    DESKTOP_THEME_GRADIENT = 0,
    DESKTOP_THEME_SOLID = 1,
    DESKTOP_THEME_WINDOWS = 2,
    DESKTOP_THEME_SUNSET = 3,
    DESKTOP_THEME_COUNT = 4
} desktop_theme_t;

typedef enum {
    WINDOW_THEME_WINDOWS = 0,
    WINDOW_THEME_MAC = 1,
    WINDOW_THEME_COUNT = 2
} chrome_theme_t;

typedef enum {
    SETUP_PHASE_NONE = 0,
    SETUP_PHASE_INSTALLER = 1,
    SETUP_PHASE_INSTALLING = 2,
    SETUP_PHASE_CUSTOMIZING = 3,
    SETUP_PHASE_USER_OOBE = 4
} setup_phase_t;

typedef enum {
    SETTINGS_HIT_NONE = 0,
    SETTINGS_HIT_TAB_PERSONALIZE = 1,
    SETTINGS_HIT_TAB_ABOUT = 2,
    SETTINGS_HIT_DESKTOP_THEME = 3,
    SETTINGS_HIT_WINDOW_THEME = 4,
    SETTINGS_HIT_TRANSPARENCY = 5
} settings_hit_t;

typedef enum {
    SETTINGS_PAGE_PERSONALIZE = 0,
    SETTINGS_PAGE_ABOUT = 1
} settings_page_t;

static const char* desktop_theme_name(u32 theme) {
    switch ((desktop_theme_t)theme) {
        case DESKTOP_THEME_GRADIENT:
            return "Gradient";
        case DESKTOP_THEME_SOLID:
            return "Pure color";
        case DESKTOP_THEME_WINDOWS:
            return "Windows inspired";
        case DESKTOP_THEME_SUNSET:
            return "Sunset glow";
        default:
            return "Gradient";
    }
}

static const char* window_theme_name(u32 theme) {
    switch ((chrome_theme_t)theme) {
        case WINDOW_THEME_WINDOWS:
            return "Default";
        case WINDOW_THEME_MAC:
            return "Light";
        default:
            return "Windows";
    }
}

static u32 seconds_since(u32 earlier, u32 current) {
    if (current >= earlier) {
        return current - earlier;
    }
    return (24u * 60u * 60u - earlier) + current;
}

static void fill_fallback_wallpaper(void) {
    u32 width = vga_width();
    u32 height = vga_height();

    for (u32 y = 0; y < height; ++y) {
        u8 red = (u8)(8 + (y * 18) / (height ? height : 1));
        u8 green = (u8)(14 + (y * 28) / (height ? height : 1));
        u8 blue = (u8)(22 + (y * 54) / (height ? height : 1));
        draw_rect(0, y, width, 1, vga_color(red, green, blue));
    }

    vga_fill_rect_alpha(0, 0, width, height, 28, 52, 92, 60);
    vga_fill_rect_alpha(0, height / 3u, width, height / 2u, 12, 16, 28, 74);
}

static void fill_solid_wallpaper(u8 red, u8 green, u8 blue) {
    draw_rect(0, 0, vga_width(), vga_height(), vga_color(red, green, blue));
}

static void fill_windows_wallpaper(void) {
    u32 width = vga_width();
    u32 height = vga_height();

    for (u32 y = 0; y < height; ++y) {
        u8 red = (u8)(34 + (y * 18u) / (height ? height : 1u));
        u8 green = (u8)(88 + (y * 36u) / (height ? height : 1u));
        u8 blue = (u8)(150 + (y * 50u) / (height ? height : 1u));
        draw_rect(0, y, width, 1, vga_color(red, green, blue));
    }

    vga_fill_rect_alpha(width / 5u, height / 6u, width / 2u, height / 2u, 255, 255, 255, 22);
    vga_fill_rect_alpha(width / 2u, height / 8u, width / 3u, (height * 3u) / 5u, 170, 220, 255, 26);
    vga_fill_rect_alpha(width / 3u, height / 2u, width / 2u, height / 3u, 70, 144, 255, 24);
}

static void fill_sunset_wallpaper(void) {
    u32 width = vga_width();
    u32 height = vga_height();

    for (u32 y = 0; y < height; ++y) {
        u8 red = (u8)(52 + (y * 110u) / (height ? height : 1u));
        u8 green = (u8)(28 + (y * 44u) / (height ? height : 1u));
        u8 blue = (u8)(72 + (y * 28u) / (height ? height : 1u));
        draw_rect(0, y, width, 1, vga_color(red, green, blue));
    }

    vga_fill_rect_alpha(width / 4u, height / 5u, width / 2u, height / 3u, 255, 210, 128, 26);
    vga_fill_rect_alpha(0, height / 2u, width, height / 2u, 24, 18, 36, 84);
}

static void draw_desktop_background(const desktop_state_t* state, const image_t* wallpaper) {
    u32 theme = state != NULL ? state->desktop_theme : DESKTOP_THEME_GRADIENT;

    switch ((desktop_theme_t)theme) {
        case DESKTOP_THEME_SOLID:
            fill_solid_wallpaper(34, 54, 84);
            break;
        case DESKTOP_THEME_WINDOWS:
            if (wallpaper != NULL) {
                image_blit_stretch(wallpaper);
                vga_fill_rect_alpha(0, 0, vga_width(), vga_height(), 18, 30, 60, 52);
            } else {
                fill_windows_wallpaper();
            }
            break;
        case DESKTOP_THEME_SUNSET:
            fill_sunset_wallpaper();
            break;
        case DESKTOP_THEME_GRADIENT:
        default:
            fill_fallback_wallpaper();
            break;
    }
}

static u32 text_width(const char* text) {
    return (u32)strlen(text) * 8u;
}

static void draw_text_centered(u32 y, const char* text, u32 color) {
    u32 width = text_width(text);
    u32 screen_width = vga_width();
    u32 x = (screen_width > width) ? (screen_width - width) / 2u : 0;
    draw_text(x, y, text, color);
}

static void append_text(char* buffer, const char* text) {
    while (*text != '\0') {
        *buffer = *text;
        ++buffer;
        ++text;
    }
    *buffer = '\0';
}

static void append_uint(char* buffer, u32 value) {
    char digits[11];
    u32 count = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (value != 0) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    for (u32 index = 0; index < count; ++index) {
        buffer[index] = digits[count - index - 1u];
    }
    buffer[count] = '\0';
}

static void format_megabytes(char* buffer, u64 bytes) {
    char number[16];
    append_uint(number, (u32)(bytes / (1024u * 1024u)));
    buffer[0] = '\0';
    append_text(buffer, number);
    append_text(buffer + strlen(buffer), " MB");
}

static void format_storage_size(char* buffer, u32 sector_count) {
    u64 bytes = (u64)sector_count * 512u;
    u64 gib = bytes / (1024u * 1024u * 1024u);
    u64 mib = bytes / (1024u * 1024u);
    char number[16];

    if (buffer == NULL) {
        return;
    }

    buffer[0] = '\0';
    if (gib > 0u) {
        append_uint(number, (u32)gib);
        append_text(buffer, number);
        append_text(buffer + strlen(buffer), " GB");
    } else {
        append_uint(number, (u32)mib);
        append_text(buffer, number);
        append_text(buffer + strlen(buffer), " MB");
    }
}

static void format_display_mode(char* buffer) {
    char number[16];

    if (buffer == NULL) {
        return;
    }

    buffer[0] = '\0';
    append_uint(number, vga_width());
    append_text(buffer, number);
    append_text(buffer + strlen(buffer), "x");
    append_uint(number, vga_height());
    append_text(buffer + strlen(buffer), number);
    append_text(buffer + strlen(buffer), "x");
    append_uint(number, 32u);
    append_text(buffer + strlen(buffer), number);
}

static void copy_boot_log_line(char* destination, const char* source) {
    u32 index = 0;

    if (destination == NULL) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    while (source[index] != '\0' && source[index] != '\n' && index + 1u < BOOT_LOG_LINE_LENGTH) {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static char to_lower_ascii(char value) {
    if (value >= 'A' && value <= 'Z') {
        return (char)(value - 'A' + 'a');
    }
    return value;
}

static bool point_in_rect(u32 x, u32 y, u32 rect_x, u32 rect_y, u32 rect_width, u32 rect_height) {
    return x >= rect_x && x < rect_x + rect_width && y >= rect_y && y < rect_y + rect_height;
}

static bool string_contains_case_insensitive(const char* text, const char* query) {
    u32 query_length = (u32)strlen(query);

    if (query_length == 0u) {
        return true;
    }

    for (u32 start = 0; text[start] != '\0'; ++start) {
        u32 index = 0;

        while (query[index] != '\0' &&
               text[start + index] != '\0' &&
               to_lower_ascii(text[start + index]) == to_lower_ascii(query[index])) {
            ++index;
        }

        if (index == query_length) {
            return true;
        }
    }

    return false;
}

static u32 install_target_count(const desktop_state_t* state) {
    u32 count = 0;

    if (state == NULL || !state->storage_info.present) {
        return 0u;
    }

    for (u32 index = 0; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
        if (state->storage_info.partitions[index].present) {
            ++count;
        }
    }

    return count == 0u ? 1u : count;
}

static bool get_install_target(const desktop_state_t* state,
                               u32 target_index,
                               u32* out_lba,
                               u32* out_sector_count,
                               u32* out_partition_number,
                               bool* out_whole_disk) {
    u32 visible_index = 0;

    if (state == NULL || !state->storage_info.present) {
        return false;
    }

    for (u32 index = 0; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
        const ata_partition_info_t* partition = &state->storage_info.partitions[index];

        if (!partition->present) {
            continue;
        }

        if (visible_index == target_index) {
            if (out_lba != NULL) {
                *out_lba = partition->start_lba;
            }
            if (out_sector_count != NULL) {
                *out_sector_count = partition->sector_count;
            }
            if (out_partition_number != NULL) {
                *out_partition_number = index + 1u;
            }
            if (out_whole_disk != NULL) {
                *out_whole_disk = false;
            }
            return true;
        }

        ++visible_index;
    }

    if (visible_index == 0u && target_index == 0u) {
        if (out_lba != NULL) {
            *out_lba = 0u;
        }
        if (out_sector_count != NULL) {
            *out_sector_count = state->storage_info.total_sectors;
        }
        if (out_partition_number != NULL) {
            *out_partition_number = 0u;
        }
        if (out_whole_disk != NULL) {
            *out_whole_disk = true;
        }
        return true;
    }

    return false;
}

static u32 install_required_sectors(const boot_info_t* boot_info) {
    u32 kernel_size;
    u32 initrd_size;

    if (boot_info == NULL) {
        return 0u;
    }

    kernel_size = (u32)(boot_info->kernel_end - boot_info->kernel_start);
    initrd_size = (u32)boot_info->initrd_size;
    return 1u + ((kernel_size + 511u) / 512u) + ((initrd_size + 511u) / 512u);
}

static bool install_target_can_fit(const desktop_state_t* state, const boot_info_t* boot_info, u32 target_index) {
    u32 target_sectors;

    if (!get_install_target(state, target_index, NULL, &target_sectors, NULL, NULL)) {
        return false;
    }

    return target_sectors == 0u || target_sectors >= install_required_sectors(boot_info);
}

static void format_install_target_name(const desktop_state_t* state, u32 target_index, char* out_label, char* out_size) {
    u32 target_sectors;
    u32 partition_number;
    bool whole_disk;

    if (out_label != NULL) {
        out_label[0] = '\0';
    }
    if (out_size != NULL) {
        out_size[0] = '\0';
    }
    if (state == NULL || !get_install_target(state, target_index, NULL, &target_sectors, &partition_number, &whole_disk)) {
        return;
    }

    if (out_label != NULL) {
        if (whole_disk) {
            strcpy(out_label, "ATA0 whole disk");
        } else {
            strcpy(out_label, "Partition ");
            append_uint(out_label + strlen(out_label), partition_number);
        }
    }
    if (out_size != NULL) {
        format_storage_size(out_size, target_sectors);
    }
}

static bool* get_window_open_flag(desktop_state_t* state, window_id_t window) {
    if (state == NULL) {
        return NULL;
    }

    switch (window) {
        case WINDOW_SEARCH:
            return &state->show_search;
        case WINDOW_SETTINGS:
            return &state->show_settings;
        case WINDOW_TERMINAL:
            return &state->show_terminal;
        case WINDOW_FILES:
            return &state->show_files;
        case WINDOW_TASKS:
            return &state->show_tasks;
        case WINDOW_WELCOME:
            return &state->show_welcome;
        default:
            return NULL;
    }
}

static bool* get_window_minimized_flag(desktop_state_t* state, window_id_t window) {
    if (state == NULL) {
        return NULL;
    }

    switch (window) {
        case WINDOW_SEARCH:
            return &state->minimized_search;
        case WINDOW_SETTINGS:
            return &state->minimized_settings;
        case WINDOW_TERMINAL:
            return &state->minimized_terminal;
        case WINDOW_FILES:
            return &state->minimized_files;
        case WINDOW_TASKS:
            return &state->minimized_tasks;
        case WINDOW_WELCOME:
            return &state->minimized_welcome;
        default:
            return NULL;
    }
}

static bool* get_window_shaded_flag(desktop_state_t* state, window_id_t window) {
    if (state == NULL) {
        return NULL;
    }

    switch (window) {
        case WINDOW_SEARCH:
            return &state->shaded_search;
        case WINDOW_SETTINGS:
            return &state->shaded_settings;
        case WINDOW_TERMINAL:
            return &state->shaded_terminal;
        case WINDOW_FILES:
            return &state->shaded_files;
        case WINDOW_TASKS:
            return &state->shaded_tasks;
        case WINDOW_WELCOME:
            return &state->shaded_welcome;
        default:
            return NULL;
    }
}

static bool window_open(const desktop_state_t* state, window_id_t window) {
    bool* flag = get_window_open_flag((desktop_state_t*)state, window);
    return flag != NULL ? *flag : false;
}

static bool window_minimized(const desktop_state_t* state, window_id_t window) {
    bool* flag = get_window_minimized_flag((desktop_state_t*)state, window);
    return flag != NULL ? *flag : false;
}

static bool window_shaded(const desktop_state_t* state, window_id_t window) {
    bool* flag = get_window_shaded_flag((desktop_state_t*)state, window);
    return flag != NULL ? *flag : false;
}

static void open_window(desktop_state_t* state, window_id_t window) {
    bool* open_flag = get_window_open_flag(state, window);
    bool* minimized_flag = get_window_minimized_flag(state, window);
    bool* shaded_flag = get_window_shaded_flag(state, window);

    if (open_flag == NULL) {
        return;
    }

    *open_flag = true;
    if (minimized_flag != NULL) {
        *minimized_flag = false;
    }
    if (shaded_flag != NULL) {
        *shaded_flag = false;
    }
}

static void close_window(desktop_state_t* state, window_id_t window) {
    bool* open_flag = get_window_open_flag(state, window);
    bool* minimized_flag = get_window_minimized_flag(state, window);
    bool* shaded_flag = get_window_shaded_flag(state, window);

    if (open_flag == NULL) {
        return;
    }

    *open_flag = false;
    if (minimized_flag != NULL) {
        *minimized_flag = false;
    }
    if (shaded_flag != NULL) {
        *shaded_flag = false;
    }
}

static void minimize_window(desktop_state_t* state, window_id_t window) {
    bool* open_flag = get_window_open_flag(state, window);
    bool* minimized_flag = get_window_minimized_flag(state, window);

    if (open_flag == NULL || minimized_flag == NULL) {
        return;
    }

    *open_flag = true;
    *minimized_flag = true;
}

static void toggle_window_shade(desktop_state_t* state, window_id_t window) {
    bool* open_flag = get_window_open_flag(state, window);
    bool* minimized_flag = get_window_minimized_flag(state, window);
    bool* shaded_flag = get_window_shaded_flag(state, window);

    if (open_flag == NULL || shaded_flag == NULL) {
        return;
    }

    *open_flag = true;
    if (minimized_flag != NULL) {
        *minimized_flag = false;
    }
    *shaded_flag = !*shaded_flag;
}

static window_id_t dock_app_to_window(dock_app_t app) {
    switch (app) {
        case APP_START:
            return WINDOW_SEARCH;
        case APP_SETTINGS:
            return WINDOW_SETTINGS;
        case APP_TERMINAL:
            return WINDOW_TERMINAL;
        case APP_FILES:
            return WINDOW_FILES;
        case APP_TASKS:
            return WINDOW_TASKS;
        default:
            return WINDOW_NONE;
    }
}

static u32 shaded_window_height(window_id_t window) {
    return window_title_height(window) + 4u;
}

static char scancode_to_ascii(u8 scancode) {
    switch (scancode) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x39: return ' ';
        default:   return '\0';
    }
}

static void draw_progress_bar(u32 x, u32 y, u32 width, u32 height, u32 percent, u32 fill_color) {
    u32 clamped = percent > 100u ? 100u : percent;
    u32 inner = (width > 8u) ? width - 8u : width;
    u32 fill = (inner * clamped) / 100u;

    draw_rounded_rect(x, y, width, height, height / 2u, vga_color(18, 22, 34));
    vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, height - 2u, 255, 255, 255, 18);
    draw_rect_outline(x, y, width, height, 1, vga_color(92, 102, 128));
    if (fill > 0u) {
        draw_rounded_rect(x + 4, y + 4, fill, height - 8u, (height - 8u) / 2u, fill_color);
        vga_fill_rect_alpha(x + 5u, y + 5u, fill > 2u ? fill - 2u : fill, height > 10u ? height - 10u : height,
                            255, 255, 255, 26);
    }
}

static void get_window_button_position(const desktop_state_t* state,
                                       const window_rect_t* rect,
                                       window_button_t button,
                                       u32* out_x,
                                       u32* out_y) {
    u32 button_y;
    u32 button_x = 0;
    bool mac_style = state != NULL && state->window_theme == WINDOW_THEME_MAC;

    if (rect == NULL || out_x == NULL || out_y == NULL || button == WINDOW_BUTTON_NONE) {
        return;
    }

    button_y = rect->y + 13u;
    if (mac_style) {
        if (button == WINDOW_BUTTON_CLOSE) {
            button_x = rect->x + 14u;
        } else if (button == WINDOW_BUTTON_MINIMIZE) {
            button_x = rect->x + 30u;
        } else {
            button_x = rect->x + 46u;
        }
    } else {
        if (button == WINDOW_BUTTON_CLOSE) {
            button_x = rect->x + rect->width - 20u;
        } else if (button == WINDOW_BUTTON_MINIMIZE) {
            button_x = rect->x + rect->width - 36u;
        } else {
            button_x = rect->x + rect->width - 52u;
        }
    }

    *out_x = button_x;
    *out_y = button_y;
}

static window_button_t hit_test_window_button(const desktop_state_t* state,
                                              const window_rect_t* rect,
                                              u32 mouse_x,
                                              u32 mouse_y) {
    u32 button_y;
    u32 button_x;

    if (rect == NULL) {
        return WINDOW_BUTTON_NONE;
    }

    get_window_button_position(state, rect, WINDOW_BUTTON_CLOSE, &button_x, &button_y);
    if (point_in_rect(mouse_x, mouse_y, button_x, button_y, WINDOW_BUTTON_SIZE, WINDOW_BUTTON_SIZE)) {
        return WINDOW_BUTTON_CLOSE;
    }
    get_window_button_position(state, rect, WINDOW_BUTTON_MINIMIZE, &button_x, &button_y);
    if (point_in_rect(mouse_x, mouse_y, button_x, button_y, WINDOW_BUTTON_SIZE, WINDOW_BUTTON_SIZE)) {
        return WINDOW_BUTTON_MINIMIZE;
    }
    get_window_button_position(state, rect, WINDOW_BUTTON_HIDE, &button_x, &button_y);
    if (point_in_rect(mouse_x, mouse_y, button_x, button_y, WINDOW_BUTTON_SIZE, WINDOW_BUTTON_SIZE)) {
        return WINDOW_BUTTON_HIDE;
    }

    return WINDOW_BUTTON_NONE;
}

static void draw_window_button(u32 x, u32 y, u32 base_color, u32 glyph_color, window_button_t button) {
    draw_rounded_rect(x, y, WINDOW_BUTTON_SIZE, WINDOW_BUTTON_SIZE, 4u, base_color);
    vga_fill_rect_alpha(x + 1u, y + 1u, WINDOW_BUTTON_SIZE - 2u, 3u, 255, 255, 255, 54);

    if (button == WINDOW_BUTTON_CLOSE) {
        draw_rect(x + 2u, y + 2u, 1u, 1u, glyph_color);
        draw_rect(x + 7u, y + 2u, 1u, 1u, glyph_color);
        draw_rect(x + 4u, y + 4u, 1u, 1u, glyph_color);
        draw_rect(x + 2u, y + 7u, 1u, 1u, glyph_color);
        draw_rect(x + 7u, y + 7u, 1u, 1u, glyph_color);
        draw_rect(x + 3u, y + 3u, 1u, 1u, glyph_color);
        draw_rect(x + 6u, y + 3u, 1u, 1u, glyph_color);
        draw_rect(x + 3u, y + 6u, 1u, 1u, glyph_color);
        draw_rect(x + 6u, y + 6u, 1u, 1u, glyph_color);
    } else if (button == WINDOW_BUTTON_MINIMIZE) {
        draw_rect(x + 2u, y + 6u, 6u, 1u, glyph_color);
    } else if (button == WINDOW_BUTTON_HIDE) {
        draw_rect_outline(x + 2u, y + 2u, 6u, 6u, 1u, glyph_color);
    }
}

static void draw_window_frame(const desktop_state_t* state,
                              u32 x,
                              u32 y,
                              u32 width,
                              u32 height,
                              const char* title,
                              u32 body_tint) {
    u32 title_height = window_title_height(WINDOW_NONE);
    u32 frame = state != NULL && state->window_theme == WINDOW_THEME_MAC ? vga_color(228, 232, 238) : vga_color(14, 18, 28);
    u32 stroke = state != NULL && state->window_theme == WINDOW_THEME_MAC ? vga_color(168, 176, 188) : vga_color(98, 116, 148);
    u32 title_text = state != NULL && state->window_theme == WINDOW_THEME_MAC ? vga_color(44, 48, 58) : vga_color(244, 247, 255);
    u32 glyph = vga_color(255, 255, 255);
    u8 alpha = (state != NULL && state->transparency_enabled) ? 164u : 255u;
    u32 button_x;
    u32 button_y;
    bool mac_style = state != NULL && state->window_theme == WINDOW_THEME_MAC;

    vga_fill_rect_alpha(x - 10u, y + 10u, width + 20u, height + 18u, 0, 0, 0, 42);
    draw_rounded_rect(x, y, width, height, WINDOW_CORNER_RADIUS, frame);
    if (mac_style) {
        vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, height - 2u, 250, 252, 255, alpha);
        vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, title_height, 246, 247, 249, state != NULL && state->transparency_enabled ? 228u : 255u);
    } else {
        vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, height - 2u, 16, 20, 32, alpha);
        vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, title_height, 30, 40, 64, state != NULL && state->transparency_enabled ? 210u : 255u);
    }
    vga_fill_rect_alpha(x + 12u, y + 8u, width - 24u, 10u, 255, 255, 255, mac_style ? 18u : 28u);
    draw_rect_outline(x, y, width, height, 1, stroke);
    draw_rect(x + 1u, y + title_height, width - 2u, 1u, body_tint);
    if (!mac_style) {
        vga_fill_rect_alpha(x + 2u, y + title_height + 1u, width - 4u, height - title_height - 3u,
                            255, 255, 255, 12);
    }

    get_window_button_position(state, &(window_rect_t){x, y, width, height}, WINDOW_BUTTON_CLOSE, &button_x, &button_y);
    draw_window_button(button_x, button_y, vga_color(236, 92, 92), glyph, WINDOW_BUTTON_CLOSE);
    get_window_button_position(state, &(window_rect_t){x, y, width, height}, WINDOW_BUTTON_MINIMIZE, &button_x, &button_y);
    draw_window_button(button_x, button_y, vga_color(255, 196, 84), glyph, WINDOW_BUTTON_MINIMIZE);
    get_window_button_position(state, &(window_rect_t){x, y, width, height}, WINDOW_BUTTON_HIDE, &button_x, &button_y);
    draw_window_button(button_x, button_y, vga_color(80, 210, 136), glyph, WINDOW_BUTTON_HIDE);
    draw_text(mac_style ? x + (width / 2u > text_width(title) / 2u ? (width / 2u - text_width(title) / 2u) : 68u) : x + 18u,
              y + 15u,
              title,
              title_text);
}

static void draw_top_bar(void) {
    char clock_buffer[9];
    u32 width = vga_width();
    u32 text = vga_color(230, 235, 246);
    u32 bar_height = TOP_BAR_HEIGHT;

    // Glass effect background matching gui.cpp
    vga_fill_rect_alpha(0, 0, width, bar_height, 22, 24, 34, 160);
    
    // Depth highlights
    vga_fill_rect_alpha(0, 0, width, 1u, 255, 255, 255, 60);
    vga_fill_rect_alpha(0, bar_height - 1u, width, 1u, 0, 0, 0, 48);
    
    rtc_read_time_string(clock_buffer);
    draw_text(18, (bar_height / 2u) - 6u, "JabulOS 64 bit", text);
    draw_text(width - 90, (bar_height / 2u) - 6u, clock_buffer, text);
}

static void draw_preview_label(void) {
    draw_text(30, vga_height() - 90u, PREVIEW_LABEL_TEXT, vga_color(214, 222, 236));
}

static bool launcher_result_at(const desktop_state_t* state,
                               const window_rect_t* rect,
                               u32 mouse_x,
                               u32 mouse_y,
                               u32* out_index) {
    bool searching;
    u32 visible_row = 0;

    if (state == NULL || rect == NULL) {
        return false;
    }

    searching = state->launcher_query[0] != '\0';
    if (!searching) {
        for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
            u32 tile_x;
            u32 tile_y;
            u32 tile_width;
            u32 tile_height;
            u32 row_x;
            u32 row_y;
            u32 row_width;
            u32 row_height;

            launcher_tile_rect(rect, index, &tile_x, &tile_y, &tile_width, &tile_height);
            if (point_in_rect(mouse_x, mouse_y, tile_x, tile_y, tile_width, tile_height)) {
                if (out_index != NULL) {
                    *out_index = index;
                }
                return true;
            }

            launcher_recommended_rect(rect, index, &row_x, &row_y, &row_width, &row_height);
            if (point_in_rect(mouse_x, mouse_y, row_x, row_y, row_width, row_height)) {
                if (out_index != NULL) {
                    *out_index = index;
                }
                return true;
            }
        }

        return false;
    }

    for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
        u32 row_y;

        if (!string_contains_case_insensitive(g_launcher_app_names[index], state->launcher_query)) {
            continue;
        }

        row_y = rect->y + 136u + visible_row * 44u;
        if (point_in_rect(mouse_x, mouse_y, rect->x + 22u, row_y, rect->width - 44u, 36u)) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return true;
        }
        ++visible_row;
    }

    return false;
}

static void draw_app_icon(u32 x, u32 y, u32 size, dock_app_t app) {
    u32 bg = vga_color(36, 44, 62);
    u32 dark = vga_color(232, 238, 246);
    u32 blue = vga_color(98, 168, 255);
    u32 gold = vga_color(255, 196, 92);
    u32 mint = vga_color(104, 224, 172);
    u32 purple = vga_color(198, 156, 255);

    draw_rounded_rect(x, y, size, size, 10, bg);
    vga_fill_rect_alpha(x + 1u, y + 1u, size - 2u, 8u, 255, 255, 255, 18);
    draw_rect_outline(x, y, size, size, 1, vga_color(92, 104, 128));

    if (app == APP_START) {
        draw_rect(x + 8, y + 8, 8, 8, blue);
        draw_rect(x + 18, y + 8, 8, 8, mint);
        draw_rect(x + 8, y + 18, 8, 8, gold);
        draw_rect(x + 18, y + 18, 8, 8, purple);
    } else if (app == APP_SETTINGS) {
        draw_rounded_rect(x + 9, y + 9, 16, 16, 8, dark);
        draw_rect(x + 6, y + 15, 4, 4, blue);
        draw_rect(x + 24, y + 15, 4, 4, blue);
        draw_rect(x + 15, y + 6, 4, 4, blue);
        draw_rect(x + 15, y + 24, 4, 4, blue);
        draw_rect(x + 14, y + 14, 6, 6, bg);
    } else if (app == APP_TERMINAL) {
        draw_rounded_rect(x + 6, y + 8, 22, 18, 4, dark);
        draw_rect(x + 10, y + 13, 6, 2, mint);
        draw_rect(x + 14, y + 15, 2, 2, mint);
        draw_rect(x + 18, y + 19, 6, 2, vga_color(220, 225, 232));
        draw_rect(x + 10, y + 20, 4, 2, blue);
    } else if (app == APP_FILES) {
        draw_rounded_rect(x + 5, y + 12, 24, 15, 5, gold);
        draw_rect(x + 9, y + 8, 10, 6, vga_color(255, 216, 120));
        draw_rect(x + 8, y + 16, 18, 2, vga_color(210, 150, 40));
    } else if (app == APP_TASKS) {
        draw_rect(x + 8, y + 22, 4, 6, purple);
        draw_rect(x + 14, y + 17, 4, 11, blue);
        draw_rect(x + 20, y + 12, 4, 16, mint);
        draw_rect(x + 26, y + 9, 4, 19, gold);
        draw_rect(x + 8, y + 28, 20, 2, dark);
    }
}

static void get_dock_layout(dock_layout_t* out_layout) {
    if (out_layout == NULL) {
        return;
    }

    out_layout->dock_width = 430u;
    out_layout->dock_height = 58u;
    out_layout->icon_size = 30u;
    out_layout->dock_x = (vga_width() > out_layout->dock_width) ? (vga_width() - out_layout->dock_width) / 2u : 0u;
    out_layout->dock_y = (vga_height() > out_layout->dock_height + 16u) ?
                         (vga_height() - out_layout->dock_height - 16u) : 0u;
}

static u32 window_title_height(window_id_t window) {
    (void)window;
    return WINDOW_TITLEBAR_HEIGHT;
}

static bool point_in_window_title(u32 x, u32 y, const window_rect_t* rect, window_id_t window) {
    return rect != NULL &&
           x >= rect->x && x < rect->x + rect->width &&
           y >= rect->y && y < rect->y + window_title_height(window);
}

static void clamp_window_rect(window_rect_t* rect) {
    u32 min_x = 12u;
    u32 min_y = 40u;
    u32 max_x;
    u32 max_y;

    if (rect == NULL) {
        return;
    }

    max_x = (vga_width() > rect->width + 12u) ? (vga_width() - rect->width - 12u) : min_x;
    max_y = (vga_height() > rect->height + 88u) ? (vga_height() - rect->height - 88u) : min_y;

    if (rect->x < min_x) {
        rect->x = min_x;
    } else if (rect->x > max_x) {
        rect->x = max_x;
    }

    if (rect->y < min_y) {
        rect->y = min_y;
    } else if (rect->y > max_y) {
        rect->y = max_y;
    }
}

static window_rect_t* get_window_rect(desktop_state_t* state, window_id_t window) {
    if (state == NULL) {
        return NULL;
    }

    switch (window) {
        case WINDOW_SEARCH:
            return &state->search_rect;
        case WINDOW_SETTINGS:
            return &state->settings_rect;
        case WINDOW_TERMINAL:
            return &state->terminal_rect;
        case WINDOW_FILES:
            return &state->files_rect;
        case WINDOW_TASKS:
            return &state->tasks_rect;
        case WINDOW_WELCOME:
            return &state->welcome_rect;
        default:
            return NULL;
    }
}

static const window_rect_t* get_window_rect_const(const desktop_state_t* state, window_id_t window) {
    return get_window_rect((desktop_state_t*)state, window);
}

static bool window_visible(const desktop_state_t* state, window_id_t window) {
    return window_open(state, window) && !window_minimized(state, window);
}

static bool dock_icon_contains(u32 mouse_x, u32 mouse_y, dock_app_t app) {
    dock_layout_t layout;
    u32 tile_x;
    u32 tile_y;
    u32 tile_width;
    u32 tile_height;

    get_dock_layout(&layout);
    tile_x = layout.dock_x + (((u32)app == 0u) ? 12u : (86u + ((u32)app - 1u) * 52u));
    tile_y = layout.dock_y + 9u;
    tile_width = ((u32)app == 0u) ? 56u : 42u;
    tile_height = 40u;

    return mouse_x >= tile_x && mouse_x < tile_x + tile_width &&
           mouse_y >= tile_y && mouse_y < tile_y + tile_height;
}

static bool dock_hit_test(u32 mouse_x, u32 mouse_y, dock_app_t* out_app) {
    for (u32 index = 0; index < 5u; ++index) {
        if (dock_icon_contains(mouse_x, mouse_y, (dock_app_t)index)) {
            if (out_app != NULL) {
                *out_app = (dock_app_t)index;
            }
            return true;
        }
    }

    return false;
}

static bool dock_app_active(const desktop_state_t* state, dock_app_t app) {
    return window_open(state, dock_app_to_window(app));
}

static bool installer_header_present(void) {
    return installer_detect_existing_install();
}

static void desktop_initialize(desktop_state_t* state, bool ata_ready, bool live_media_boot, u32 current_second) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->live_media_boot = live_media_boot;
    state->oobe_storage_ready = ata_ready;
    if (ata_ready) {
        (void)ata_read_primary_master_info(&state->storage_info);
    }
    state->oobe_disk_has_install = ata_ready && installer_header_present();
    state->oobe_stage = ata_ready ? OOBE_STAGE_INTRO : OOBE_STAGE_STORAGE_MISSING;
    state->desktop_theme = DESKTOP_THEME_WINDOWS;
    state->window_theme = WINDOW_THEME_WINDOWS;
    state->transparency_enabled = true;
    state->settings_page = SETTINGS_PAGE_PERSONALIZE;
    state->setup_phase = (live_media_boot && !state->oobe_disk_has_install) ? SETUP_PHASE_INSTALLER : SETUP_PHASE_NONE;
    state->setup_phase_started_second = current_second;
    state->show_search = false;
    state->show_settings = false;
    state->show_terminal = false;
    state->show_files = false;
    state->show_tasks = false;
    state->show_welcome = false;
    state->search_rect = (window_rect_t){184u, 72u, 560u, 414u};
    state->settings_rect = (window_rect_t){236u, 72u, 420u, 378u};
    state->terminal_rect = (window_rect_t){552u, 82u, 324u, 236u};
    state->files_rect = (window_rect_t){258u, 304u, 278u, 230u};
    state->tasks_rect = (window_rect_t){552u, 330u, 324u, 236u};
    state->welcome_rect = (window_rect_t){236u, 108u, 560u, 278u};
    state->dragging_window = WINDOW_NONE;
    state->launcher_query[0] = '\0';
    state->install_target_index = 0u;
}

static bool toggle_dock_app(desktop_state_t* state, dock_app_t app) {
    window_id_t window;

    if (state == NULL) {
        return false;
    }

    state->show_welcome = false;
    window = dock_app_to_window(app);
    if (window == WINDOW_NONE) {
        return false;
    }

    if (!window_open(state, window)) {
        open_window(state, window);
        return true;
    }

    if (window_minimized(state, window)) {
        open_window(state, window);
        return true;
    }

    if (app == APP_START) {
        close_window(state, window);
        return true;
    }

    minimize_window(state, window);
    return true;
}

static bool handle_keyboard_scancode(desktop_state_t* state, u8 scancode) {
    char input;
    u32 query_length;
    u32 visible_index;

    if ((scancode & 0x80u) != 0 || state == NULL) {
        return false;
    }

    if (window_open(state, WINDOW_SEARCH) && !window_minimized(state, WINDOW_SEARCH)) {
        if (scancode == 0x01) {
            close_window(state, WINDOW_SEARCH);
            state->show_welcome = false;
            return true;
        }

        if (scancode == 0x0E) {
            query_length = (u32)strlen(state->launcher_query);
            if (query_length > 0u) {
                state->launcher_query[query_length - 1u] = '\0';
                return true;
            }
        }

        if (scancode == 0x1C) {
            for (visible_index = 0; visible_index < LAUNCHER_APP_COUNT; ++visible_index) {
                u32 found = 0;
                for (u32 app_index = 0; app_index < LAUNCHER_APP_COUNT; ++app_index) {
                    if (string_contains_case_insensitive(g_launcher_app_names[app_index], state->launcher_query)) {
                        if (found == visible_index) {
                            open_window(state, g_launcher_window_ids[app_index]);
                            close_window(state, WINDOW_SEARCH);
                            state->show_welcome = false;
                            state->launcher_query[0] = '\0';
                            return true;
                        }
                        ++found;
                    }
                }
            }
            return false;
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            query_length = (u32)strlen(state->launcher_query);
            if (query_length + 1u < sizeof(state->launcher_query)) {
                state->launcher_query[query_length] = input;
                state->launcher_query[query_length + 1u] = '\0';
                return true;
            }
        }
    }

    switch (scancode) {
        case 0x01:
        case 0x1C:
        case 0x11:
            state->show_welcome = false;
            return true;
        case 0x1E:
            toggle_dock_app(state, APP_START);
            return true;
        case 0x1F:
            toggle_dock_app(state, APP_SETTINGS);
            return true;
        case 0x14:
            toggle_dock_app(state, APP_TERMINAL);
            return true;
        case 0x21:
            toggle_dock_app(state, APP_FILES);
            return true;
        case 0x32:
            toggle_dock_app(state, APP_TASKS);
            return true;
        default:
            return false;
    }
}

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

static bool mouse_apply_packet(mouse_state_t* mouse) {
    s32 new_x;
    s32 new_y;
    bool left_now;
    bool changed = false;

    if (mouse == NULL) {
        return false;
    }

    left_now = (mouse->packet[0] & 0x01u) != 0;
    if (left_now != mouse->left_down) {
        changed = true;
    }
    mouse->left_down = left_now;

    if ((mouse->packet[0] & 0xC0u) != 0) {
        return changed;
    }

    new_x = (s32)mouse->x + (s32)((s8)mouse->packet[1]);
    new_y = (s32)mouse->y - (s32)((s8)mouse->packet[2]);

    if (new_x < 0) {
        new_x = 0;
    } else if ((u32)new_x >= vga_width()) {
        new_x = (s32)vga_width() - 1;
    }

    if (new_y < 0) {
        new_y = 0;
    } else if ((u32)new_y >= vga_height()) {
        new_y = (s32)vga_height() - 1;
    }

    if ((u32)new_x != mouse->x || (u32)new_y != mouse->y) {
        changed = true;
    }

    mouse->x = (u32)new_x;
    mouse->y = (u32)new_y;
    return changed;
}

static bool mouse_handle_byte(mouse_state_t* mouse, u8 value) {
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

static void mouse_initialize(mouse_state_t* mouse) {
    u8 config;

    if (mouse == NULL) {
        return;
    }

    memset(mouse, 0, sizeof(*mouse));
    mouse->x = vga_width() / 2u;
    mouse->y = vga_height() / 2u;

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

static bool poll_input(desktop_state_t* state, mouse_state_t* mouse) {
    bool changed = false;

    while ((inb(0x64) & 0x01u) != 0) {
        u8 status = inb(0x64);
        u8 data = inb(0x60);

        if ((status & 0x20u) != 0) {
            if (mouse_handle_byte(mouse, data)) {
                changed = true;
            }
        } else if (handle_keyboard_scancode(state, data)) {
            changed = true;
        }
    }

    return changed;
}

static void draw_dock(const desktop_state_t* state, const mouse_state_t* mouse) {
    dock_layout_t layout;
    char clock_buffer[9];
    u32 text_color = vga_color(246, 250, 255);

    (void)mouse;
    get_dock_layout(&layout);

    draw_rounded_rect(layout.dock_x, layout.dock_y, layout.dock_width, layout.dock_height, 22u, vga_color(12, 16, 26));
    vga_fill_rect_alpha(layout.dock_x + 1u, layout.dock_y + 1u, layout.dock_width - 2u, layout.dock_height - 2u,
                        20, 26, 40, 172);
    vga_fill_rect_alpha(layout.dock_x + 8u, layout.dock_y + 6u, layout.dock_width - 16u, 10u, 255, 255, 255, 18);
    draw_rect_outline(layout.dock_x, layout.dock_y, layout.dock_width, layout.dock_height, 1u, vga_color(88, 102, 128));

    for (u32 index = 0; index < 5; ++index) {
        dock_app_t app = (dock_app_t)index;
        u32 tile_x = layout.dock_x + ((index == 0u) ? 12u : (86u + (index - 1u) * 52u));
        u32 tile_y = layout.dock_y + 9u;
        u32 tile_width = (index == 0u) ? 56u : 42u;
        u32 tile_height = 40u;
        u32 icon_x = tile_x + ((tile_width - layout.icon_size) / 2u);
        u32 icon_y = tile_y + 5u;
        bool active = dock_app_active(state, app);
        bool visible = window_visible(state, dock_app_to_window(app));

        if (index == 0u) {
            draw_rounded_rect(tile_x, tile_y, tile_width, tile_height, 18u, vga_color(44, 72, 124));
            vga_fill_rect_alpha(tile_x + 2u, tile_y + 2u, tile_width - 4u, 12u, 255, 255, 255, 42);
            draw_rect_outline(tile_x, tile_y, tile_width, tile_height, 1, vga_color(132, 166, 220));
        } else if (active) {
            draw_rounded_rect(tile_x, tile_y, tile_width, tile_height, 12u,
                              visible ? vga_color(56, 92, 150) : vga_color(34, 54, 86));
            vga_fill_rect_alpha(tile_x + 2u, tile_y + 2u, tile_width - 4u, 10u, 255, 255, 255, 28);
            draw_rect_outline(tile_x, tile_y, tile_width, tile_height, 1, vga_color(116, 134, 170));
        }

        draw_app_icon(icon_x, icon_y, layout.icon_size, app);
        if (active) {
            draw_rounded_rect(tile_x + 8u, layout.dock_y + layout.dock_height - 8u, tile_width - 16u, 3u, 1u,
                              vga_color(174, 216, 255));
        }
        if (index == 0u) {
            draw_text(tile_x + 8u, tile_y + 24u, "Start", text_color);
        }
    }

    rtc_read_time_string(clock_buffer);
    draw_rounded_rect(layout.dock_x + layout.dock_width - 96u, layout.dock_y + 10u, 82u, 34u, 12u, vga_color(24, 34, 52));
    vga_fill_rect_alpha(layout.dock_x + layout.dock_width - 94u, layout.dock_y + 12u, 78u, 10u, 255, 255, 255, 20);
    draw_rect_outline(layout.dock_x + layout.dock_width - 96u, layout.dock_y + 10u, 82u, 34u, 1u, vga_color(102, 118, 144));
    draw_text(layout.dock_x + layout.dock_width - 86u, layout.dock_y + 22u, clock_buffer, text_color);
}

static void launcher_tile_rect(const window_rect_t* rect, u32 index, u32* out_x, u32* out_y, u32* out_width, u32* out_height) {
    u32 column = index % 2u;
    u32 row = index / 2u;

    if (rect == NULL) {
        return;
    }

    if (out_x != NULL) {
        *out_x = rect->x + 22u + column * 130u;
    }
    if (out_y != NULL) {
        *out_y = rect->y + 114u + row * 92u;
    }
    if (out_width != NULL) {
        *out_width = 114u;
    }
    if (out_height != NULL) {
        *out_height = 76u;
    }
}

static void launcher_recommended_rect(const window_rect_t* rect, u32 index, u32* out_x, u32* out_y, u32* out_width, u32* out_height) {
    if (rect == NULL) {
        return;
    }

    if (out_x != NULL) {
        *out_x = rect->x + 296u;
    }
    if (out_y != NULL) {
        *out_y = rect->y + 114u + index * 46u;
    }
    if (out_width != NULL) {
        *out_width = rect->width - 318u;
    }
    if (out_height != NULL) {
        *out_height = 38u;
    }
}

static void draw_launcher_tile(u32 x, u32 y, u32 width, u32 height, dock_app_t app, const char* label) {
    u32 label_x;

    draw_rounded_rect(x, y, width, height, 18u, vga_color(22, 28, 40));
    vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, height - 2u, 244, 248, 255, 10);
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 14u, 255, 255, 255, 18);
    draw_rect_outline(x, y, width, height, 1u, vga_color(90, 106, 132));
    draw_app_icon(x + (width - 36u) / 2u, y + 12u, 36u, app);
    label_x = x + ((width > text_width(label)) ? (width - text_width(label)) / 2u : 8u);
    draw_text(label_x, y + 52u, label, vga_color(232, 238, 246));
}

static void draw_launcher_panel(const desktop_state_t* state, const window_rect_t* rect) {
    u32 text = vga_color(232, 238, 246);
    bool searching;
    u32 visible_row = 0;

    if (rect == NULL || state == NULL) {
        return;
    }

    searching = state->launcher_query[0] != '\0';

    draw_window_frame(state, rect->x, rect->y, rect->width, rect->height, "Start", vga_color(92, 140, 232));
    draw_text(rect->x + 22u, rect->y + 50u, "Windows 11 inspired launcher", vga_color(170, 204, 255));
    draw_rounded_rect(rect->x + 22u, rect->y + 68u, rect->width - 44u, 32u, 14u, vga_color(255, 255, 255));
    draw_rect_outline(rect->x + 22u, rect->y + 68u, rect->width - 44u, 32u, 1u, vga_color(172, 192, 224));
    if (state->launcher_query[0] != '\0') {
        draw_text(rect->x + 34u, rect->y + 80u, state->launcher_query, vga_color(34, 40, 48));
    } else {
        draw_text(rect->x + 34u, rect->y + 80u, "Search apps, settings, tools", vga_color(112, 120, 130));
    }

    if (!searching) {
        draw_text(rect->x + 22u, rect->y + 112u, "Pinned", vga_color(204, 214, 232));
        for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
            u32 tile_x;
            u32 tile_y;
            u32 tile_width;
            u32 tile_height;

            launcher_tile_rect(rect, index, &tile_x, &tile_y, &tile_width, &tile_height);
            draw_launcher_tile(tile_x, tile_y, tile_width, tile_height, g_launcher_dock_apps[index], g_launcher_app_names[index]);
        }

        draw_text(rect->x + 296u, rect->y + 112u, "Recommended", vga_color(204, 214, 232));
        for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
            u32 row_x;
            u32 row_y;
            u32 row_width;
            u32 row_height;

            launcher_recommended_rect(rect, index, &row_x, &row_y, &row_width, &row_height);
            draw_rounded_rect(row_x, row_y, row_width, row_height, 14u, vga_color(18, 24, 36));
            vga_fill_rect_alpha(row_x + 1u, row_y + 1u, row_width - 2u, 10u, 255, 255, 255, 10);
            draw_rect_outline(row_x, row_y, row_width, row_height, 1u, vga_color(88, 102, 128));
            draw_app_icon(row_x + 8u, row_y + 4u, 28u, g_launcher_dock_apps[index]);
            draw_text(row_x + 44u, row_y + 8u, g_launcher_app_names[index], text);
            if (index == 0u) {
                draw_text(row_x + 44u, row_y + 20u, "Personalization and About", vga_color(142, 156, 178));
            } else if (index == 1u) {
                draw_text(row_x + 44u, row_y + 20u, "Open the live shell", vga_color(142, 156, 178));
            } else if (index == 2u) {
                draw_text(row_x + 44u, row_y + 20u, "Browse files and assets", vga_color(142, 156, 178));
            } else {
                draw_text(row_x + 44u, row_y + 20u, "View system activity", vga_color(142, 156, 178));
            }
        }

        draw_rounded_rect(rect->x + 22u, rect->y + rect->height - 56u, rect->width - 44u, 34u, 14u, vga_color(18, 24, 36));
        vga_fill_rect_alpha(rect->x + 23u, rect->y + rect->height - 55u, rect->width - 46u, 12u, 255, 255, 255, 10);
        draw_rect_outline(rect->x + 22u, rect->y + rect->height - 56u, rect->width - 44u, 34u, 1u, vga_color(88, 102, 128));
        draw_text(rect->x + 36u, rect->y + rect->height - 43u, "froxy", vga_color(236, 242, 250));
        draw_text(rect->x + rect->width - 190u, rect->y + rect->height - 43u, "Power and profile area", vga_color(150, 162, 184));
        return;
    }

    draw_text(rect->x + 22u, rect->y + 112u, "Search results", vga_color(204, 214, 232));
    for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
        u32 row_y;

        if (!string_contains_case_insensitive(g_launcher_app_names[index], state->launcher_query)) {
            continue;
        }

        row_y = rect->y + 136u + visible_row * 44u;
        draw_rounded_rect(rect->x + 22u, row_y, rect->width - 44u, 36u, 12u, vga_color(250, 252, 255));
        draw_rect_outline(rect->x + 22u, row_y, rect->width - 44u, 36u, 1u, vga_color(214, 226, 240));
        draw_app_icon(rect->x + 30u, row_y + 3u, 28u, g_launcher_dock_apps[index]);
        draw_text(rect->x + 70u, row_y + 14u, g_launcher_app_names[index], vga_color(34, 40, 48));
        ++visible_row;
    }

    if (visible_row == 0u) {
        draw_text(rect->x + 24u, rect->y + 146u, "No apps match your search", vga_color(142, 156, 178));
    }

    draw_text(rect->x + 22u, rect->y + rect->height - 46u, "Press Enter to open the first result", vga_color(150, 162, 184));
    draw_text(rect->x + 22u, rect->y + rect->height - 28u, "Press Esc to close Start", vga_color(150, 162, 184));
}

static void draw_settings_option_row(u32 x, u32 y, u32 width, const char* label, const char* value) {
    draw_rounded_rect(x, y, width, 34u, 12u, vga_color(18, 24, 36));
    vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, 10u, 255, 255, 255, 12);
    draw_rect_outline(x, y, width, 34u, 1u, vga_color(88, 102, 128));
    draw_text(x + 14u, y + 13u, label, vga_color(228, 234, 244));
    draw_text(x + width - 16u - text_width(value), y + 13u, value, vga_color(132, 196, 255));
}

static void draw_settings_tab(u32 x, u32 y, const char* label, bool active) {
    draw_rounded_rect(x, y, 126u, 28u, 12u, active ? vga_color(66, 116, 204) : vga_color(18, 24, 36));
    vga_fill_rect_alpha(x + 1u, y + 1u, 124u, 10u, 255, 255, 255, active ? 26u : 10u);
    draw_rect_outline(x, y, 126u, 28u, 1u, active ? vga_color(150, 190, 248) : vga_color(88, 102, 128));
    draw_text(x + 18u, y + 10u, label, active ? vga_color(246, 250, 255) : vga_color(198, 208, 224));
}

static void draw_settings_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    char memory_total[24];
    char display_mode[24];
    char storage_size[24];
    char storage_model[41];
    char boot_mode[24];
    u32 frame_height;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_SETTINGS) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Settings", vga_color(92, 132, 210));
    if (shaded) {
        return;
    }

    draw_settings_tab(rect->x + 20u, rect->y + 50u, "Personalize", state->settings_page == SETTINGS_PAGE_PERSONALIZE);
    draw_settings_tab(rect->x + 154u, rect->y + 50u, "About", state->settings_page == SETTINGS_PAGE_ABOUT);

    if (state->settings_page == SETTINGS_PAGE_ABOUT) {
        format_megabytes(memory_total, pmm_total_memory());
        format_display_mode(display_mode);
        format_storage_size(storage_size, state->storage_info.total_sectors);
        strcpy(storage_model, state->storage_info.model[0] != '\0' ? state->storage_info.model : "Virtual ATA disk");
        strcpy(boot_mode, state->live_media_boot ? "Live ISO" : "Installed");

        draw_text(rect->x + 20u, rect->y + 92u, "JabulOS Aero credits", vga_color(224, 230, 240));
        draw_text(rect->x + 20u, rect->y + 112u, "Owner and operating system creator: froxy", vga_color(208, 220, 238));
        draw_text(rect->x + 20u, rect->y + 130u, "Desktop shell, installer flow, launcher,", vga_color(184, 194, 210));
        draw_text(rect->x + 20u, rect->y + 148u, "and Aero styling by froxy", vga_color(184, 194, 210));
        draw_text(rect->x + 20u, rect->y + 166u, "Custom 64 bit kernel with a live graphical shell", vga_color(184, 194, 210));
        draw_settings_option_row(rect->x + 20u, rect->y + 196u, rect->width - 40u, "Boot mode", boot_mode);
        draw_settings_option_row(rect->x + 20u, rect->y + 236u, rect->width - 40u, "Display", display_mode);
        draw_settings_option_row(rect->x + 20u, rect->y + 276u, rect->width - 40u, "RAM total", memory_total);
        draw_settings_option_row(rect->x + 20u, rect->y + 316u, rect->width - 40u, "Storage", state->storage_info.present ? storage_size : "Not detected");
        draw_text(rect->x + 20u, rect->y + 356u, storage_model, vga_color(132, 196, 255));
        return;
    }

    draw_text(rect->x + 20u, rect->y + 92u, "Customize the desktop live", vga_color(224, 230, 240));
    draw_settings_option_row(rect->x + 20u, rect->y + 122u, rect->width - 40u, "Desktop theme", desktop_theme_name(state->desktop_theme));
    draw_settings_option_row(rect->x + 20u, rect->y + 162u, rect->width - 40u, "Window theme", window_theme_name(state->window_theme));
    draw_settings_option_row(rect->x + 20u, rect->y + 202u, rect->width - 40u, "Transparency", state->transparency_enabled ? "Enabled" : "Disabled");
    draw_text(rect->x + 20u, rect->y + 246u, "Click a row to cycle the option", vga_color(184, 194, 210));
    draw_text(rect->x + 20u, rect->y + 264u, "Open About for credits, owner, and system specs", vga_color(132, 196, 255));
}

static void draw_terminal_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;

    if (rect == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_TERMINAL) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Terminal", vga_color(74, 186, 146));
    if (shaded) {
        return;
    }
    draw_rect(rect->x + 12, rect->y + 42, rect->width - 24, 180, vga_color(10, 14, 22));
    vga_fill_rect_alpha(rect->x + 12u, rect->y + 42u, rect->width - 24u, 180u, 255, 255, 255, 8);
    draw_text(rect->x + 24, rect->y + 56, "root jabulos", vga_color(104, 214, 160));
    draw_text(rect->x + 24, rect->y + 78, "long mode kernel online", vga_color(214, 221, 232));
    draw_text(rect->x + 24, rect->y + 100, "64 bit desktop shell ready", vga_color(214, 221, 232));
    draw_text(rect->x + 24, rect->y + 122, "window drag active", vga_color(214, 221, 232));
    draw_text(rect->x + 24, rect->y + 144, "dock opens apps", vga_color(214, 221, 232));
    draw_text(rect->x + 24, rect->y + 166, "translucent compositor live", vga_color(214, 221, 232));
    draw_progress_bar(rect->x + 24, rect->y + 188, 248, 16, 88, vga_color(90, 226, 170));
    draw_text(rect->x + 24, rect->y + 210, "shell compositor steady", vga_color(104, 214, 160));
}

static void draw_file_manager_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 row_y;
    u32 frame_height;

    if (rect == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_FILES) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "File Manager", vga_color(214, 170, 88));
    if (shaded) {
        return;
    }
    draw_rect(rect->x + 14, rect->y + 42, 72, 170, vga_color(38, 30, 18));
    vga_fill_rect_alpha(rect->x + 14u, rect->y + 42u, 72u, 170u, 255, 255, 255, 10);
    draw_text(rect->x + 24, rect->y + 56, "Home", vga_color(236, 222, 188));
    draw_text(rect->x + 24, rect->y + 80, "Apps", vga_color(236, 222, 188));
    draw_text(rect->x + 24, rect->y + 104, "System", vga_color(236, 222, 188));
    draw_text(rect->x + 24, rect->y + 128, "Kernel", vga_color(236, 222, 188));

    row_y = rect->y + 54;
    for (u32 row = 0; row < 4; ++row) {
        draw_rounded_rect(rect->x + 98, row_y, 158, 30, 8, vga_color(38, 34, 24));
        draw_rect_outline(rect->x + 98, row_y, 158, 30, 1, vga_color(122, 108, 76));
        draw_app_icon(rect->x + 106, row_y + 1, 26, APP_FILES);
        if (row == 0) {
            draw_text(rect->x + 142, row_y + 11, "Desktop", vga_color(236, 230, 216));
        } else if (row == 1) {
            draw_text(rect->x + 142, row_y + 11, "Documents", vga_color(236, 230, 216));
        } else if (row == 2) {
            draw_text(rect->x + 142, row_y + 11, "Apps", vga_color(236, 230, 216));
        } else {
            draw_text(rect->x + 142, row_y + 11, "KernelLogs", vga_color(236, 230, 216));
        }
        row_y += 36;
    }
}

static void draw_metric_row(u32 x, u32 y, const char* title, u32 percent, u32 color) {
    draw_text(x, y, title, vga_color(48, 54, 62));
    draw_progress_bar(x + 104, y - 4, 146, 16, percent, color);
}

static void draw_task_manager_window(const boot_info_t* boot_info,
                                     bool wallpaper_loaded,
                                     bool ata_ready,
                                     const desktop_state_t* state,
                                     const window_rect_t* rect,
                                     bool shaded) {
    char mem_total[24];
    char mem_free[24];
    char mem_used[24];
    u32 frame_height;
    u64 total = pmm_total_memory();
    u64 free = pmm_free_memory();
    u64 used = (total > free) ? (total - free) : 0;
    u32 memory_percent = (total != 0) ? (u32)((used * 100u) / total) : 0;

    (void)boot_info;

    if (rect == NULL) {
        return;
    }

    format_megabytes(mem_total, total);
    format_megabytes(mem_free, free);
    format_megabytes(mem_used, used);

    frame_height = shaded ? shaded_window_height(WINDOW_TASKS) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Task Manager", vga_color(142, 112, 214));
    if (shaded) {
        return;
    }
    draw_metric_row(rect->x + 20, rect->y + 50, "CPU load", 46, vga_color(142, 116, 255));
    draw_metric_row(rect->x + 20, rect->y + 76, "RAM load", memory_percent, vga_color(82, 174, 255));
    draw_metric_row(rect->x + 20, rect->y + 102, "Disk io", ata_ready ? 54u : 0u, vga_color(94, 214, 172));
    draw_metric_row(rect->x + 20, rect->y + 128, "Shell draw", 18, vga_color(246, 196, 92));

    draw_text(rect->x + 20, rect->y + 164, "RAM used", vga_color(220, 228, 238));
    draw_text(rect->x + 100, rect->y + 164, mem_used, vga_color(220, 228, 238));
    draw_text(rect->x + 20, rect->y + 184, "RAM free", vga_color(220, 228, 238));
    draw_text(rect->x + 100, rect->y + 184, mem_free, vga_color(220, 228, 238));
    draw_text(rect->x + 20, rect->y + 204, "RAM total", vga_color(220, 228, 238));
    draw_text(rect->x + 100, rect->y + 204, mem_total, vga_color(220, 228, 238));
    draw_text(rect->x + 20, rect->y + 224, wallpaper_loaded ? "Wallpaper yes" : "Wallpaper no",
              vga_color(176, 190, 208));
}

static bool setup_active(const desktop_state_t* state) {
    return state != NULL && state->setup_phase != SETUP_PHASE_NONE;
}

static void draw_centered_box(u32 width, u32 height, u32 color, u32* out_x, u32* out_y) {
    u32 x = (vga_width() > width) ? (vga_width() - width) / 2u : 0u;
    u32 y = (vga_height() > height) ? (vga_height() - height) / 2u : 0u;

    draw_rounded_rect(x, y, width, height, 26u, color);
    vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, height - 2u, 18, 24, 36, 192);
    draw_rect_outline(x, y, width, height, 1u, vga_color(104, 120, 148));

    if (out_x != NULL) {
        *out_x = x;
    }
    if (out_y != NULL) {
        *out_y = y;
    }
}

static u32 installer_progress_percent(const desktop_state_t* state, u32 current_second) {
    u32 elapsed;

    if (state == NULL || state->setup_phase != SETUP_PHASE_INSTALLING) {
        return 0u;
    }

    elapsed = seconds_since(state->setup_phase_started_second, current_second);
    if (elapsed >= SETUP_INSTALL_SECONDS) {
        return 100u;
    }
    return (elapsed * 100u) / SETUP_INSTALL_SECONDS;
}

static void draw_setup_installer_screen(const desktop_state_t* state, const boot_info_t* boot_info, u32 current_second) {
    u32 panel_x;
    u32 panel_y;
    u32 target_count;
    bool installing = state->setup_phase == SETUP_PHASE_INSTALLING;

    target_count = install_target_count(state);

    draw_centered_box(700u, 404u, vga_color(10, 14, 24), &panel_x, &panel_y);
    draw_text(panel_x + 34u, panel_y + 40u, "Install JabulOS", vga_color(242, 246, 255));
    draw_text(panel_x + 34u, panel_y + 66u, "Live setup from ISO", vga_color(156, 198, 255));
    draw_text(panel_x + 34u, panel_y + 98u, "Select which partition you would like to install to.", vga_color(214, 222, 236));
    draw_text(panel_x + 34u, panel_y + 118u, "Available targets are listed below with their sizes.", vga_color(214, 222, 236));
    if (state->oobe_install_failed) {
        draw_text(panel_x + 34u, panel_y + 146u, "The selected target is too small or the disk write failed.", vga_color(252, 204, 204));
    } else if (installing) {
        draw_text(panel_x + 34u, panel_y + 146u, "Installing system files. Estimated time about 15 minutes.", vga_color(214, 222, 236));
    } else {
        draw_text(panel_x + 34u, panel_y + 146u,
                  state->oobe_storage_ready ? "ATA disk detected. Choose a target and install." : "No ATA disk detected. Install is unavailable.",
                  state->oobe_storage_ready ? vga_color(206, 244, 220) : vga_color(252, 204, 204));
    }

    if (!installing && state->oobe_storage_ready && target_count > 0u) {
        for (u32 index = 0; index < target_count; ++index) {
            char label[32];
            char details[48];
            char size_buffer[24];
            u32 row_y = panel_y + 174u + index * 42u;
            u32 target_lba;
            u32 target_sectors;
            u32 partition_number;
            bool whole_disk;
            bool fits_target;
            bool selected = state->install_target_index == index;

            get_install_target(state, index, &target_lba, &target_sectors, &partition_number, &whole_disk);
            fits_target = install_target_can_fit(state, boot_info, index);
            format_storage_size(size_buffer, target_sectors);
            if (whole_disk) {
                strcpy(label, "ATA0 whole disk");
            } else {
                strcpy(label, "Partition ");
                append_uint(label + strlen(label), partition_number);
            }
            if (fits_target) {
                strcpy(details, "Start LBA ");
                append_uint(details + strlen(details), target_lba);
            } else {
                strcpy(details, "Not enough space for the image");
            }

            draw_rounded_rect(panel_x + 34u, row_y, 632u, 34u, 12u,
                              selected ? vga_color(40, 74, 126) : (fits_target ? vga_color(18, 24, 36) : vga_color(54, 24, 24)));
            vga_fill_rect_alpha(panel_x + 35u, row_y + 1u, 630u, 10u, 255, 255, 255, selected ? 20u : 10u);
            draw_rect_outline(panel_x + 34u, row_y, 632u, 34u, 1u,
                              selected ? vga_color(140, 182, 244) : (fits_target ? vga_color(88, 102, 128) : vga_color(176, 94, 94)));
            draw_text(panel_x + 48u, row_y + 12u, label, vga_color(232, 238, 246));
            draw_text(panel_x + 196u, row_y + 12u, details, fits_target ? vga_color(160, 176, 198) : vga_color(246, 186, 186));
            draw_text(panel_x + 34u + 632u - 16u - text_width(size_buffer), row_y + 12u, size_buffer, vga_color(132, 196, 255));
        }
    }

    draw_progress_bar(panel_x + 34u, panel_y + 318u, 632u, 18u,
                      installing ? installer_progress_percent(state, current_second) : 0u,
                      vga_color(88, 144, 240));

    if (!installing) {
        draw_text(panel_x + 34u, panel_y + 344u,
                  state->oobe_storage_ready ? "The selected target is used for the install image." : "Connect a disk to continue.",
                  vga_color(184, 194, 210));
        draw_oobe_button(panel_x + 34u, panel_y + 356u, 148u, "Skip for now", false);
        draw_oobe_button(panel_x + 700u - 34u - 176u, panel_y + 356u, 176u,
                         state->oobe_storage_ready ? "Install selected" : "No disk", true);
    } else {
        draw_text(panel_x + 34u, panel_y + 350u, "Please wait while JabulOS prepares your desktop.", vga_color(184, 194, 210));
    }
}

static void draw_setup_customizing_screen(void) {
    u32 text_y = (vga_height() > 40u) ? (vga_height() / 2u - 10u) : 0u;
    draw_text_centered(text_y, "Getting ready to customize jabul.", vga_color(244, 248, 255));
}

static void draw_setup_user_oobe_screen(const desktop_state_t* state) {
    u32 panel_x;
    u32 panel_y;

    draw_centered_box(640u, 360u, vga_color(10, 14, 24), &panel_x, &panel_y);
    draw_text(panel_x + 40u, panel_y + 40u, "Lets finish setting up Jabul", vga_color(242, 246, 255));
    draw_text(panel_x + 40u, panel_y + 68u, "Windows 11 style first run", vga_color(156, 198, 255));
    draw_text(panel_x + 40u, panel_y + 100u, "Choose your look now. You can still change it later in Settings.", vga_color(214, 222, 236));
    draw_settings_option_row(panel_x + 40u, panel_y + 138u, 560u, "Desktop theme", desktop_theme_name(state->desktop_theme));
    draw_settings_option_row(panel_x + 40u, panel_y + 180u, 560u, "Window theme", window_theme_name(state->window_theme));
    draw_settings_option_row(panel_x + 40u, panel_y + 222u, 560u, "Transparency", state->transparency_enabled ? "Enabled" : "Disabled");
    draw_text(panel_x + 40u, panel_y + 274u, "Click any row to cycle choices, then finish setup.", vga_color(184, 194, 210));
    draw_oobe_button(panel_x + 40u, panel_y + 306u, 156u, "Use defaults", false);
    draw_oobe_button(panel_x + 640u - 40u - 176u, panel_y + 306u, 176u, "Finish setup", true);
}

static void render_setup_screen(const desktop_state_t* state,
                                const image_t* wallpaper,
                                const boot_info_t* boot_info,
                                u32 current_second) {
    draw_desktop_background(state, wallpaper);
    vga_fill_rect_alpha(0, 0, vga_width(), vga_height(), 8, 12, 20, 122);

    if (state->setup_phase == SETUP_PHASE_CUSTOMIZING) {
        draw_setup_customizing_screen();
    } else if (state->setup_phase == SETUP_PHASE_USER_OOBE) {
        draw_setup_user_oobe_screen(state);
    } else {
        draw_setup_installer_screen(state, boot_info, current_second);
    }
}

static void draw_oobe_button(u32 x, u32 y, u32 width, const char* label, bool primary) {
    u32 fill = primary ? vga_color(72, 132, 236) : vga_color(24, 30, 44);
    u32 stroke = primary ? vga_color(168, 210, 255) : vga_color(100, 110, 132);
    u32 text = vga_color(244, 248, 255);
    u32 label_width = text_width(label);
    u32 label_x = x + ((width > label_width) ? (width - label_width) / 2u : 0u);

    draw_rounded_rect(x, y, width, OOBE_BUTTON_HEIGHT, 12u, fill);
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 255, 255, 255, primary ? 40 : 18);
    draw_rect_outline(x, y, width, OOBE_BUTTON_HEIGHT, 1u, stroke);
    draw_text(label_x, y + 13u, label, text);
}

static oobe_button_t hit_test_oobe_button(const desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect = get_window_rect_const(state, WINDOW_WELCOME);
    u32 button_y;
    u32 secondary_x;
    u32 primary_x;

    if (state == NULL || rect == NULL || !window_visible(state, WINDOW_WELCOME) || window_shaded(state, WINDOW_WELCOME)) {
        return OOBE_BUTTON_NONE;
    }

    button_y = rect->y + rect->height - 50u;
    secondary_x = rect->x + 24u;
    primary_x = rect->x + rect->width - OOBE_BUTTON_WIDTH - 24u;

    if (point_in_rect(mouse_x, mouse_y, primary_x, button_y, OOBE_BUTTON_WIDTH, OOBE_BUTTON_HEIGHT)) {
        return OOBE_BUTTON_PRIMARY;
    }
    if (point_in_rect(mouse_x, mouse_y, secondary_x, button_y, OOBE_BUTTON_WIDTH, OOBE_BUTTON_HEIGHT)) {
        return OOBE_BUTTON_SECONDARY;
    }

    return OOBE_BUTTON_NONE;
}

static void dismiss_oobe(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    state->show_welcome = false;
    state->show_search = false;
}

static void draw_oobe_window(const desktop_state_t* state, const window_rect_t* rect) {
    char target_label[32];
    char target_size[24];
    const char* title;
    const char* primary_label;
    const char* secondary_label;
    u32 accent = vga_color(88, 144, 240);

    if (state == NULL || rect == NULL || !window_visible(state, WINDOW_WELCOME)) {
        return;
    }

    if (state->oobe_stage == OOBE_STAGE_CONFIRM) {
        title = "Disk install";
        primary_label = "Write disk";
        secondary_label = "Go back";
        accent = vga_color(238, 146, 88);
    } else if (state->oobe_stage == OOBE_STAGE_COMPLETE) {
        title = "Install complete";
        primary_label = "Open desktop";
        secondary_label = "Keep setup";
        accent = vga_color(86, 198, 140);
    } else if (state->oobe_stage == OOBE_STAGE_ERROR) {
        title = "Install failed";
        primary_label = "Try again";
        secondary_label = "Open desktop";
        accent = vga_color(232, 92, 92);
    } else if (state->oobe_stage == OOBE_STAGE_STORAGE_MISSING) {
        title = "No disk";
        primary_label = "Open desktop";
        secondary_label = "Keep setup";
        accent = vga_color(232, 92, 92);
    } else {
        title = "First boot";
        primary_label = "Install now";
        secondary_label = "Open desktop";
    }

    format_install_target_name(state, state->install_target_index, target_label, target_size);
    draw_window_frame(state, rect->x, rect->y, rect->width, rect->height, title, accent);

    draw_text(rect->x + 24u, rect->y + 56u, PREVIEW_LABEL_TEXT, vga_color(232, 238, 248));
    draw_text(rect->x + 24u, rect->y + 78u, "Modern translucent shell active", vga_color(214, 222, 236));
    draw_text(rect->x + 24u, rect->y + 98u, "Kernel already runs in 64 bit mode", vga_color(188, 198, 214));

    draw_rounded_rect(rect->x + 24u, rect->y + 126u, rect->width - 48u, 54u, 14u, vga_color(18, 24, 36));
    vga_fill_rect_alpha(rect->x + 25u, rect->y + 127u, rect->width - 50u, 52u, 255, 255, 255, 10);
    draw_rect_outline(rect->x + 24u, rect->y + 126u, rect->width - 48u, 54u, 1u, vga_color(88, 102, 130));
    if (target_label[0] != '\0') {
        draw_text(rect->x + rect->width - 180u, rect->y + 110u, target_label, vga_color(132, 196, 255));
        draw_text(rect->x + rect->width - 92u, rect->y + 110u, target_size, vga_color(184, 194, 210));
    }

    if (state->oobe_stage == OOBE_STAGE_CONFIRM) {
        draw_text(rect->x + 38u, rect->y + 144u,
                  target_label[0] != '\0' ? "Warning this overwrites the selected target" : "Warning this overwrites the selected disk target",
                  vga_color(246, 210, 186));
        draw_text(rect->x + 38u, rect->y + 162u, "Kernel and initrd are written directly", vga_color(214, 222, 236));
    } else if (state->oobe_stage == OOBE_STAGE_COMPLETE) {
        draw_text(rect->x + 38u, rect->y + 144u,
                  target_label[0] != '\0' ? "System image was written to the selected target" : "System image was written to disk",
                  vga_color(206, 244, 220));
        draw_text(rect->x + 38u, rect->y + 162u, "Next boots skip setup one time flow", vga_color(214, 222, 236));
    } else if (state->oobe_stage == OOBE_STAGE_ERROR) {
        draw_text(rect->x + 38u, rect->y + 144u, "Installer could not write the disk", vga_color(252, 204, 204));
        draw_text(rect->x + 38u, rect->y + 162u, "Retry in a VM or check storage", vga_color(214, 222, 236));
    } else if (state->oobe_stage == OOBE_STAGE_STORAGE_MISSING) {
        draw_text(rect->x + 38u, rect->y + 144u, "No ATA primary master was detected", vga_color(252, 204, 204));
        draw_text(rect->x + 38u, rect->y + 162u, "Desktop works but install is disabled", vga_color(214, 222, 236));
    } else {
        draw_text(rect->x + 38u, rect->y + 144u,
                  target_label[0] != '\0' ? "Ready to install the live image to the selected target" : "Ready to install the live image",
                  vga_color(214, 222, 236));
        draw_text(rect->x + 38u, rect->y + 162u, "One time OOBE runs only until install exists", vga_color(214, 222, 236));
    }

    draw_text(rect->x + 24u, rect->y + 198u, state->oobe_storage_ready ? "Storage ATA ready" : "Storage ATA missing",
              vga_color(214, 222, 236));
    draw_text(rect->x + 220u, rect->y + 198u, state->oobe_disk_has_install ? "Disk image present" : "Disk image not found",
              vga_color(214, 222, 236));
    draw_progress_bar(rect->x + 24u, rect->y + 220u, rect->width - 48u, 16u,
                      state->oobe_disk_has_install ? 100u : (state->oobe_storage_ready ? 52u : 8u),
                      state->oobe_disk_has_install ? vga_color(86, 198, 140) : vga_color(88, 144, 240));

    draw_oobe_button(rect->x + 24u, rect->y + rect->height - 50u, OOBE_BUTTON_WIDTH, secondary_label, false);
    draw_oobe_button(rect->x + rect->width - OOBE_BUTTON_WIDTH - 24u, rect->y + rect->height - 50u,
                     OOBE_BUTTON_WIDTH, primary_label, true);
}

static bool handle_oobe_click(desktop_state_t* state, const boot_info_t* boot_info, u32 mouse_x, u32 mouse_y) {
    oobe_button_t button;

    if (state == NULL || boot_info == NULL || !state->show_welcome) {
        return false;
    }

    button = hit_test_oobe_button(state, mouse_x, mouse_y);
    if (button == OOBE_BUTTON_NONE) {
        return false;
    }

    if (button == OOBE_BUTTON_SECONDARY) {
        if (state->oobe_stage == OOBE_STAGE_CONFIRM) {
            state->oobe_stage = OOBE_STAGE_INTRO;
        } else if (state->oobe_stage == OOBE_STAGE_COMPLETE || state->oobe_stage == OOBE_STAGE_STORAGE_MISSING) {
            state->show_welcome = true;
        } else if (state->oobe_stage == OOBE_STAGE_ERROR) {
            dismiss_oobe(state);
        } else {
            dismiss_oobe(state);
        }
        return true;
    }

    if (state->oobe_stage == OOBE_STAGE_INTRO) {
        if (!state->oobe_storage_ready) {
            dismiss_oobe(state);
        } else {
            state->oobe_stage = OOBE_STAGE_CONFIRM;
        }
        return true;
    }

    if (state->oobe_stage == OOBE_STAGE_CONFIRM) {
        u32 kernel_size = (u32)(boot_info->kernel_end - boot_info->kernel_start);
        u32 target_lba;
        u32 target_sectors;
        bool installed;

        if (!get_install_target(state, state->install_target_index, &target_lba, &target_sectors, NULL, NULL)) {
            state->oobe_install_failed = true;
            state->oobe_stage = OOBE_STAGE_ERROR;
            return true;
        }

        installed = installer_install_to_target((const void*)boot_info->kernel_start,
                                                kernel_size,
                                                (const void*)boot_info->initrd_start,
                                                (u32)boot_info->initrd_size,
                                                target_lba,
                                                target_sectors);
        state->oobe_install_complete = installed;
        state->oobe_install_failed = !installed;
        state->oobe_disk_has_install = installed || state->oobe_disk_has_install;
        state->oobe_stage = installed ? OOBE_STAGE_COMPLETE : OOBE_STAGE_ERROR;
        return true;
    }

    if (state->oobe_stage == OOBE_STAGE_ERROR) {
        state->oobe_stage = state->oobe_storage_ready ? OOBE_STAGE_CONFIRM : OOBE_STAGE_STORAGE_MISSING;
        return true;
    }

    dismiss_oobe(state);
    return true;
}

static void draw_mouse_cursor_front(u32 x, u32 y) {
    for (u32 row = 0; row < 18; ++row) {
        for (u32 col = 0; col <= row / 2u; ++col) {
            vga_put_pixel_front(x + col, y + row, vga_color(255, 255, 255));
        }
    }

    for (u32 row = 0; row < 18; ++row) {
        u32 edge = row / 2u;
        vga_put_pixel_front(x + edge, y + row, vga_color(34, 38, 44));
        if (edge > 0) {
            vga_put_pixel_front(x + edge - 1u, y + row, vga_color(34, 38, 44));
        }
    }

    for (u32 tail = 0; tail < 7; ++tail) {
        vga_put_pixel_front(x + 4u + tail, y + 16u + tail, vga_color(34, 38, 44));
        vga_put_pixel_front(x + 5u + tail, y + 16u + tail, vga_color(34, 38, 44));
    }
}

static void refresh_cursor_only(u32 previous_x, u32 previous_y, const mouse_state_t* mouse) {
    if (mouse == NULL) {
        return;
    }

    vga_present_region(previous_x, previous_y, CURSOR_WIDTH, CURSOR_HEIGHT);
    if (mouse->x != previous_x || mouse->y != previous_y) {
        vga_present_region(mouse->x, mouse->y, CURSOR_WIDTH, CURSOR_HEIGHT);
    }
    draw_mouse_cursor_front(mouse->x, mouse->y);
}

static window_id_t find_draggable_window(const desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_id_t order[] = {
        WINDOW_WELCOME,
        WINDOW_TASKS,
        WINDOW_FILES,
        WINDOW_TERMINAL,
        WINDOW_SETTINGS,
        WINDOW_SEARCH
    };

    for (u32 index = 0; index < sizeof(order) / sizeof(order[0]); ++index) {
        window_id_t window = order[index];
        const window_rect_t* rect = get_window_rect_const(state, window);

        if (window_visible(state, window) && point_in_window_title(mouse_x, mouse_y, rect, window)) {
            return window;
        }
    }

    return WINDOW_NONE;
}

static bool activate_launcher_result(desktop_state_t* state, u32 launcher_index) {
    if (state == NULL || launcher_index >= LAUNCHER_APP_COUNT) {
        return false;
    }

    open_window(state, g_launcher_window_ids[launcher_index]);
    close_window(state, WINDOW_SEARCH);
    state->show_welcome = false;
    state->launcher_query[0] = '\0';
    return true;
}

static bool handle_launcher_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 launcher_index;

    if (state == NULL || !window_visible(state, WINDOW_SEARCH) || window_shaded(state, WINDOW_SEARCH)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_SEARCH);
    if (rect == NULL) {
        return false;
    }

    if (point_in_rect(mouse_x, mouse_y, rect->x + 22u, rect->y + 68u, rect->width - 44u, 32u)) {
        return true;
    }

    if (launcher_result_at(state, rect, mouse_x, mouse_y, &launcher_index)) {
        return activate_launcher_result(state, launcher_index);
    }

    return false;
}

static settings_hit_t hit_test_settings_option(const desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;

    if (state == NULL || !window_visible(state, WINDOW_SETTINGS) || window_shaded(state, WINDOW_SETTINGS)) {
        return SETTINGS_HIT_NONE;
    }

    rect = get_window_rect_const(state, WINDOW_SETTINGS);
    if (rect == NULL) {
        return SETTINGS_HIT_NONE;
    }

    if (point_in_rect(mouse_x, mouse_y, rect->x + 20u, rect->y + 50u, 126u, 28u)) {
        return SETTINGS_HIT_TAB_PERSONALIZE;
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + 154u, rect->y + 50u, 126u, 28u)) {
        return SETTINGS_HIT_TAB_ABOUT;
    }
    if (state->settings_page != SETTINGS_PAGE_PERSONALIZE) {
        return SETTINGS_HIT_NONE;
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + 20u, rect->y + 122u, rect->width - 40u, 34u)) {
        return SETTINGS_HIT_DESKTOP_THEME;
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + 20u, rect->y + 162u, rect->width - 40u, 34u)) {
        return SETTINGS_HIT_WINDOW_THEME;
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + 20u, rect->y + 202u, rect->width - 40u, 34u)) {
        return SETTINGS_HIT_TRANSPARENCY;
    }

    return SETTINGS_HIT_NONE;
}

static bool handle_settings_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    settings_hit_t hit = hit_test_settings_option(state, mouse_x, mouse_y);

    if (state == NULL || hit == SETTINGS_HIT_NONE) {
        return false;
    }

    if (hit == SETTINGS_HIT_TAB_PERSONALIZE) {
        state->settings_page = SETTINGS_PAGE_PERSONALIZE;
    } else if (hit == SETTINGS_HIT_TAB_ABOUT) {
        state->settings_page = SETTINGS_PAGE_ABOUT;
    } else if (hit == SETTINGS_HIT_DESKTOP_THEME) {
        state->desktop_theme = (state->desktop_theme + 1u) % DESKTOP_THEME_COUNT;
    } else if (hit == SETTINGS_HIT_WINDOW_THEME) {
        state->window_theme = (state->window_theme + 1u) % WINDOW_THEME_COUNT;
    } else if (hit == SETTINGS_HIT_TRANSPARENCY) {
        state->transparency_enabled = !state->transparency_enabled;
    }

    return true;
}

static bool handle_setup_click(desktop_state_t* state,
                               const boot_info_t* boot_info,
                               u32 current_second,
                               u32 mouse_x,
                               u32 mouse_y) {
    u32 panel_x;
    u32 panel_y;

    if (!setup_active(state) || boot_info == NULL) {
        return false;
    }

    if (state->setup_phase == SETUP_PHASE_INSTALLER) {
        panel_x = (vga_width() > 700u) ? (vga_width() - 700u) / 2u : 0u;
        panel_y = (vga_height() > 404u) ? (vga_height() - 404u) / 2u : 0u;

        if (state->oobe_storage_ready) {
            u32 target_count = install_target_count(state);
            for (u32 index = 0; index < target_count; ++index) {
                u32 row_y = panel_y + 174u + index * 42u;
                if (point_in_rect(mouse_x, mouse_y, panel_x + 34u, row_y, 632u, 34u)) {
                    state->install_target_index = index;
                    return true;
                }
            }
        }

        if (point_in_rect(mouse_x, mouse_y, panel_x + 34u, panel_y + 356u, 148u, OOBE_BUTTON_HEIGHT)) {
            state->setup_phase = SETUP_PHASE_NONE;
            return true;
        }

        if (point_in_rect(mouse_x, mouse_y, panel_x + 700u - 34u - 176u, panel_y + 356u, 176u, OOBE_BUTTON_HEIGHT)) {
            u32 kernel_size;
            u32 target_lba;
            u32 target_sectors;
            bool installed;

            if (!state->oobe_storage_ready) {
                return true;
            }
            if (!get_install_target(state, state->install_target_index, &target_lba, &target_sectors, NULL, NULL)) {
                return true;
            }
            if (!install_target_can_fit(state, boot_info, state->install_target_index)) {
                state->oobe_install_failed = true;
                return true;
            }

            kernel_size = (u32)(boot_info->kernel_end - boot_info->kernel_start);
            installed = installer_install_to_target((const void*)boot_info->kernel_start,
                                                    kernel_size,
                                                    (const void*)boot_info->initrd_start,
                                                    (u32)boot_info->initrd_size,
                                                    target_lba,
                                                    target_sectors);
            state->oobe_install_complete = installed;
            state->oobe_install_failed = !installed;
            state->oobe_disk_has_install = installed || state->oobe_disk_has_install;
            if (installed) {
                state->setup_phase = SETUP_PHASE_INSTALLING;
                state->setup_phase_started_second = current_second;
            }
            return true;
        }
    } else if (state->setup_phase == SETUP_PHASE_USER_OOBE) {
        panel_x = (vga_width() > 640u) ? (vga_width() - 640u) / 2u : 0u;
        panel_y = (vga_height() > 360u) ? (vga_height() - 360u) / 2u : 0u;

        if (point_in_rect(mouse_x, mouse_y, panel_x + 40u, panel_y + 138u, 560u, 34u)) {
            state->desktop_theme = (state->desktop_theme + 1u) % DESKTOP_THEME_COUNT;
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, panel_x + 40u, panel_y + 180u, 560u, 34u)) {
            state->window_theme = (state->window_theme + 1u) % WINDOW_THEME_COUNT;
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, panel_x + 40u, panel_y + 222u, 560u, 34u)) {
            state->transparency_enabled = !state->transparency_enabled;
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, panel_x + 40u, panel_y + 306u, 156u, OOBE_BUTTON_HEIGHT)) {
            state->desktop_theme = DESKTOP_THEME_WINDOWS;
            state->window_theme = WINDOW_THEME_WINDOWS;
            state->transparency_enabled = true;
            state->setup_phase = SETUP_PHASE_NONE;
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, panel_x + 640u - 40u - 176u, panel_y + 306u, 176u, OOBE_BUTTON_HEIGHT)) {
            state->setup_phase = SETUP_PHASE_NONE;
            return true;
        }
    }

    return false;
}

static bool update_setup_state(desktop_state_t* state, u32 current_second) {
    if (state == NULL) {
        return false;
    }

    if (state->setup_phase == SETUP_PHASE_INSTALLING &&
        seconds_since(state->setup_phase_started_second, current_second) >= SETUP_INSTALL_SECONDS) {
        state->setup_phase = SETUP_PHASE_CUSTOMIZING;
        state->setup_phase_started_second = current_second;
        return true;
    }

    if (state->setup_phase == SETUP_PHASE_CUSTOMIZING &&
        seconds_since(state->setup_phase_started_second, current_second) >= SETUP_CUSTOMIZE_SECONDS) {
        state->setup_phase = SETUP_PHASE_USER_OOBE;
        state->setup_phase_started_second = current_second;
        return true;
    }

    return false;
}

static bool handle_window_button_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_id_t order[] = {
        WINDOW_WELCOME,
        WINDOW_TASKS,
        WINDOW_FILES,
        WINDOW_TERMINAL,
        WINDOW_SETTINGS,
        WINDOW_SEARCH
    };

    if (state == NULL) {
        return false;
    }

    for (u32 index = 0; index < sizeof(order) / sizeof(order[0]); ++index) {
        window_id_t window = order[index];
        const window_rect_t* rect = get_window_rect_const(state, window);
        window_button_t button;

        if (!window_visible(state, window)) {
            continue;
        }

        button = hit_test_window_button(state, rect, mouse_x, mouse_y);
        if (button == WINDOW_BUTTON_NONE) {
            continue;
        }

        state->dragging_window = WINDOW_NONE;
        if (button == WINDOW_BUTTON_CLOSE) {
            close_window(state, window);
        } else if (button == WINDOW_BUTTON_MINIMIZE) {
            minimize_window(state, window);
        } else if (button == WINDOW_BUTTON_HIDE) {
            toggle_window_shade(state, window);
        }
        return true;
    }

    return false;
}

static bool update_dragged_window(desktop_state_t* state, const mouse_state_t* mouse) {
    window_rect_t* rect;
    s32 next_x;
    s32 next_y;

    if (state == NULL || mouse == NULL || state->dragging_window == WINDOW_NONE || !mouse->left_down) {
        return false;
    }

    rect = get_window_rect(state, state->dragging_window);
    if (rect == NULL) {
        return false;
    }

    next_x = (s32)mouse->x - state->drag_offset_x;
    next_y = (s32)mouse->y - state->drag_offset_y;

    if ((u32)next_x == rect->x && (u32)next_y == rect->y) {
        return false;
    }

    rect->x = (u32)next_x;
    rect->y = (u32)next_y;
    clamp_window_rect(rect);
    return true;
}

static void render_desktop(const image_t* wallpaper,
                           const boot_info_t* boot_info,
                           bool wallpaper_loaded,
                           bool ata_ready,
                           const desktop_state_t* state,
                           const mouse_state_t* mouse) {
    const window_id_t draw_order[] = {
        WINDOW_SEARCH,
        WINDOW_SETTINGS,
        WINDOW_TERMINAL,
        WINDOW_FILES,
        WINDOW_TASKS,
        WINDOW_WELCOME
    };

    if (setup_active(state)) {
        render_setup_screen(state, wallpaper, boot_info, rtc_read_seconds_of_day());
        vga_present();
        draw_mouse_cursor_front(mouse->x, mouse->y);
        return;
    }

    draw_desktop_background(state, wallpaper);
    draw_top_bar();

    for (u32 index = 0; index < sizeof(draw_order) / sizeof(draw_order[0]); ++index) {
        window_id_t window = draw_order[index];
        const window_rect_t* rect = get_window_rect_const(state, window);

        if (window == state->dragging_window || !window_visible(state, window)) {
            continue;
        }

        if (window == WINDOW_SEARCH) {
            draw_launcher_panel(state, rect);
        } else if (window == WINDOW_SETTINGS) {
            draw_settings_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_TERMINAL) {
            draw_terminal_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_FILES) {
            draw_file_manager_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_TASKS) {
            draw_task_manager_window(boot_info, wallpaper_loaded, ata_ready, state, rect, window_shaded(state, window));
        } else if (window == WINDOW_WELCOME) {
            draw_oobe_window(state, rect);
        }
    }

    if (state->dragging_window != WINDOW_NONE && window_visible(state, state->dragging_window)) {
        const window_rect_t* rect = get_window_rect_const(state, state->dragging_window);

        if (state->dragging_window == WINDOW_SEARCH) {
            draw_launcher_panel(state, rect);
        } else if (state->dragging_window == WINDOW_SETTINGS) {
            draw_settings_window(state, rect, window_shaded(state, state->dragging_window));
        } else if (state->dragging_window == WINDOW_TERMINAL) {
            draw_terminal_window(state, rect, window_shaded(state, state->dragging_window));
        } else if (state->dragging_window == WINDOW_FILES) {
            draw_file_manager_window(state, rect, window_shaded(state, state->dragging_window));
        } else if (state->dragging_window == WINDOW_TASKS) {
            draw_task_manager_window(boot_info, wallpaper_loaded, ata_ready, state, rect,
                                     window_shaded(state, state->dragging_window));
        } else if (state->dragging_window == WINDOW_WELCOME) {
            draw_oobe_window(state, rect);
        }
    }

    draw_dock(state, mouse);
    draw_preview_label();
    vga_present();
    draw_mouse_cursor_front(mouse->x, mouse->y);
}

static void refresh_desktop_chrome(const desktop_state_t* state, const mouse_state_t* mouse) {
    dock_layout_t layout;
    u32 preview_y = vga_height() - 68u;

    draw_top_bar();
    draw_dock(state, mouse);
    draw_preview_label();

    vga_present_region(0, 0, vga_width(), TOP_BAR_HEIGHT);
    get_dock_layout(&layout);
    vga_present_region(layout.dock_x, layout.dock_y, layout.dock_width, layout.dock_height);
    vga_present_region(26u, preview_y, text_width(PREVIEW_LABEL_TEXT), 8u);
    draw_mouse_cursor_front(mouse->x, mouse->y);
}

static void render_boot_log_screen(void) {
    static const char* ascii_art[] = {
        "JJJJ   AA   BBBB  U  U  L      OOO   SSS ",
        "  JJ  A  A  B   B U  U  L     O   O S   ",
        "  JJ  AAAA  BBBB  U  U  L     O   O  SSS",
        "J JJ  A  A  B   B U  U  L     O   O    S",
        " JJJ  A  A  BBBB   UU   LLLLL  OOO  SSS "
    };
    u32 height = vga_height();
    u32 start_line;
    u32 visible_lines;
    u32 log_y;

    vga_clear(vga_color(7, 10, 16));
    draw_text_centered(96, ascii_art[0], vga_color(196, 220, 255));
    draw_text_centered(112, ascii_art[1], vga_color(196, 220, 255));
    draw_text_centered(128, ascii_art[2], vga_color(196, 220, 255));
    draw_text_centered(144, ascii_art[3], vga_color(196, 220, 255));
    draw_text_centered(160, ascii_art[4], vga_color(196, 220, 255));
    draw_text_centered(208, "BOOT LOG", vga_color(120, 166, 232));

    visible_lines = (height > 300u) ? ((height - 300u) / 12u) : 10u;
    if (visible_lines > BOOT_LOG_MAX_LINES) {
        visible_lines = BOOT_LOG_MAX_LINES;
    }
    start_line = (g_boot_log_count > visible_lines) ? (g_boot_log_count - visible_lines) : 0u;
    log_y = 244u;

    for (u32 index = start_line; index < g_boot_log_count; ++index) {
        draw_text(72, log_y, g_boot_log_lines[index], vga_color(205, 214, 228));
        log_y += 12u;
    }

    vga_present();
}

void gui_boot_log_reset(void) {
    memset(g_boot_log_lines, 0, sizeof(g_boot_log_lines));
    g_boot_log_count = 0;
    render_boot_log_screen();
}

void gui_boot_log_append(const char* text) {
    if (g_boot_log_count < BOOT_LOG_MAX_LINES) {
        copy_boot_log_line(g_boot_log_lines[g_boot_log_count], text);
        ++g_boot_log_count;
    } else {
        memmove(g_boot_log_lines, g_boot_log_lines[1], sizeof(g_boot_log_lines[0]) * (BOOT_LOG_MAX_LINES - 1u));
        copy_boot_log_line(g_boot_log_lines[BOOT_LOG_MAX_LINES - 1u], text);
    }

    render_boot_log_screen();
}

void gui_show_boot_sequence(void) {
    render_boot_log_screen();
}

void gui_run_desktop(const image_t* wallpaper,
                     const boot_info_t* boot_info,
                     bool wallpaper_loaded,
                     bool ata_ready) {
    desktop_state_t state;
    mouse_state_t mouse;
    bool scene_dirty = true;
    u32 last_clock_second;
    u32 previous_cursor_x;
    u32 previous_cursor_y;

    last_clock_second = rtc_read_seconds_of_day();
    desktop_initialize(&state, ata_ready, boot_info != NULL && boot_info->initrd_start != 0, last_clock_second);
    mouse_initialize(&mouse);
    previous_cursor_x = mouse.x;
    previous_cursor_y = mouse.y;

    for (;;) {
        bool input_changed;
        bool clock_changed;
        bool mouse_moved;
        bool mouse_pressed;
        bool mouse_released;
        bool previous_down = mouse.left_down;
        dock_app_t clicked_app;
        u32 current_clock_second;

        input_changed = poll_input(&state, &mouse);
        current_clock_second = rtc_read_seconds_of_day();
        clock_changed = current_clock_second != last_clock_second;
        mouse_moved = mouse.x != previous_cursor_x || mouse.y != previous_cursor_y;
        mouse_pressed = mouse.left_down && !previous_down;
        mouse_released = !mouse.left_down && previous_down;

        if (update_setup_state(&state, current_clock_second)) {
            scene_dirty = true;
        }
        if (setup_active(&state) && clock_changed) {
            scene_dirty = true;
        }

        if (mouse_pressed) {
            if (setup_active(&state)) {
                if (handle_setup_click(&state, boot_info, current_clock_second, mouse.x, mouse.y)) {
                    scene_dirty = true;
                }
            } else if (dock_hit_test(mouse.x, mouse.y, &clicked_app)) {
                if (toggle_dock_app(&state, clicked_app)) {
                    scene_dirty = true;
                }
            } else if (handle_window_button_click(&state, mouse.x, mouse.y)) {
                scene_dirty = true;
            } else if (handle_settings_click(&state, mouse.x, mouse.y)) {
                scene_dirty = true;
            } else if (handle_launcher_click(&state, mouse.x, mouse.y)) {
                scene_dirty = true;
            } else if (handle_oobe_click(&state, boot_info, mouse.x, mouse.y)) {
                scene_dirty = true;
            } else {
                window_id_t target = find_draggable_window(&state, mouse.x, mouse.y);
                window_rect_t* rect = get_window_rect(&state, target);

                if (target != WINDOW_NONE && rect != NULL) {
                    state.dragging_window = target;
                    state.drag_offset_x = (s32)mouse.x - (s32)rect->x;
                    state.drag_offset_y = (s32)mouse.y - (s32)rect->y;
                    scene_dirty = true;
                }
            }
        }

        if (!setup_active(&state) && update_dragged_window(&state, &mouse)) {
            scene_dirty = true;
        }

        if (!setup_active(&state) && mouse_released && state.dragging_window != WINDOW_NONE) {
            state.dragging_window = WINDOW_NONE;
            scene_dirty = true;
        }

        if (input_changed && !mouse_moved && !mouse_pressed && !mouse_released) {
            scene_dirty = true;
        }

        if (scene_dirty) {
            render_desktop(wallpaper, boot_info, wallpaper_loaded, ata_ready, &state, &mouse);
            scene_dirty = false;
            last_clock_second = current_clock_second;
            previous_cursor_x = mouse.x;
            previous_cursor_y = mouse.y;
            continue;
        }

        if (mouse_moved) {
            refresh_cursor_only(previous_cursor_x, previous_cursor_y, &mouse);
            previous_cursor_x = mouse.x;
            previous_cursor_y = mouse.y;
        }

        if (!setup_active(&state) && clock_changed) {
            refresh_desktop_chrome(&state, &mouse);
            last_clock_second = current_clock_second;
            previous_cursor_x = mouse.x;
            previous_cursor_y = mouse.y;
        }
    }
}
