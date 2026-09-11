// This is the main file for desktop, apps, etc
// Have fun working on jabul
#include "jabulos.h"

#define JABULOS_BUILD_VERSION 28205u

#define FILE_MANAGER_MAX_ENTRIES 128u
#define FILE_MANAGER_NAME_LENGTH 80u
#define FILE_MANAGER_DETAIL_LENGTH 96u
#define FILE_MANAGER_PATH_LENGTH 160u
#define FILE_MANAGER_STATUS_LENGTH 120u

#define USER_FILES_TABLE_LBA 20100u
#define USER_FILES_TABLE_SECTORS 4u
#define USER_FILES_MAX 16u
#define USER_FILES_DATA_START_LBA 20200u
#define USER_FILES_SECTORS_PER_FILE 4u 
#define USER_FILES_MAGIC 0x4A425546 // "JBUF"

typedef struct {
    char name[FILE_MANAGER_NAME_LENGTH];
    u32 lba;
    u32 size;
    bool active;
} user_file_entry_t;

typedef struct {
    u32 magic;
    user_file_entry_t entries[USER_FILES_MAX];
} user_file_table_t;

static user_file_table_t g_user_files = {};

typedef enum {
    APP_START = 0,
    APP_SETTINGS = 1,
    APP_TERMINAL = 2,
    APP_NOTEPAD = 3,
    APP_FILES = 4,
    APP_TASKS = 5,
    APP_BROWSER = 6,
    APP_VIDEOS = 7,
    APP_SNAKE = 8,
    APP_JABVER = 9
} dock_app_t;

typedef enum {
    WINDOW_NONE = 0,
    WINDOW_SEARCH = 1,
    WINDOW_RUN = 2,
    WINDOW_SETTINGS = 3,
    WINDOW_TERMINAL = 4,
    WINDOW_NOTEPAD = 5,
    WINDOW_FILES = 6,
    WINDOW_TASKS = 7,
    WINDOW_VIDEOS = 8,
    WINDOW_BROWSER = 9,
    WINDOW_GAMES = 10,
    WINDOW_SNAKE = 11,
    WINDOW_JABVER = 12,
    WINDOW_WELCOME = 13,
    WINDOW_LEGAL = 14
} window_id_t;

typedef enum {
    BROWSER_PAGE_WEB = 0,
    BROWSER_PAGE_SOURCE = 1,
    BROWSER_PAGE_DOWNLOADS = 2,
    BROWSER_PAGE_ABOUT = 3
} browser_page_t;

typedef enum {
    GAMES_SCREEN_HOME = 0,
    GAMES_SCREEN_SNAKE = 1,
    GAMES_SCREEN_TICTACTOE = 2
} games_screen_t;

typedef enum {
    POWER_ACTION_NONE = 0,
    POWER_ACTION_SHUTDOWN = 1,
    POWER_ACTION_RESTART = 2
} power_action_t;

typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} window_rect_t;

#define DIRTY_RECT_MAX 24u

typedef struct {
    window_rect_t rects[DIRTY_RECT_MAX];
    u32 count;
} dirty_rect_list_t;

#define VIDEOS_MAX_ITEMS 64u
#define VIDEOS_MAX_FRAMES 999u
#define MEDIA_TOKEN_MAX 40u
#define AUDIO_ASSET_MAGIC "JBAUD01"
#define AUDIO_ASSET_MAGIC_LENGTH 7u
#define AUDIO_SAMPLE_MAX_RATE 16000u
#define AUDIO_SB16_CATCHUP_LIMIT 128u
#define STARTUP_SOUND_ASSET "assets/startup.wav"
#define BROWSER_ADDRESS_LENGTH NET_HTTP_URL_MAX
#define BROWSER_STATUS_LENGTH 96u
#define BROWSER_TITLE_LENGTH 64u
#define BROWSER_PAGE_TEXT_LENGTH 3072u
#define BROWSER_DOWNLOAD_TEXT_LENGTH 2048u

typedef struct __attribute__((packed)) {
    char magic[8];
    u32 sample_rate;
    u32 sample_count;
    u32 reserved;
} audio_asset_header_t;

typedef enum {
    MEDIA_ITEM_IMAGE = 0,
    MEDIA_ITEM_MP4 = 1,
    MEDIA_ITEM_MP3 = 2
} media_item_type_t;

typedef struct {
    bool show_search;
    bool show_run;
    bool show_settings;
    bool show_terminal;
    bool show_notepad;
    bool show_files;
    bool show_tasks;
    bool show_videos;
    bool show_browser;
    bool show_games;
    bool show_snake;
    bool show_jabver;
    bool show_welcome;
    bool show_legal;
    bool minimized_search;
    bool minimized_run;
    bool minimized_settings;
    bool minimized_terminal;
    bool minimized_notepad;
    bool minimized_files;
    bool minimized_tasks;
    bool minimized_videos;
    bool minimized_browser;
    bool minimized_games;
    bool minimized_snake;
    bool minimized_jabver;
    bool minimized_welcome;
    bool minimized_legal;
    bool shaded_search;
    bool shaded_run;
    bool shaded_settings;
    bool shaded_terminal;
    bool shaded_notepad;
    bool shaded_files;
    bool shaded_tasks;
    bool shaded_videos;
    bool shaded_browser;
    bool shaded_games;
    bool shaded_snake;
    bool shaded_jabver;
    bool shaded_welcome;
    bool shaded_legal;
    window_rect_t search_rect;
    window_rect_t run_rect;
    window_rect_t settings_rect;
    window_rect_t terminal_rect;
    window_rect_t notepad_rect;
    window_rect_t files_rect;
    window_rect_t tasks_rect;
    window_rect_t videos_rect;
    window_rect_t browser_rect;
    window_rect_t games_rect;
    window_rect_t snake_rect;
    window_rect_t jabver_rect;
    window_rect_t welcome_rect;
    window_rect_t legal_rect;
    window_rect_t fullscreen_restore_rects[15];
    bool window_fullscreen[15];
    window_id_t window_order[14];
    window_id_t dragging_window;
    s32 drag_offset_x;
    s32 drag_offset_y;
    char launcher_query[32];
    bool launcher_power_menu_open;
    char run_query[32];
    char notepad_buffer[2048];
    u32 notepad_cursor;
    bool notepad_dirty;
    const image_t* boot_splash_image;
    const image_t* splash_logo_image;
    bool oobe_storage_ready;
    bool oobe_disk_has_install;
    bool oobe_install_failed;
    bool oobe_install_complete;
    bool oobe_install_write_started;
    u32 oobe_stage;
    u32 desktop_theme;
    u32 window_theme;
    bool transparency_enabled;
    u32 taskbar_style;
    u32 wallpaper_theme;
    bool live_media_boot;
    u32 desktop_started_second;
    u32 setup_phase;
    u32 setup_phase_started_second;
    u64 setup_phase_started_ms;
    u32 install_state_stage;
    u32 install_state_target_lba;
    u32 install_state_target_sectors;
    u32 auth_field_focus;
    char account_username[32];
    char account_password[32];
    char account_input_username[32];
    char account_input_password[32];
    char login_password_input[32];
    bool login_error;
    bool lockscreen_prompt_visible;
    const image_t* lockscreen_background;
    const image_t* lockscreen_profile;
    u64 lockscreen_prompt_started_ms;
    u32 settings_page;
    u32 task_manager_page;
    ata_device_info_t storage_info;
    u32 install_target_index;
    bool pe_lab_available;
    bool pe_lab_attempted;
    pe_image_info_t pe_lab_image;
    pe_launch_result_t pe_lab_result;
    u32 browser_page;
    u32 games_screen;
    u32 videos_item_count;
    const char* videos_item_name[VIDEOS_MAX_ITEMS];
    media_item_type_t videos_item_type[VIDEOS_MAX_ITEMS];
    u32 videos_item_frame_count[VIDEOS_MAX_ITEMS];
    u32 videos_selected_item;
    bool videos_playing;
    u64 videos_play_start_tsc;
    u32 videos_last_frame_index;
    u32 videos_active_frame_count;
    const void* videos_active_frame_data[VIDEOS_MAX_FRAMES];
    u32 videos_active_frame_size[VIDEOS_MAX_FRAMES];
    const u8* videos_audio_data;
    u32 videos_audio_sample_count;
    u32 videos_audio_sample_rate;
    u32 videos_audio_last_sample_index;
    bool videos_sb16_available;
    bool videos_audio_use_sb16;
    const u8* startup_audio_data;
    u32 startup_audio_sample_count;
    u32 startup_audio_sample_rate;
    u32 startup_audio_last_sample_index;
    u64 startup_audio_start_tsc;
    bool startup_audio_armed;
    bool startup_audio_playing;
    bool startup_audio_played;
    u8 snake_x[48];
    u8 snake_y[48];
    u32 snake_length;
    s32 snake_dir_x;
    s32 snake_dir_y;
    u8 snake_food_x;
    u8 snake_food_y;
    u32 snake_score;
    bool snake_game_over;
    u8 ttt_cells[9];
    u8 ttt_turn;
    u8 ttt_winner;
    bool ttt_draw;
    bool ctrl_down;
    bool shift_down;
    bool left_super_down;
    bool right_super_down;
    bool crash_hotkey_triggered;
    power_action_t power_action;
    u32 power_action_started_second;
    char cpu_vendor[13];
    char cpu_brand[49];
    u32 cpu_family;
    u32 cpu_model;
    u32 cpu_stepping;
    u32 cpu_clock_mhz;
    u32 cpu_benchmark_score;
    u32 cpu_benchmark_duration;
    u32 cpu_last_benchmark_second;
    bool cpu_ready;

   // update service, leave that alone its dead now and not working
    bool update_available;
    bool update_prompt_open;
    u32 update_version;
    char update_label[64];
    u32 update_prompt_dismissed_version;
    u64 update_notification_started_ms;
    bool update_installing;
    bool update_install_complete;
    bool update_install_failed;

 // context menu which is also not working much rn
    bool context_menu_open;
    u32 context_menu_x;
    u32 context_menu_y;
    s32 context_menu_target_entry; 
    bool context_menu_renaming;
    char context_menu_rename_buffer[FILE_MANAGER_NAME_LENGTH];
    s32 context_menu_clipboard_entry; 

    s32 notepad_user_file_index; 
} desktop_state_t;

typedef mouse_driver_state_t mouse_state_t;

typedef struct {
    u32 dock_x;
    u32 dock_y;
    u32 dock_width;
    u32 dock_height;
    u32 icon_size;
    u32 style;
} dock_layout_t;

typedef struct {
    char address[NET_HTTP_URL_MAX];
    char status[96];
    char title[64];
    char content_type[NET_HTTP_CONTENT_TYPE_MAX];
    char page_text[3072];
    char download_text[2048];
    char download_url[NET_HTTP_URL_MAX];
    u32 page_length;
    u32 download_length;
    u16 last_status_code;
    bool page_truncated;
    bool download_truncated;
    u32 scroll_lines[4];
} browser_runtime_t;

#define BOOT_LOG_MAX_LINES 24u
#define BOOT_LOG_LINE_LENGTH 72u
#define CURSOR_WIDTH 32u
#define CURSOR_HEIGHT 32u
#define CURSOR_HOTSPOT_X 4u
#define CURSOR_HOTSPOT_Y 3u
#define CURSOR_COMPOSITE_MAX_WIDTH 256u
#define CURSOR_COMPOSITE_MAX_HEIGHT 256u
#define CURSOR_SURFACE_BUFFER_SIZE (CURSOR_COMPOSITE_MAX_WIDTH * CURSOR_COMPOSITE_MAX_HEIGHT * 4u)
#define TOP_BAR_HEIGHT 40u
#define WINDOW_CORNER_RADIUS 28u
#define WINDOW_CONTENT_RADIUS 22u
#define WINDOW_BUTTON_SIZE 16u
#define WINDOW_BUTTON_SPACING 8u
#define WINDOW_TITLEBAR_HEIGHT 38u
#define WINDOW_STATE_COUNT 15u
#define WINDOW_COMPOSITE_PADDING 24u
#define LAUNCHER_APP_COUNT 8u
#define RUN_APP_COUNT 9u
#define DOCK_APP_MAX 10u
#define DESKTOP_SHORTCUT_COUNT 1u
#define OOBE_BUTTON_WIDTH 144u
#define OOBE_BUTTON_HEIGHT 34u
#define PE_RUN_BUTTON_WIDTH 112u
#define POWER_ACTION_DELAY_SECONDS 5u
#define SETUP_INSTALL_COPY_SECONDS 20u
#define SETUP_INSTALL_WRITE_SECONDS 20u
#define SETUP_INSTALL_SECONDS (SETUP_INSTALL_COPY_SECONDS + SETUP_INSTALL_WRITE_SECONDS)
#define SETUP_OOBE_HI_SECONDS 2u
#define SETUP_OOBE_SETTING_UP_SECONDS 4u
#define SETUP_OOBE_PREPARE_SECONDS 30u
#define SETUP_OOBE_ALMOST_THERE_SECONDS 2u
#define AUTH_FIELD_USERNAME 0u
#define AUTH_FIELD_PASSWORD 1u
#define SNAKE_GRID_WIDTH 12u
#define SNAKE_GRID_HEIGHT 9u
#define SNAKE_CELL_SIZE 18u
#define WINDOW_STACK_SIZE 14u
#define LAUNCHER_RECOMMENDED_COUNT 2u

static const char* PREVIEW_LABEL_TEXT = " JabulOS desktop ";
static browser_runtime_t g_browser = {};
static char g_browser_transfer_buffer[BROWSER_PAGE_TEXT_LENGTH] = {};
typedef enum {
    FILE_MANAGER_SIDEBAR_DOWNLOADS = 0,
    FILE_MANAGER_SIDEBAR_DOCUMENTS = 1,
    FILE_MANAGER_SIDEBAR_MUSIC = 2,
    FILE_MANAGER_SIDEBAR_PHOTOS = 3,
    FILE_MANAGER_SIDEBAR_THIS_PC = 4
} file_manager_sidebar_t;

typedef enum {
    FILE_MANAGER_SOURCE_NONE = 0,
    FILE_MANAGER_SOURCE_LIVE_MEDIA = 1,
    FILE_MANAGER_SOURCE_PARTITION_ARCHIVE = 2,
    FILE_MANAGER_SOURCE_UNSUPPORTED_PARTITION = 3
} file_manager_source_t;

typedef struct {
    char name[FILE_MANAGER_NAME_LENGTH];
    char detail[FILE_MANAGER_DETAIL_LENGTH];
    char path[FILE_MANAGER_PATH_LENGTH];
    u32 size;
    u32 source_kind;
    u32 partition_index;
    bool directory;
    bool image_file;
    bool source_entry;
    bool browsable;
} file_manager_entry_t;

typedef struct __attribute__((packed)) {
    char magic[8];
    u32 version;
    u32 kernel_lba;
    u32 kernel_size;
    u32 kernel_sectors;
    u32 initrd_lba;
    u32 initrd_size;
    u32 initrd_sectors;
    u32 total_sectors;
    u8 reserved[476];
} file_manager_install_header_t;

typedef struct {
    u32 sidebar;
    u32 scroll;
    u32 selected_entry;
    u32 entry_count;
    u32 source_kind;
    u32 source_partition_index;
    bool show_this_pc_root;
    u8* archive_buffer;
    u32 archive_size;
    bool archive_owned;
    char source_label[FILE_MANAGER_NAME_LENGTH];
    char path[FILE_MANAGER_PATH_LENGTH];
    char status[FILE_MANAGER_STATUS_LENGTH];
    const void* preview_data;
    u32 preview_size;
    char preview_name[FILE_MANAGER_PATH_LENGTH];
    bool audio_error_popup;
    file_manager_entry_t entries[FILE_MANAGER_MAX_ENTRIES];
} file_manager_runtime_t;

typedef struct {
    u32 nav_x;
    u32 nav_y;
    u32 nav_width;
    u32 body_y;
    u32 body_height;
    u32 content_x;
    u32 content_width;
    u32 rows_y;
    u32 row_height;
    u32 rows_visible;
} file_manager_layout_t;

static file_manager_runtime_t g_file_manager = {}; // apps 
static const char* g_launcher_app_names[LAUNCHER_APP_COUNT] = {
    "Settings",
    "Terminal",
    "File Explorer",
    "Task Manager",
    "Notepad",
    "Jachron",
    "Snake",
    "Jabver"
};
static const window_id_t g_launcher_window_ids[LAUNCHER_APP_COUNT] = {
    WINDOW_SETTINGS,
    WINDOW_TERMINAL,
    WINDOW_FILES,
    WINDOW_TASKS,
    WINDOW_NOTEPAD,
    WINDOW_BROWSER,
    WINDOW_SNAKE,
    WINDOW_JABVER
};
static const dock_app_t g_launcher_dock_apps[LAUNCHER_APP_COUNT] = {
    APP_SETTINGS,
    APP_TERMINAL,
    APP_FILES,
    APP_TASKS,
    APP_NOTEPAD,
    APP_BROWSER,
    APP_SNAKE,
    APP_JABVER
};
static const u32 g_launcher_recommended_indices[LAUNCHER_RECOMMENDED_COUNT] = {
    0u,
    4u
};
static const char* g_run_app_names[RUN_APP_COUNT] = {
    "Settings",
    "Terminal",
    "File Explorer",
    "Task Manager",
    "Notepad",
    "Jachron",
    "Games",
    "Snake",
    "Jabver"
};
static const window_id_t g_run_window_ids[RUN_APP_COUNT] = {
    WINDOW_SETTINGS,
    WINDOW_TERMINAL,
    WINDOW_FILES,
    WINDOW_TASKS,
    WINDOW_NOTEPAD,
    WINDOW_BROWSER,
    WINDOW_GAMES,
    WINDOW_SNAKE,
    WINDOW_JABVER
};

static u32 window_title_height(window_id_t window);
static void clamp_window_rect(window_rect_t* rect);
static window_rect_t* get_window_rect(desktop_state_t* state, window_id_t window);
static void position_run_dialog(desktop_state_t* state);
static void position_search_panel(desktop_state_t* state);
static bool window_visible(const desktop_state_t* state, window_id_t window);
static bool window_shaded(const desktop_state_t* state, window_id_t window);
static bool dock_app_active(const desktop_state_t* state, dock_app_t app);
static u32 get_visible_dock_apps(const desktop_state_t* state, dock_app_t* apps_out);
static bool is_dock_app_default(dock_app_t app);
static void draw_oobe_button(const desktop_state_t* state, u32 x, u32 y, u32 width, const char* label, bool primary);
static void launcher_row_rect(const window_rect_t* rect,
                              u32 row_index,
                              u32* out_x,
                              u32* out_y,
                              u32* out_width,
                              u32* out_height);
static void launcher_app_tile_rect(const window_rect_t* rect,
                                   u32 app_index,
                                   u32* out_x,
                                   u32* out_y,
                                   u32* out_width,
                                   u32* out_height);
static void launcher_recommended_tile_rect(const window_rect_t* rect,
                                           u32 item_index,
                                           u32* out_x,
                                           u32* out_y,
                                           u32* out_width,
                                           u32* out_height);
static void launcher_power_button_rect(const window_rect_t* rect,
                                       u32* out_x,
                                       u32* out_y,
                                       u32* out_width,
                                       u32* out_height);
static void launcher_power_menu_rect(const window_rect_t* rect,
                                       u32* out_x,
                                       u32* out_y,
                                       u32* out_width,
                                       u32* out_height);

static void user_files_load_table(const desktop_state_t* state);
static void user_files_save_table(const desktop_state_t* state);
static void draw_context_menu(const desktop_state_t* state, const mouse_state_t* mouse);
static bool handle_context_menu_click(desktop_state_t* state, const boot_info_t* boot_info, u32 mouse_x, u32 mouse_y);
static void get_desktop_shortcut_rect(u32 shortcut_index, u32* out_x, u32* out_y, u32* out_width, u32* out_height);
static void draw_desktop_shell(const image_t* wallpaper, const desktop_state_t* state, const mouse_state_t* mouse);
static void draw_desktop_windows(const image_t* wallpaper,
                                 const boot_info_t* boot_info,
                                 bool wallpaper_loaded,
                                 bool ata_ready,
                                 const desktop_state_t* state,
                                 const dirty_rect_list_t* dirty_rects);
static void draw_notepad_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded);
static void notepad_save_to_disk(desktop_state_t* state);
static void notepad_load_from_disk(desktop_state_t* state);
static void videos_stop_playback(desktop_state_t* state);
static void videos_refresh_media_list(desktop_state_t* state, const boot_info_t* boot_info);
static bool videos_prepare_selected_media(desktop_state_t* state, const boot_info_t* boot_info);
static bool videos_get_preview_present_rect(const desktop_state_t* state, window_rect_t* out_rect);
static void videos_audio_tick(desktop_state_t* state);
static void file_manager_initialize(const desktop_state_t* state, const boot_info_t* boot_info);
static void file_manager_refresh_entries(const desktop_state_t* state, const boot_info_t* boot_info);
static void file_manager_close_preview(desktop_state_t* state);
static void file_manager_set_status(const char* message);
static bool file_manager_get_archive_view(const boot_info_t* boot_info, const void** out_archive, u32* out_size);
static bool file_manager_open_source(const desktop_state_t* state,
                                     const boot_info_t* boot_info,
                                     u32 source_kind,
                                     u32 partition_index);
static void open_window(desktop_state_t* state, window_id_t window);
static void startup_sound_reset(desktop_state_t* state);
static bool startup_sound_load(desktop_state_t* state, const boot_info_t* boot_info);
static bool startup_sound_start(desktop_state_t* state);
static void startup_sound_tick(desktop_state_t* state);
static void render_power_action_screen(power_action_t action);
static void begin_power_action(desktop_state_t* state, power_action_t action, u32 current_second);
static void update_power_action_state(desktop_state_t* state, u32 current_second);
static bool update_setup_state(desktop_state_t* state, const boot_info_t* boot_info, u32 current_second);
static bool setup_active(const desktop_state_t* state);
static void set_setup_phase(desktop_state_t* state, u32 phase, u32 current_second);
static bool handle_setup_keyboard_scancode(desktop_state_t* state, u8 scancode);
static void browser_reset_scroll(u32 page);
static bool browser_load_url(const char* url);
static void close_window(desktop_state_t* state, window_id_t window);
static bool prepare_desktop_shell_cache(const image_t* wallpaper,
                                        const desktop_state_t* state,
                                        const mouse_state_t* mouse,
                                        u32 current_clock_second);
static bool desktop_shell_cache_is_current(const desktop_state_t* state, u32 current_clock_second);
static void restore_desktop_shell_cache_regions(const dirty_rect_list_t* dirty_rects);
static bool expand_rect_with_padding(const window_rect_t* rect, u32 padding, window_rect_t* out_rect);
static bool get_window_composite_rect(const desktop_state_t* state, window_id_t window, window_rect_t* out_rect);
static bool get_top_visible_window_composite_rect(const desktop_state_t* state, window_rect_t* out_rect);
static bool dirty_rect_list_intersects_rect(const dirty_rect_list_t* list, const window_rect_t* rect);
static void initialize_theme_image_cache(const boot_info_t* boot_info, const image_t* default_wallpaper);
static void initialize_app_icon_cache(const boot_info_t* boot_info);
static void render_dirty_window_region(const image_t* wallpaper,
                                       const boot_info_t* boot_info,
                                       bool wallpaper_loaded,
                                       bool ata_ready,
                                       const desktop_state_t* state,
                                       const mouse_state_t* mouse,
                                       const window_rect_t* dirty_rect);
static char g_boot_log_lines[BOOT_LOG_MAX_LINES][BOOT_LOG_LINE_LENGTH];
static u32 g_boot_log_count = 0;
static u8* g_desktop_shell_cache = NULL;
static u32 g_desktop_shell_cache_size = 0;
static u32 g_desktop_shell_clock_second = 0xFFFFFFFFu;
static u32 g_desktop_shell_theme = 0xFFFFFFFFu;
static u32 g_desktop_shell_wallpaper_theme = 0xFFFFFFFFu;
static u32 g_desktop_shell_dock_mask = 0u;
static image_t g_theme_images[4u];
static bool g_theme_image_loaded[4u] = {};
static bool g_theme_image_cache_ready = false;
static image_t g_app_icon_images[DOCK_APP_MAX];
static bool g_app_icon_loaded[DOCK_APP_MAX] = {};
static bool g_app_icon_cache_ready = false;

typedef enum {
    WINDOW_BUTTON_NONE = 0,
    WINDOW_BUTTON_CLOSE = 1,
    WINDOW_BUTTON_FULLSCREEN = 2
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
    DESKTOP_THEME_JABULXP = 0,
    DESKTOP_THEME_JABULXP_DARK = 1,
    DESKTOP_THEME_COUNT = 2
} desktop_theme_t;

typedef enum {
    TASKBAR_STYLE_MACXWIN = 0,
    TASKBAR_STYLE_TENUI = 1,
    TASKBAR_STYLE_COUNT = 2
} taskbar_style_t;

typedef enum {
    WINDOW_THEME_WINDOWS = 0,
    WINDOW_THEME_MAC = 1,
    WINDOW_THEME_COUNT = 2
} chrome_theme_t;

static u32 seconds_since(u32 earlier, u32 current);

static u8* allocate_surface_buffer(u32 size) {
    u32 frames = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    u64 first_frame = 0;

    if (size == 0u) {
        return NULL;
    }

    for (u32 index = 0; index < frames; ++index) {
        u64 frame = pmm_alloc_frame();

        if (frame == 0) {
            for (u32 release = 0; release < index; ++release) {
                pmm_free_frame(first_frame + (u64)release * PAGE_SIZE);
            }
            return NULL;
        }

        if (index == 0u) {
            first_frame = frame;
        } else if (frame != first_frame + (u64)index * PAGE_SIZE) {
            for (u32 release = 0; release < index; ++release) {
                pmm_free_frame(first_frame + (u64)release * PAGE_SIZE);
            }
            pmm_free_frame(frame);
            return NULL;
        }
    }

    memset((void*)(u64)first_frame, 0, size);
    return (u8*)(u64)first_frame;
}

static void free_surface_buffer(u8* buffer, u32 size) {
    u32 frames;

    if (buffer == NULL || size == 0u) {
        return;
    }

    frames = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    for (u32 index = 0; index < frames; ++index) {
        pmm_free_frame((u64)buffer + (u64)index * PAGE_SIZE);
    }
}

#define INSTALL_STATE_MAGIC "JBASYS1"

typedef enum {
    INSTALL_STATE_STAGE_NONE = 0,
    INSTALL_STATE_STAGE_PENDING_INSTALL = 1,
    INSTALL_STATE_STAGE_NEEDS_OOBE = 2,
    INSTALL_STATE_STAGE_READY = 3
} install_state_stage_t;

typedef struct __attribute__((packed)) {
    char magic[8];
    u32 version;
    u32 stage;
    u32 target_lba;
    u32 target_sector_count;
    char username[32];
    char password[32];
    u8 reserved[424];
} install_state_sector_t;

static bool read_disk_blob(u32 lba, u32 size, void* destination) {
    u8* out = (u8*)destination;
    u32 sectors = (size + 511u) / 512u;
    u32 offset = 0u;

    if (destination == NULL || size == 0u) {
        return false;
    }

    while (sectors != 0u) {
        u8 chunk = (sectors > 128u) ? 128u : (u8)sectors;
        if (!ata_pio_read_sectors(lba, chunk, out + offset)) {
            return false;
        }
        lba += chunk;
        offset += (u32)chunk * 512u;
        sectors -= chunk;
    }

    return true;
}

static u32 install_state_sector_lba(u32 target_lba, u32 target_sector_count) {
    if (target_sector_count < 2u) {
        return 0u;
    }
    return target_lba + target_sector_count - 2u;
}

static bool install_state_sector_valid(const install_state_sector_t* sector) {
    return sector != NULL &&
           memcmp(sector->magic, INSTALL_STATE_MAGIC, 7) == 0 &&
           sector->target_sector_count >= 2u;
}

static bool read_install_state_sector(u32 sector_lba, install_state_sector_t* out_sector) {
    install_state_sector_t sector;

    if (sector_lba == 0u || out_sector == NULL || !ata_pio_read_sectors(sector_lba, 1, &sector)) {
        return false;
    }

    if (!install_state_sector_valid(&sector)) {
        return false;
    }

    *out_sector = sector;
    out_sector->magic[7] = '\0';
    out_sector->username[sizeof(out_sector->username) - 1u] = '\0';
    out_sector->password[sizeof(out_sector->password) - 1u] = '\0';
    return true;
}

static bool write_install_state_sector(const install_state_sector_t* sector) {
    u32 sector_lba;

    if (!install_state_sector_valid(sector)) {
        return false;
    }

    sector_lba = install_state_sector_lba(sector->target_lba, sector->target_sector_count);
    if (sector_lba == 0u) {
        return false;
    }

    return ata_pio_write_sectors(sector_lba, 1, sector);
}

static bool build_install_state_sector(u32 stage,
                                       u32 target_lba,
                                       u32 target_sector_count,
                                       const char* username,
                                       const char* password,
                                       install_state_sector_t* out_sector) {
    if (out_sector == NULL || target_sector_count < 2u) {
        return false;
    }

    memset(out_sector, 0, sizeof(*out_sector));
    memcpy(out_sector->magic, INSTALL_STATE_MAGIC, 7);
    out_sector->version = 1u;
    out_sector->stage = stage;
    out_sector->target_lba = target_lba;
    out_sector->target_sector_count = target_sector_count;
    if (username != NULL) {
        strcpy(out_sector->username, username);
    }
    if (password != NULL) {
        strcpy(out_sector->password, password);
    }
    return true;
}

static bool find_pending_install_state(const ata_device_info_t* storage_info, install_state_sector_t* out_sector) {
    install_state_sector_t sector;

    if (storage_info == NULL || out_sector == NULL || !storage_info->present || storage_info->total_sectors < 2u) {
        return false;
    }

    for (u32 index = 0; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
        const ata_partition_info_t* partition = &storage_info->partitions[index];
        u32 sector_lba;

        if (!partition->present || partition->sector_count < 2u) {
            continue;
        }

        sector_lba = install_state_sector_lba(partition->start_lba, partition->sector_count);
        if (read_install_state_sector(sector_lba, &sector) &&
            sector.stage == INSTALL_STATE_STAGE_PENDING_INSTALL) {
            *out_sector = sector;
            return true;
        }
    }

    if (storage_info->partitions[0].present ||
        storage_info->partitions[1].present ||
        storage_info->partitions[2].present ||
        storage_info->partitions[3].present) {
        return false;
    }

    if (read_install_state_sector(install_state_sector_lba(0u, storage_info->total_sectors), &sector) &&
        sector.stage == INSTALL_STATE_STAGE_PENDING_INSTALL) {
        *out_sector = sector;
        return true;
    }

    return false;
}

static bool load_existing_install_state(install_state_sector_t* out_sector) {
    u32 target_lba = 0u;
    u32 target_sector_count = 0u;
    u32 sector_lba;

    if (out_sector == NULL ||
        !installer_find_existing_install(&target_lba, &target_sector_count) ||
        target_sector_count < 2u) {
        return false;
    }

    sector_lba = install_state_sector_lba(target_lba, target_sector_count);
    if (!read_install_state_sector(sector_lba, out_sector)) {
        return false;
    }

    return out_sector->stage == INSTALL_STATE_STAGE_NEEDS_OOBE ||
           out_sector->stage == INSTALL_STATE_STAGE_READY;
}

static bool persist_state_for_target(u32 stage,
                                     u32 target_lba,
                                     u32 target_sector_count,
                                     const char* username,
                                     const char* password) {
    install_state_sector_t sector;

    if (!build_install_state_sector(stage, target_lba, target_sector_count, username, password, &sector)) {
        return false;
    }

    return write_install_state_sector(&sector);
}

static bool persist_state_from_desktop(const desktop_state_t* state,
                                       u32 stage,
                                       const char* username,
                                       const char* password) {
    if (state == NULL) {
        return false;
    }

    return persist_state_for_target(stage,
                                    state->install_state_target_lba,
                                    state->install_state_target_sectors,
                                    username,
                                    password);
}

static bool persist_installed_target_state(desktop_state_t* state,
                                           u32 stage,
                                           u32 target_lba,
                                           u32 target_sector_count,
                                           const char* username,
                                           const char* password) {
    if (state == NULL) {
        return false;
    }

    state->install_state_target_lba = target_lba;
    state->install_state_target_sectors = target_sector_count;
    state->install_state_stage = stage;
    return persist_state_for_target(stage,
                                    target_lba,
                                    target_sector_count,
                                    username,
                                    password);
}

static u32 desktop_shell_dock_mask(const desktop_state_t* state) {
    if (state == NULL) return 0u;

    u32 mask = 0u;
    if (state->show_search)   mask |= 1u << 0;
    if (state->show_settings) mask |= 1u << 1;
    if (state->show_terminal) mask |= 1u << 2;
    if (state->show_files)    mask |= 1u << 3;
    if (state->show_tasks)    mask |= 1u << 4;
    
    mask |= (state->taskbar_style & 0x3u) << 8u;
    return mask;
}

static void set_setup_phase(desktop_state_t* state, u32 phase, u32 current_second) {
    if (state == NULL) {
        return;
    }

    state->setup_phase = phase;
    state->setup_phase_started_second = current_second;
    state->setup_phase_started_ms = timer_ticks_ms();
    if (phase == 9u) {
        state->login_error = false;
        state->login_password_input[0] = '\0';
        state->lockscreen_prompt_visible = false;
        state->auth_field_focus = AUTH_FIELD_PASSWORD;
    }
}

typedef enum {
    SETUP_PHASE_NONE = 0,
    SETUP_PHASE_WELCOME = 1,
    SETUP_PHASE_INSTALLER = 2,
    SETUP_PHASE_INSTALLING = 3,
    SETUP_PHASE_USER_OOBE = 4,
    SETUP_PHASE_OOBE_HI = 5,
    SETUP_PHASE_OOBE_SETTING_UP = 6,
    SETUP_PHASE_OOBE_PREPARE = 7,
    SETUP_PHASE_OOBE_ALMOST_THERE = 8,
    SETUP_PHASE_LOCKSCREEN = 9,
    SETUP_PHASE_CHECKING_HARDWARE = 10,
    SETUP_PHASE_HARDWARE_ERROR = 11,
    SETUP_PHASE_SKIPPING_INSTALLER = 12,
    SETUP_PHASE_UPDATE_PROGRESS = 13
} setup_phase_t;

typedef enum {
    SETTINGS_HIT_NONE = 0,
    SETTINGS_HIT_NAV_SYSTEM_SPECS = 1,
    SETTINGS_HIT_NAV_PERSONALIZE = 3,
    SETTINGS_HIT_NAV_THEMES = 4,
    SETTINGS_HIT_TASKBAR_STYLE_MACXWIN = 7,
    SETTINGS_HIT_TASKBAR_STYLE_TENUI = 8,
    SETTINGS_HIT_THEME_ALANBLISS = 9,
    SETTINGS_HIT_THEME_BLOOM = 10,
    SETTINGS_HIT_THEME_VOID = 11,
    SETTINGS_HIT_THEME_MACTOSH = 12,
    SETTINGS_HIT_NAV_UPDATE = 13,
    SETTINGS_HIT_UPDATE_INSTALL = 14
} settings_hit_t;

typedef enum {
    SETTINGS_PAGE_SYSTEM_SPECS = 0,
    SETTINGS_PAGE_PERSONALIZE = 2,
    SETTINGS_PAGE_THEMES = 3,
    SETTINGS_PAGE_UPDATE = 4
} settings_page_t;

typedef enum {
    WALLPAPER_THEME_ALANBLISS = 0,
    WALLPAPER_THEME_BLOOM = 1,
    WALLPAPER_THEME_VOID = 2,
    WALLPAPER_THEME_MACTOSH = 3,
    WALLPAPER_THEME_COUNT = 4
} wallpaper_theme_t;

typedef enum {
    TASK_MANAGER_HIT_NONE = 0,
    TASK_MANAGER_HIT_NAV_CPU = 1,
    TASK_MANAGER_HIT_NAV_RAM = 2,
    TASK_MANAGER_HIT_NAV_STORAGE = 3
} task_manager_hit_t;

typedef enum {
    TASK_MANAGER_PAGE_CPU = 0,
    TASK_MANAGER_PAGE_RAM = 1,
    TASK_MANAGER_PAGE_STORAGE = 2
} task_manager_page_t;

static u32 seconds_since(u32 earlier, u32 current) {
    if (current >= earlier) {
        return current - earlier;
    }
    return (24u * 60u * 60u - earlier) + current;
}

static bool desktop_dark_mode(const desktop_state_t* state) {
    return state != NULL && state->desktop_theme == DESKTOP_THEME_JABULXP_DARK;
}

static const char* wallpaper_theme_name(u32 theme) {
    switch ((wallpaper_theme_t)theme) {
        case WALLPAPER_THEME_BLOOM:
            return "Bloom";
        case WALLPAPER_THEME_VOID:
            return "Void";
        case WALLPAPER_THEME_MACTOSH:
            return "Horizon Land";
        case WALLPAPER_THEME_ALANBLISS:
        default:
            return "DefaultBLiss";
    }
}

static const char* wallpaper_theme_asset_path(u32 theme) {
    switch ((wallpaper_theme_t)theme) {
        case WALLPAPER_THEME_BLOOM:
            return "themes/Bloom.bmp";
        case WALLPAPER_THEME_VOID:
            return "themes/Void.bmp";
        case WALLPAPER_THEME_MACTOSH:
            return "themes/Mactosh.bmp";
        case WALLPAPER_THEME_ALANBLISS:
        default:
            return "themes/AlanBliss.bmp";
    }
}

static const char* dock_app_icon_asset_path(dock_app_t app) {
    switch (app) {
        case APP_FILES:
            return "icons/FileManager.bmp";
        case APP_TASKS:
            return "icons/TaskManager.bmp";
        case APP_SNAKE:
            return "icons/Snake.bmp";
        case APP_JABVER:
            return "icons/Jabver.bmp";
        case APP_START:
            return NULL;
        case APP_SETTINGS:
            return "icons/Settings.bmp";
        case APP_TERMINAL:
            return "icons/Terminal.bmp";
        case APP_BROWSER:
            return "icons/FileManager.bmp";
        case APP_VIDEOS:
            return "icons/JabulMedia.bmp";
        default:
            return "icons/FileManager.bmp";
    }
}

static u32 safe_subtract(u32 value, u32 amount);
static void draw_text_clipped(u32 x, u32 y, u32 max_width, const char* text, u32 color);
static void draw_rounded_panel(u32 x,
                               u32 y,
                               u32 width,
                               u32 height,
                               u32 radius,
                               u32 fill_color,
                               u32 stroke_color);

static u32 ui_surface_color(const desktop_state_t* state, u32 level) {
    bool dark = desktop_dark_mode(state);

    if (!dark) {
        switch (level) {
            case 0u: return vga_color(255, 255, 255);
            case 1u: return vga_color(248, 250, 255);
            case 2u: return vga_color(238, 241, 248);
            case 3u: return vga_color(228, 234, 246);
            default: return vga_color(220, 228, 244);
        }
    }

    switch (level) {
        case 0u: return vga_color(10, 14, 22);
        case 1u: return vga_color(18, 22, 34);
        case 2u: return vga_color(24, 29, 44);
        case 3u: return vga_color(32, 38, 58);
        default: return vga_color(40, 48, 72);
    }
}

static u32 ui_stroke_color(const desktop_state_t* state, u32 level) {
    bool dark = desktop_dark_mode(state);

    if (!dark) {
        switch (level) {
            case 0u: return vga_color(224, 230, 242);
            case 1u: return vga_color(196, 204, 222);
            case 2u: return vga_color(170, 180, 204);
            default: return vga_color(136, 150, 182);
        }
    }

    switch (level) {
        case 0u: return vga_color(54, 64, 92);
        case 1u: return vga_color(72, 84, 116);
        case 2u: return vga_color(92, 106, 144);
        default: return vga_color(118, 136, 182);
    }
}

static u32 ui_text_primary(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(236, 241, 252) : vga_color(24, 30, 48);
}

static u32 ui_text_secondary(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(184, 194, 218) : vga_color(76, 86, 112);
}

static u32 ui_text_muted(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(142, 154, 182) : vga_color(112, 122, 146);
}

static u32 ui_text_accent(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(128, 176, 255) : vga_color(56, 112, 208);
}

static u32 ui_input_fill(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(12, 16, 26) : vga_color(255, 255, 255);
}

static u32 ui_input_stroke(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(76, 90, 126) : vga_color(188, 198, 220);
}

static u32 ui_success_color(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(122, 208, 154) : vga_color(66, 122, 86);
}

static u32 ui_danger_color(const desktop_state_t* state) {
    return desktop_dark_mode(state) ? vga_color(246, 132, 132) : vga_color(186, 76, 76);
}

static void fill_jabulxp_wallpaper(void) {
    u32 width = display_driver_width();
    u32 height = display_driver_height();

    for (u32 y = 0; y < height; ++y) {
        u8 red = (u8)(140 + (y * 34u) / (height ? height : 1u));
        u8 green = (u8)(132 + (y * 26u) / (height ? height : 1u));
        u8 blue = (u8)(214 + (y * 18u) / (height ? height : 1u));
        draw_rect(0, y, width, 1, vga_color(red, green, blue));
    }

    vga_fill_rect_alpha(width / 7u, height / 6u, width / 2u, height / 3u, 255, 255, 255, 30);
    vga_fill_rect_alpha(width / 2u, height / 7u, width / 3u, (height * 2u) / 3u, 118, 82, 236, 26);
    vga_fill_rect_alpha(width / 4u, height / 3u, width / 2u, height / 2u, 246, 244, 255, 22);
    vga_fill_rect_alpha(width / 3u, height / 2u, width / 3u, height / 5u, 98, 112, 255, 24);
}

static void fill_jabulxp_dark_wallpaper(void) {
    u32 width = display_driver_width();
    u32 height = display_driver_height();

    for (u32 y = 0; y < height; ++y) {
        u8 red = (u8)(22 + (y * 10u) / (height ? height : 1u));
        u8 green = (u8)(24 + (y * 12u) / (height ? height : 1u));
        u8 blue = (u8)(38 + (y * 20u) / (height ? height : 1u));
        draw_rect(0, y, width, 1, vga_color(red, green, blue));
    }

    vga_fill_rect_alpha(width / 7u, height / 6u, width / 2u, height / 3u, 255, 255, 255, 10);
    vga_fill_rect_alpha(width / 2u, height / 7u, width / 3u, (height * 2u) / 3u, 88, 92, 220, 14);
    vga_fill_rect_alpha(width / 4u, height / 3u, width / 2u, height / 2u, 180, 182, 255, 10);
    vga_fill_rect_alpha(width / 3u, height / 2u, width / 3u, height / 5u, 98, 112, 255, 12);
}

static void fill_generated_theme_wallpaper(u32 theme) {
    u32 width = display_driver_width();
    u32 height = display_driver_height();

    switch ((wallpaper_theme_t)theme) {
        case WALLPAPER_THEME_BLOOM:
            for (u32 y = 0; y < height; ++y) {
                u8 red = (u8)(134u + (y * 74u) / (height ? height : 1u));
                u8 green = (u8)(78u + (y * 34u) / (height ? height : 1u));
                u8 blue = (u8)(182u + (y * 48u) / (height ? height : 1u));
                draw_rect(0, y, width, 1, vga_color(red, green, blue));
            }
            vga_fill_rect_alpha(width / 9u, height / 8u, width / 2u, height / 3u, 255, 236, 248, 34u);
            vga_fill_rect_alpha(width / 2u, height / 5u, width / 3u, height / 2u, 255, 168, 210, 28u);
            vga_fill_rect_alpha(width / 5u, (height * 3u) / 5u, (width * 3u) / 5u, height / 5u, 242, 132, 196, 34u);
            break;
        case WALLPAPER_THEME_VOID:
            for (u32 y = 0; y < height; ++y) {
                u8 red = (u8)(8u + (y * 10u) / (height ? height : 1u));
                u8 green = (u8)(10u + (y * 16u) / (height ? height : 1u));
                u8 blue = (u8)(24u + (y * 34u) / (height ? height : 1u));
                draw_rect(0, y, width, 1, vga_color(red, green, blue));
            }
            vga_fill_rect_alpha(width / 8u, height / 7u, width / 3u, height / 4u, 120, 90, 255, 16u);
            vga_fill_rect_alpha(width / 2u, height / 4u, width / 4u, height / 2u, 42, 158, 196, 14u);
            vga_fill_rect_alpha(width / 6u, (height * 2u) / 3u, (width * 2u) / 3u, height / 6u, 30, 60, 112, 20u);
            break;
        case WALLPAPER_THEME_MACTOSH:
            for (u32 y = 0; y < height; ++y) {
                u8 red = (u8)(178u + (y * 24u) / (height ? height : 1u));
                u8 green = (u8)(206u + (y * 20u) / (height ? height : 1u));
                u8 blue = (u8)(220u + (y * 28u) / (height ? height : 1u));
                draw_rect(0, y, width, 1, vga_color(red, green, blue));
            }
            vga_fill_rect_alpha(width / 7u, height / 8u, width / 2u, height / 3u, 255, 255, 255, 28u);
            vga_fill_rect_alpha(width / 2u, height / 5u, width / 3u, (height * 2u) / 5u, 88, 192, 214, 24u);
            vga_fill_rect_alpha(width / 4u, (height * 2u) / 3u, width / 2u, height / 7u, 255, 154, 88, 28u);
            break;
        case WALLPAPER_THEME_ALANBLISS:
        default:
            for (u32 y = 0; y < height; ++y) {
                u8 red = (u8)(66u + (y * 38u) / (height ? height : 1u));
                u8 green = (u8)(138u + (y * 50u) / (height ? height : 1u));
                u8 blue = (u8)(210u + (y * 26u) / (height ? height : 1u));
                draw_rect(0, y, width, 1, vga_color(red, green, blue));
            }
            vga_fill_rect_alpha(width / 8u, height / 7u, width / 3u, height / 4u, 255, 255, 255, 24u);
            vga_fill_rect_alpha(width / 10u, (height * 3u) / 5u, (width * 4u) / 5u, height / 4u, 66, 176, 110, 36u);
            vga_fill_rect_alpha(width / 2u, height / 5u, width / 3u, height / 3u, 110, 196, 228, 18u);
            break;
    }
}

static void draw_theme_preview_art(u32 x, u32 y, u32 width, u32 height, u32 theme) {
    u32 sky;
    u32 glow;
    u32 ground;

    switch ((wallpaper_theme_t)theme) {
        case WALLPAPER_THEME_BLOOM:
            sky = vga_color(186, 112, 214);
            glow = vga_color(255, 214, 234);
            ground = vga_color(232, 126, 196);
            break;
        case WALLPAPER_THEME_VOID:
            sky = vga_color(18, 22, 46);
            glow = vga_color(120, 104, 244);
            ground = vga_color(30, 62, 108);
            break;
        case WALLPAPER_THEME_MACTOSH:
            sky = vga_color(194, 214, 224);
            glow = vga_color(255, 248, 255);
            ground = vga_color(92, 192, 210);
            break;
        case WALLPAPER_THEME_ALANBLISS:
        default:
            sky = vga_color(98, 172, 230);
            glow = vga_color(240, 250, 255);
            ground = vga_color(72, 178, 104);
            break;
    }

    draw_rounded_rect(x, y, width, height, 14u, sky);
    vga_fill_rect_alpha(x + 4u, y + 4u, safe_subtract(width, 8u), height / 3u, 255, 255, 255, 26u);

    if (theme == WALLPAPER_THEME_VOID) {
        vga_fill_rect_alpha(x + width / 5u, y + height / 5u, width / 3u, height / 3u, 140, 110, 255, 24u);
        draw_rounded_rect(x + 10u, y + height / 2u, safe_subtract(width, 20u), height / 4u, 12u, ground);
        vga_fill_rect_alpha(x + width / 2u, y + height / 3u, width / 4u, height / 3u, 54, 166, 194, 20u);
    } else if (theme == WALLPAPER_THEME_MACTOSH) {
        vga_fill_rect_alpha(x + 8u, y + 8u, safe_subtract(width, 16u), height / 4u, 255, 255, 255, 18u);
        draw_rounded_rect(x + 12u, y + height / 2u, safe_subtract(width, 24u), height / 4u, 12u, ground);
        vga_fill_rect_alpha(x + width / 3u, y + height / 3u, width / 3u, height / 4u, 255, 162, 102, 22u);
    } else if (theme == WALLPAPER_THEME_BLOOM) {
        vga_fill_rect_alpha(x + 12u, y + 10u, safe_subtract(width, 24u), height / 3u, 255, 230, 246, 22u);
        draw_rounded_rect(x + 10u, y + height / 2u, safe_subtract(width, 20u), height / 4u, 12u, ground);
        vga_fill_rect_alpha(x + width / 2u, y + height / 3u, width / 4u, height / 3u, 255, 186, 218, 18u);
    } else {
        vga_fill_rect_alpha(x + 10u, y + 10u, safe_subtract(width, 20u), height / 4u, 255, 255, 255, 16u);
        draw_rounded_rect(x + 10u, y + height / 2u, safe_subtract(width, 20u), height / 4u, 12u, ground);
        vga_fill_rect_alpha(x + width / 2u, y + height / 3u, width / 4u, height / 4u, 120, 208, 228, 18u);
    }

    draw_rounded_rect_outline(x, y, width, height, 14u, 1u, glow);
}

static bool settings_theme_card_rect(u32 content_x,
                                     u32 content_y,
                                     u32 content_width,
                                     u32 index,
                                     window_rect_t* out_rect) {
    u32 grid_width;
    u32 card_width;
    u32 card_height = 126u;
    u32 card_gap = 14u;
    u32 column;
    u32 row;

    if (out_rect == NULL || index >= WALLPAPER_THEME_COUNT) {
        return false;
    }

    grid_width = safe_subtract(content_width, 32u);
    if (grid_width <= card_gap) {
        return false;
    }

    card_width = (grid_width - card_gap) / 2u;
    column = index % 2u;
    row = index / 2u;
    out_rect->x = content_x + 16u + column * (card_width + card_gap);
    out_rect->y = content_y + 50u + row * (card_height + 14u);
    out_rect->width = card_width;
    out_rect->height = card_height;
    return true;
}

static void draw_settings_theme_card(const desktop_state_t* state,
                                     const window_rect_t* rect,
                                     u32 theme,
                                     bool active) {
    u32 fill = active ? ui_surface_color(state, 3u) : ui_surface_color(state, 1u);
    u32 outline = active ? ui_text_accent(state) : ui_stroke_color(state, 1u);
    u32 label = active ? ui_text_accent(state) : ui_text_primary(state);
    u32 preview_height = rect->height > 50u ? rect->height - 52u : 0u;

    if (rect == NULL) {
        return;
    }

    draw_rounded_panel(rect->x, rect->y, rect->width, rect->height, 18u, fill, outline);
    vga_fill_rect_alpha(rect->x + 2u,
                        rect->y + 2u,
                        rect->width - 4u,
                        12u,
                        255,
                        255,
                        255,
                        active ? (desktop_dark_mode(state) ? 10u : 18u) : (desktop_dark_mode(state) ? 6u : 10u));
    if (theme < WALLPAPER_THEME_COUNT && g_theme_image_loaded[theme]) {
        image_blit_fit_rounded(&g_theme_images[theme],
                               rect->x + 10u,
                               rect->y + 10u,
                               safe_subtract(rect->width, 20u),
                               preview_height,
                               14u);
    } else {
        draw_theme_preview_art(rect->x + 10u, rect->y + 10u, safe_subtract(rect->width, 20u), preview_height, theme);
    }
    draw_text_clipped(rect->x + 14u,
                      rect->y + rect->height - 26u,
                      safe_subtract(rect->width, 28u),
                      wallpaper_theme_name(theme),
                      label);
}

static void draw_desktop_background(const desktop_state_t* state, const image_t* wallpaper) {
    u32 active_theme = state != NULL ? state->wallpaper_theme : (u32)WALLPAPER_THEME_ALANBLISS;

    if (active_theme >= WALLPAPER_THEME_COUNT) {
        active_theme = WALLPAPER_THEME_ALANBLISS;
    }

    if (active_theme < WALLPAPER_THEME_COUNT && g_theme_image_loaded[active_theme]) {
        display_driver_draw_wallpaper(&g_theme_images[active_theme]);
    } else if (active_theme == WALLPAPER_THEME_ALANBLISS && wallpaper != NULL) {
        display_driver_draw_wallpaper(wallpaper);
    } else if (active_theme == WALLPAPER_THEME_ALANBLISS) {
        if (desktop_dark_mode(state)) {
            fill_jabulxp_dark_wallpaper();
        } else {
            fill_jabulxp_wallpaper();
        }
        return;
    } else {
        fill_generated_theme_wallpaper(active_theme);
    }

    if (desktop_dark_mode(state)) {
        vga_fill_rect_alpha(0, 0, display_driver_width(), display_driver_height(), 0, 0, 0, active_theme == WALLPAPER_THEME_VOID ? 72u : 92u);
    } else if (active_theme < WALLPAPER_THEME_COUNT && g_theme_image_loaded[active_theme]) {
        vga_fill_rect_alpha(0, 0, display_driver_width(), display_driver_height(), 66, 58, 152, 28u);
    }
}

static void initialize_theme_image_cache(const boot_info_t* boot_info, const image_t* default_wallpaper) {
    if (g_theme_image_cache_ready) {
        return;
    }

    memset(g_theme_images, 0, sizeof(g_theme_images));
    memset(g_theme_image_loaded, 0, sizeof(g_theme_image_loaded));

    if (default_wallpaper != NULL) {
        g_theme_images[WALLPAPER_THEME_ALANBLISS] = *default_wallpaper;
        g_theme_image_loaded[WALLPAPER_THEME_ALANBLISS] = true;
    }

    if (boot_info != NULL && boot_info->initrd_start != 0u && boot_info->initrd_size != 0u) {
        const void* archive = (const void*)boot_info->initrd_start;
        u32 archive_size = (u32)boot_info->initrd_size;

        for (u32 index = 0u; index < WALLPAPER_THEME_COUNT; ++index) {
            initrd_file_t file;
            if (initrd_find_file(archive, archive_size, wallpaper_theme_asset_path(index), &file) &&
                image_load_any(file.data, file.size, &g_theme_images[index])) {
                g_theme_image_loaded[index] = true;
            }
        }
    }

    g_theme_image_cache_ready = true;
}

static void initialize_app_icon_cache(const boot_info_t* boot_info) {
    if (g_app_icon_cache_ready) {
        return;
    }

    memset(g_app_icon_images, 0, sizeof(g_app_icon_images));
    memset(g_app_icon_loaded, 0, sizeof(g_app_icon_loaded));

    if (boot_info != NULL && boot_info->initrd_start != 0u && boot_info->initrd_size != 0u) {
        const void* archive = (const void*)boot_info->initrd_start;
        u32 archive_size = (u32)boot_info->initrd_size;

        for (u32 app_index = 0u; app_index < DOCK_APP_MAX; ++app_index) {
            const char* asset_path = dock_app_icon_asset_path((dock_app_t)app_index);
            initrd_file_t file;

            if (asset_path == NULL) {
                continue;
            }
            if (initrd_find_file(archive, archive_size, asset_path, &file) &&
                image_load_any(file.data, file.size, &g_app_icon_images[app_index])) {
                g_app_icon_loaded[app_index] = true;
            }
        }
    }

    g_app_icon_cache_ready = true;
}

static u32 text_width(const char* text) {
    return (u32)strlen(text) * vga_text_advance_x();
}

static u32 safe_subtract(u32 value, u32 amount) {
    return value > amount ? value - amount : 0u;
}

static bool get_cursor_draw_rect(u32 mouse_x, u32 mouse_y, window_rect_t* out_rect) {
    u32 draw_x;
    u32 draw_y;
    u32 width;
    u32 height;

    if (out_rect == NULL) {
        return false;
    }

    draw_x = safe_subtract(mouse_x, CURSOR_HOTSPOT_X);
    draw_y = safe_subtract(mouse_y, CURSOR_HOTSPOT_Y);
    if (draw_x >= display_driver_width() || draw_y >= display_driver_height()) {
        return false;
    }

    width = CURSOR_WIDTH;
    height = CURSOR_HEIGHT;
    if (draw_x + width > display_driver_width() || draw_x + width < draw_x) {
        width = display_driver_width() - draw_x;
    }
    if (draw_y + height > display_driver_height() || draw_y + height < draw_y) {
        height = display_driver_height() - draw_y;
    }
    if (width == 0u || height == 0u) {
        return false;
    }

    out_rect->x = draw_x;
    out_rect->y = draw_y;
    out_rect->width = width;
    out_rect->height = height;
    return true;
}

static void draw_mouse_cursor_to_surface(u32 x, u32 y, bool front_buffer) {
    static const char* cursor_shape[CURSOR_HEIGHT] = {
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00003220000000000000000000000000",
        "00022222300000000000000000000000",
        "00023113220000000000000000000000",
        "00321111322000000000000000000000",
        "00321111112230000000000000000000",
        "00321111111322000000000000000000",
        "00321111111132200000000000000000",
        "00321111111111223000000000000000",
        "00321111111111132300000000000000",
        "00321111111111113200000000000000",
        "00321111111111111230000000000000",
        "00321111111111113210000000000000",
        "00321111111133332310000000000000",
        "00321111112222233100000000000000",
        "00321111123111111100000000000000",
        "00321111231111100000000000000000",
        "00123112211000000000000000000000",
        "00132223110000000000000000000000",
        "00011311100000000000000000000000",
        "00001111000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000",
        "00000000000000000000000000000000"
    };
    u32 draw_x = safe_subtract(x, CURSOR_HOTSPOT_X);
    u32 draw_y = safe_subtract(y, CURSOR_HOTSPOT_Y);
    u32 outline = vga_color(18, 18, 18);
    u32 fill = vga_color(255, 255, 255);
    u32 accent = vga_color(176, 176, 176);

    for (u32 row = 0; row < CURSOR_HEIGHT; ++row) {
        for (u32 col = 0; col < CURSOR_WIDTH; ++col) {
            char pixel = cursor_shape[row][col];
            u32 color = 0u;
            bool visible = true;

            if (pixel == '1') {
                color = outline;
            } else if (pixel == '2') {
                color = fill;
            } else if (pixel == '3') {
                color = accent;
            } else {
                visible = false;
            }

            if (!visible) {
                continue;
            }

            if (front_buffer) {
                display_driver_put_pixel_front(draw_x + col, draw_y + row, color);
            } else {
                put_pixel(draw_x + col, draw_y + row, color);
            }
        }
    }
}

static u32 text_line_height(void) {
    return vga_text_height() + 4u;
}

static void draw_text_span(u32 x, u32 y, const char* text, u32 length, u32 color) {
    if (text == NULL) {
        return;
    }

    for (u32 index = 0; index < length; ++index) {
        draw_char(x + index * vga_text_advance_x(), y, text[index], color);
    }
}

static void draw_text_clipped(u32 x, u32 y, u32 max_width, const char* text, u32 color) {
    u32 max_chars;
    u32 length;

    if (text == NULL || text[0] == '\0' || max_width < vga_text_advance_x()) {
        return;
    }

    max_chars = max_width / vga_text_advance_x();
    length = (u32)strlen(text);
    if (length <= max_chars) {
        draw_text(x, y, text, color);
        return;
    }

    if (max_chars <= 3u) {
        draw_text_span(x, y, text, max_chars, color);
        return;
    }

    draw_text_span(x, y, text, max_chars - 3u, color);
    draw_text_span(x + (max_chars - 3u) * vga_text_advance_x(), y, "...", 3u, color);
}

static void draw_text_centered_clipped(u32 x, u32 y, u32 max_width, const char* text, u32 color) {
    u32 draw_width;
    u32 text_x;

    if (text == NULL || text[0] == '\0' || max_width < vga_text_advance_x()) {
        return;
    }

    draw_width = text_width(text);
    if (draw_width >= max_width) {
        draw_text_clipped(x, y, max_width, text, color);
        return;
    }

    text_x = x + (max_width - draw_width) / 2u;
    draw_text(text_x, y, text, color);
}

static void draw_text_right_clipped(u32 x, u32 y, u32 max_width, const char* text, u32 color) {
    u32 draw_width;
    u32 text_x;

    if (text == NULL || text[0] == '\0' || max_width < vga_text_advance_x()) {
        return;
    }

    draw_width = text_width(text);
    if (draw_width >= max_width) {
        draw_text_clipped(x, y, max_width, text, color);
        return;
    }

    text_x = x + (max_width - draw_width);
    draw_text(text_x, y, text, color);
}

static void draw_text_boldish(u32 x, u32 y, u32 max_width, const char* text, u32 color) {
    if (text == NULL || text[0] == '\0' || max_width < vga_text_advance_x()) {
        return;
    }

    draw_text_clipped(x, y, max_width, text, color);
    if (max_width > vga_text_advance_x()) {
        draw_text_clipped(x + 1u, y, max_width - 1u, text, color);
    }
}

static u32 wrap_line_length(const char* text, u32 max_chars) {
    u32 index = 0u;
    u32 last_space = 0u;

    if (text == NULL || text[0] == '\0' || max_chars == 0u) {
        return 0u;
    }

    while (text[index] != '\0' && text[index] != '\n') {
        if (text[index] == ' ') {
            last_space = index;
        }
        ++index;
        if (index > max_chars) {
            return last_space > 0u ? last_space : max_chars;
        }
    }

    return index;
}

static u32 draw_text_wrapped(u32 x,
                             u32 y,
                             u32 max_width,
                             u32 max_height,
                             const char* text,
                             u32 color) {
    const char* cursor = text;
    u32 line_height = text_line_height();
    u32 max_chars;
    u32 current_y = y;
    u32 bottom_y;

    if (text == NULL || text[0] == '\0' || max_width < vga_text_advance_x() || max_height < vga_text_height()) {
        return y;
    }

    max_chars = max_width / vga_text_advance_x();
    bottom_y = y + max_height;

    while (*cursor != '\0' && current_y + vga_text_height() <= bottom_y) {
        u32 line_len;
        const char* next;

        while (*cursor == ' ') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        if (*cursor == '\n') {
            ++cursor;
            current_y += line_height;
            continue;
        }

        line_len = wrap_line_length(cursor, max_chars);
        if (line_len == 0u) {
            break;
        }

        draw_text_span(x, current_y, cursor, line_len, color);
        next = cursor + line_len;
        while (*next == ' ') {
            ++next;
        }
        if (*next == '\n') {
            ++next;
        }
        cursor = next;
        current_y += line_height;
    }

    return current_y;
}

static void draw_text_centered(u32 y, const char* text, u32 color) {
    u32 width = text_width(text);
    u32 screen_width = display_driver_width();
    u32 x = (screen_width > width) ? (screen_width - width) / 2u : 0;
    draw_text(x, y, text, color);
}

static void draw_rounded_panel(u32 x,
                               u32 y,
                               u32 width,
                               u32 height,
                               u32 radius,
                               u32 fill_color,
                               u32 stroke_color) {
    u32 inner_radius = radius > 1u ? radius - 1u : 0u;

    if (width == 0u || height == 0u) {
        return;
    }

    draw_rounded_rect(x, y, width, height, radius, fill_color);
    draw_rounded_rect_outline(x, y, width, height, radius, 1u, stroke_color);
    if (width > 2u && height > 2u && fill_color != 0u) {
        draw_rounded_rect(x + 1u, y + 1u, width - 2u, height - 2u, inner_radius, fill_color);
    }
}

typedef enum {
    BOOT_INPUT_NONE = 0,
    BOOT_INPUT_ANY_KEY = 1,
    BOOT_INPUT_RESTART = 2,
    BOOT_INPUT_SHUTDOWN = 3
} boot_input_action_t;

static boot_input_action_t boot_poll_keyboard_input(bool chaos_mode) {
    while ((inb(0x64) & 0x01u) != 0u) {
        u8 status = inb(0x64);

        if ((status & 0x20u) != 0u) {
            (void)inb(0x60);
            continue;
        }

        u8 scancode = inb(0x60);
        u8 key_code = (u8)(scancode & 0x7Fu);
        if (scancode == 0xE0u || scancode == 0xE1u || (scancode & 0x80u) != 0u) {
            continue;
        }

        if (chaos_mode) {
            if (key_code == 0x13u) {
                return BOOT_INPUT_RESTART;
            }
            if (key_code == 0x1Fu) {
                return BOOT_INPUT_SHUTDOWN;
            }
        }

        return BOOT_INPUT_ANY_KEY;
    }

    return BOOT_INPUT_NONE;
}

static u32 elapsed_seconds_since(u32 start_second, u32 current_second) {
    const u32 seconds_per_day = 24u * 60u * 60u;
    if (current_second >= start_second) {
        return current_second - start_second;
    }
    return (seconds_per_day - start_second) + current_second;
}

static void cpuid_query(u32 leaf, u32 subleaf, u32* out_eax, u32* out_ebx, u32* out_ecx, u32* out_edx) {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;

    asm volatile ("cpuid"
                  : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                  : "a"(leaf), "c"(subleaf));

    if (out_eax != NULL) {
        *out_eax = eax;
    }
    if (out_ebx != NULL) {
        *out_ebx = ebx;
    }
    if (out_ecx != NULL) {
        *out_ecx = ecx;
    }
    if (out_edx != NULL) {
        *out_edx = edx;
    }
}

static u64 cpu_read_tsc(void) {
    u32 low;
    u32 high;

    asm volatile ("rdtsc" : "=a"(low), "=d"(high));
    return ((u64)high << 32) | low;
}

static void cpu_write_register_text(char* destination, u32 value) {
    destination[0] = (char)(value & 0xFFu);
    destination[1] = (char)((value >> 8) & 0xFFu);
    destination[2] = (char)((value >> 16) & 0xFFu);
    destination[3] = (char)((value >> 24) & 0xFFu);
}

static void cpu_trim_text(char* text) {
    size_t length;

    if (text == NULL) {
        return;
    }

    length = strlen(text);
    while (length > 0u && (text[length - 1u] == ' ' || text[length - 1u] == '\0')) {
        text[length - 1u] = '\0';
        --length;
    }
}

static void cpu_query_identity(desktop_state_t* state) {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;
    u32 max_leaf;
    u32 max_extended_leaf;

    if (state == NULL) {
        return;
    }

    cpuid_query(0u, 0u, &max_leaf, &ebx, &ecx, &edx);
    cpu_write_register_text(state->cpu_vendor + 0u, ebx);
    cpu_write_register_text(state->cpu_vendor + 4u, edx);
    cpu_write_register_text(state->cpu_vendor + 8u, ecx);
    state->cpu_vendor[12] = '\0';

    if (max_leaf >= 1u) {
        u32 base_family;
        u32 base_model;
        u32 extended_family;
        u32 extended_model;

        cpuid_query(1u, 0u, &eax, &ebx, &ecx, &edx);
        state->cpu_stepping = eax & 0x0Fu;
        base_model = (eax >> 4) & 0x0Fu;
        base_family = (eax >> 8) & 0x0Fu;
        extended_model = (eax >> 16) & 0x0Fu;
        extended_family = (eax >> 20) & 0xFFu;
        state->cpu_family = base_family == 0x0Fu ? base_family + extended_family : base_family;
        state->cpu_model = ((base_family == 0x06u || base_family == 0x0Fu) ? (extended_model << 4) : 0u) + base_model;
    }

    cpuid_query(0x80000000u, 0u, &max_extended_leaf, &ebx, &ecx, &edx);
    if (max_extended_leaf >= 0x80000004u) {
        for (u32 leaf = 0u; leaf < 3u; ++leaf) {
            cpuid_query(0x80000002u + leaf, 0u, &eax, &ebx, &ecx, &edx);
            cpu_write_register_text(state->cpu_brand + leaf * 16u + 0u, eax);
            cpu_write_register_text(state->cpu_brand + leaf * 16u + 4u, ebx);
            cpu_write_register_text(state->cpu_brand + leaf * 16u + 8u, ecx);
            cpu_write_register_text(state->cpu_brand + leaf * 16u + 12u, edx);
        }
        state->cpu_brand[48] = '\0';
        cpu_trim_text(state->cpu_brand);
    }

    if (state->cpu_brand[0] == '\0') {
        strcpy(state->cpu_brand, state->cpu_vendor[0] != '\0' ? state->cpu_vendor : "Unknown CPU");
    }
}

static volatile u64 g_cpu_benchmark_sink = 0u;
static const u16 SB16_BASE_PORT = 0x220u;

static bool sb16_wait_write_ready(void) {
    for (u32 attempt = 0u; attempt < 100000u; ++attempt) {
        if ((inb((u16)(SB16_BASE_PORT + 0x0Cu)) & 0x80u) == 0u) {
            return true;
        }
    }
    return false;
}

static bool sb16_wait_read_ready(void) {
    for (u32 attempt = 0u; attempt < 100000u; ++attempt) {
        if ((inb((u16)(SB16_BASE_PORT + 0x0Eu)) & 0x80u) != 0u) {
            return true;
        }
    }
    return false;
}

static bool sb16_write(u8 value) {
    if (!sb16_wait_write_ready()) {
        return false;
    }
    outb((u16)(SB16_BASE_PORT + 0x0Cu), value);
    return true;
}

static bool sb16_read(u8* out_value) {
    if (out_value == NULL || !sb16_wait_read_ready()) {
        return false;
    }
    *out_value = inb((u16)(SB16_BASE_PORT + 0x0Au));
    return true;
}

static bool sb16_reset_dsp(void) {
    u8 ack = 0u;

    outb((u16)(SB16_BASE_PORT + 0x06u), 1u);
    for (u32 wait = 0u; wait < 1024u; ++wait) {
        io_wait();
    }
    outb((u16)(SB16_BASE_PORT + 0x06u), 0u);
    if (!sb16_read(&ack)) {
        return false;
    }
    return ack == 0xAAu;
}

static bool sb16_set_time_constant(u32 sample_rate) {
    u32 clamped_rate;
    u8 time_constant;

    if (sample_rate < 4000u) {
        clamped_rate = 4000u;
    } else if (sample_rate > 22000u) {
        clamped_rate = 22000u;
    } else {
        clamped_rate = sample_rate;
    }

    time_constant = (u8)(256u - (1000000u / clamped_rate));
    return sb16_write(0x40u) && sb16_write(time_constant);
}

static bool sb16_detect(void) {
    u8 major = 0u;
    u8 minor = 0u;

    if (!sb16_reset_dsp()) {
        return false;
    }
    if (!sb16_write(0xE1u) || !sb16_read(&major) || !sb16_read(&minor)) {
        return false;
    }
    return major != 0u || minor != 0u;
}

[[maybe_unused]] static bool sb16_begin_stream(u32 sample_rate) {
    return sb16_reset_dsp() &&
           sb16_set_time_constant(sample_rate) &&
           sb16_write(0xD1u);
}

static void sb16_end_stream(void) {
    if (!sb16_reset_dsp()) {
        return;
    }
    (void)sb16_write(0xD3u);
}

static bool sb16_play_sample(u8 sample) {
    return sb16_write(0x10u) && sb16_write(sample);
}

static void cpu_run_benchmark(desktop_state_t* state) {
    volatile u64 accumulator = 0x9E3779B97F4A7C15ull;
    u32 idle_second;
    u32 benchmark_start_second;
    u32 benchmark_end_second;
    u64 tsc_start;
    u64 tsc_end;
    u32 score = 0u;

    if (state == NULL) {
        return;
    }

    idle_second = rtc_read_seconds_of_day();
    while (rtc_read_seconds_of_day() == idle_second) {
    }

    benchmark_start_second = rtc_read_seconds_of_day();
    tsc_start = cpu_read_tsc();
    while (rtc_read_seconds_of_day() == benchmark_start_second) {
        for (u32 index = 0; index < 8192u; ++index) {
            accumulator ^= accumulator << 13;
            accumulator ^= accumulator >> 7;
            accumulator ^= accumulator << 17;
            accumulator += 0xD6E8FEB86659FD93ull;
        }
        score += 8192u;
    }
    tsc_end = cpu_read_tsc();
    benchmark_end_second = rtc_read_seconds_of_day();

    g_cpu_benchmark_sink ^= accumulator;
    state->cpu_benchmark_duration = elapsed_seconds_since(benchmark_start_second, benchmark_end_second);
    if (state->cpu_benchmark_duration == 0u) {
        state->cpu_benchmark_duration = 1u;
    }
    state->cpu_clock_mhz = (u32)((tsc_end - tsc_start) / ((u64)state->cpu_benchmark_duration * 1000000ull));
    state->cpu_benchmark_score = score / state->cpu_benchmark_duration;
    state->cpu_last_benchmark_second = benchmark_end_second;
    state->cpu_ready = true;
}

static void pc_speaker_start(u32 frequency_hz) {
    u32 divisor;
    u8 speaker_state;

    if (frequency_hz == 0u) {
        return;
    }

    divisor = 1193182u / frequency_hz;
    outb(0x43, 0xB6);
    outb(0x42, (u8)(divisor & 0xFFu));
    outb(0x42, (u8)((divisor >> 8) & 0xFFu));

    speaker_state = inb(0x61);
    if ((speaker_state & 0x03u) != 0x03u) {
        outb(0x61, speaker_state | 0x03u);
    }
}

static void pc_speaker_stop(void) {
    outb(0x61, inb(0x61) & (u8)~0x03u);
}

static void pc_speaker_play_pwm_sample(u8 sample) {
    u32 microseconds = ((u32)sample * 60u) / 255u;
    u32 divisor = (microseconds * 1193182u) / 1000000u;
    u8 speaker_state;

    if (microseconds == 0u) {
        speaker_state = inb(0x61);
        outb(0x61, (speaker_state | 0x01u) & (u8)~0x02u);
        return;
    }

    if (divisor == 0u) {
        divisor = 1u;
    }

    outb(0x43, 0xB0);
    outb(0x42, (u8)(divisor & 0xFFu));
    outb(0x42, (u8)((divisor >> 8) & 0xFFu));

    speaker_state = inb(0x61);
    if ((speaker_state & 0x03u) != 0x03u) {
        outb(0x61, speaker_state | 0x03u);
    }
}

static void render_desktop_bsod_screen(void) {
    u32 screen_height = display_driver_height();
    u32 title_y = screen_height > 180u ? (screen_height / 2u) - 42u : screen_height / 2u;
    u32 footer_y = screen_height > 52u ? screen_height - 44u : title_y + 30u;
    u32 white = vga_color(255, 255, 255);

    vga_clear(vga_color(0, 64, 200));
    draw_text_centered(title_y, "Not Our Fault", white);
    draw_text_centered(footer_y, "NOF", white);
    vga_present();
}

static void run_desktop_bsod_sequence(void) {
    u32 start_second = rtc_read_seconds_of_day();

    render_desktop_bsod_screen();
    pc_speaker_start(440u);

    for (;;) {
        if (elapsed_seconds_since(start_second, rtc_read_seconds_of_day()) >= 5u) {
            break;
        }
    }

    pc_speaker_stop();
    reboot_system();
}

static void render_boot_chaos_screen(u32 phase) {
    static const char* const chaos_lines[] = {
        "VX-13 // STATIC NOISE // UNUSUAL TEXT // 0xBEEF // SIGNAL LOST //",
        "NULL VECTOR // GLITCH FEED // JABULOS AERO // NO SAFE OUTPUT //",
        "SYSTEM CHANNEL CORRUPTED // LETTER STREAM UNSTABLE // HOLD TIGHT //",
        "TEXT STORM ACTIVE // SCREEN OVERRIDE // KERNEL STATIC // BEEP LOOP //",
        "NOISE TABLE 77 // MEMORY ECHO // DISPLAY DISTORTION // SPILL DATA //",
        "ODD SYMBOL CASCADE // VISUAL CHANNEL JAMMED // BOOT STAGE MUTATED //",
        "ALERT // UNUSUAL TEXT MODE // CHAOS FEED // AUDIO WALL // ALERT //",
        "SHAPELESS STRINGS // JITTER BLOCKS // BROKEN PROMPT // HOT LOOP //"
    };
    const u32 line_count = sizeof(chaos_lines) / sizeof(chaos_lines[0]);
    u32 width = vga_width();
    u32 height = vga_height();
    u32 title_y = height > 140u ? height / 2u - 34u : 18u;
    u32 prompt_y = height > 56u ? height - 42u : title_y + 24u;
    u32 background = vga_color((u8)((phase * 17u) % 56u),
                               0u,
                               (u8)(32u + ((phase * 29u) % 88u)));
    u32 white = vga_color(255, 255, 255);

    vga_clear(background);

    for (u32 row = 0u, y = 12u; y + vga_text_height() + 56u < height; ++row, y += vga_text_height() + 8u) {
        u32 x_span = width > 320u ? width - 320u : 1u;
        u32 x = x_span == 0u ? 0u : (row * 41u + phase * 23u) % x_span;
        u32 color = (row & 1u) != 0u
                        ? vga_color(255, (u8)(120u + ((phase * 13u) % 120u)), 120)
                        : vga_color((u8)(80u + ((phase * 19u) % 160u)), 255, 180);
        draw_text_clipped(x, y, width > x ? width - x : 0u, chaos_lines[(row + phase) % line_count], color);
    }

    draw_text_centered(title_y, "UNUSUAL TEXT MODE ACTIVE", white);
    draw_text_centered(title_y + 24u, "Press R to restart or S to shut down", white);
    draw_text_centered(prompt_y, "Boot animation keypress detected", vga_color(255, 208, 208));
    vga_present();
}

static void run_boot_chaos_mode(void) {
    for (u32 phase = 0u;; ++phase) {
        render_boot_chaos_screen(phase);
        pc_speaker_start((phase & 1u) != 0u ? 2450u : 1680u);

        u64 chaos_start = timer_ticks_ms();
        while (timer_ticks_ms() - chaos_start < 20u) {
            boot_input_action_t action = boot_poll_keyboard_input(true);
            if (action == BOOT_INPUT_RESTART) {
                pc_speaker_stop();
                reboot_system();
            }
            if (action == BOOT_INPUT_SHUTDOWN) {
                pc_speaker_stop();
                power_off_system();
            }
        }
    }
}

static void render_power_action_screen(power_action_t action) {
    u32 screen_height = display_driver_height();
    u32 title_y = screen_height > 180u ? (screen_height / 2u) - 18u : screen_height / 2u;
    u32 detail_y = title_y + 28u;
    u32 white = vga_color(255, 255, 255);
    const char* title = action == POWER_ACTION_RESTART ? "Restarting" : "Shutting down";

    vga_clear(vga_color(0, 0, 0));
    draw_text_centered(title_y, title, white);
    draw_text_centered(detail_y, "Please wait...", white);
    vga_present();
}

static void begin_power_action(desktop_state_t* state, power_action_t action, u32 current_second) {
    if (state == NULL || action == POWER_ACTION_NONE) {
        return;
    }

    state->power_action = action;
    state->power_action_started_second = current_second;
    state->launcher_power_menu_open = false;
    close_window(state, WINDOW_SEARCH);
}

static void update_power_action_state(desktop_state_t* state, u32 current_second) {
    if (state == NULL || state->power_action == POWER_ACTION_NONE) {
        return;
    }

    if (elapsed_seconds_since(state->power_action_started_second, current_second) < POWER_ACTION_DELAY_SECONDS) {
        return;
    }

    if (state->power_action == POWER_ACTION_RESTART) {
        reboot_system();
    }

    power_off_system();
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

static void format_percent_text(char* buffer, u32 percent) {
    if (buffer == NULL) {
        return;
    }

    buffer[0] = '\0';
    append_uint(buffer, percent > 100u ? 100u : percent);
    append_text(buffer + strlen(buffer), "%");
}

static void format_uptime_text(char* buffer, u64 uptime_ms) {
    u32 total_seconds = (u32)(uptime_ms / 1000ull);
    u32 hours = total_seconds / 3600u;
    u32 minutes = (total_seconds % 3600u) / 60u;
    u32 seconds = total_seconds % 60u;
    char number[16];

    if (buffer == NULL) {
        return;
    }

    buffer[0] = '\0';
    append_uint(buffer, hours);
    append_text(buffer + strlen(buffer), "h ");
    append_uint(number, minutes);
    append_text(buffer + strlen(buffer), number);
    append_text(buffer + strlen(buffer), "m ");
    append_uint(number, seconds);
    append_text(buffer + strlen(buffer), number);
    append_text(buffer + strlen(buffer), "s");
}

static void format_transfer_rate_text(char* buffer, u32 kib_per_second) {
    char number[16];

    if (buffer == NULL) {
        return;
    }

    buffer[0] = '\0';
    if (kib_per_second >= 1024u) {
        append_uint(number, kib_per_second / 1024u);
        append_text(buffer, number);
        append_text(buffer + strlen(buffer), " MB/s");
    } else {
        append_uint(number, kib_per_second);
        append_text(buffer, number);
        append_text(buffer + strlen(buffer), " KB/s");
    }
}

static u32 task_manager_cpu_usage_percent(const desktop_state_t* state) {
    if (state == NULL || !state->cpu_ready) {
        return 0u;
    }
    return state->cpu_benchmark_score >= 1600000u ? 100u : (state->cpu_benchmark_score * 100u) / 1600000u;
}

static u32 task_manager_storage_write_benchmark_kib(const desktop_state_t* state) {
    u32 capacity_hint;
    u32 benchmark;

    if (state == NULL || !state->storage_info.present) {
        return 0u;
    }

    capacity_hint = state->storage_info.total_sectors / 32768u;
    benchmark = 2048u + (state->cpu_clock_mhz * 2u) + capacity_hint;
    return benchmark > 32768u ? 32768u : benchmark;
}

static u32 task_manager_storage_usage_percent(const desktop_state_t* state) {
    u32 benchmark = task_manager_storage_write_benchmark_kib(state);
    return benchmark >= 16384u ? 100u : (benchmark * 100u) / 16384u;
}

static void split_task_manager_header_text(const char* source, char* line_one, char* line_two, u32 max_line_chars) {
    u32 length;
    u32 split;

    if (line_one == NULL || line_two == NULL) {
        return;
    }

    line_one[0] = '\0';
    line_two[0] = '\0';
    if (source == NULL || source[0] == '\0') {
        return;
    }

    length = (u32)strlen(source);
    split = length > max_line_chars ? max_line_chars : length;
    while (split > 0u && split < length && source[split] != ' ') {
        --split;
    }
    if (split == 0u || split >= length) {
        split = length > max_line_chars ? max_line_chars : length;
    }

    memcpy(line_one, source, split);
    line_one[split] = '\0';
    if (split < length) {
        u32 source_index = source[split] == ' ' ? split + 1u : split;
        u32 remaining = length - source_index;
        if (remaining > max_line_chars) {
            remaining = max_line_chars;
        }
        memcpy(line_two, source + source_index, remaining);
        line_two[remaining] = '\0';
    }
}

static void format_display_mode(char* buffer) {
    char number[16];

    if (buffer == NULL) {
        return;
    }

    buffer[0] = '\0';
    append_uint(number, display_driver_width());
    append_text(buffer, number);
    append_text(buffer + strlen(buffer), "x");
    append_uint(number, display_driver_height());
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

static void snake_reset_state(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    state->snake_length = 3u;
    state->snake_x[0] = 4u;
    state->snake_y[0] = 4u;
    state->snake_x[1] = 3u;
    state->snake_y[1] = 4u;
    state->snake_x[2] = 2u;
    state->snake_y[2] = 4u;
    state->snake_dir_x = 1;
    state->snake_dir_y = 0;
    state->snake_food_x = 8u;
    state->snake_food_y = 4u;
    state->snake_score = 0u;
    state->snake_game_over = false;
}

static void snake_spawn_food(desktop_state_t* state) {
    u32 total_cells = SNAKE_GRID_WIDTH * SNAKE_GRID_HEIGHT;
    u32 seed = (rtc_read_seconds_of_day() + state->snake_score * 7u + state->snake_length * 13u) % total_cells;

    for (u32 offset = 0; offset < total_cells; ++offset) {
        u32 candidate = (seed + offset * 5u) % total_cells;
        u8 next_x = (u8)(candidate % SNAKE_GRID_WIDTH);
        u8 next_y = (u8)(candidate / SNAKE_GRID_WIDTH);
        bool occupied = false;

        for (u32 index = 0; index < state->snake_length; ++index) {
            if (state->snake_x[index] == next_x && state->snake_y[index] == next_y) {
                occupied = true;
                break;
            }
        }
        if (!occupied) {
            state->snake_food_x = next_x;
            state->snake_food_y = next_y;
            return;
        }
    }
}

static bool snake_move(desktop_state_t* state, s32 dir_x, s32 dir_y) {
    s32 next_x;
    s32 next_y;
    bool ate_food;
    u32 old_length;
    u32 new_length;

    if (state == NULL || (dir_x == 0 && dir_y == 0)) {
        return false;
    }

    if (state->snake_game_over) {
        snake_reset_state(state);
        return true;
    }

    if (state->snake_length > 1u &&
        dir_x == -state->snake_dir_x &&
        dir_y == -state->snake_dir_y) {
        dir_x = state->snake_dir_x;
        dir_y = state->snake_dir_y;
    }

    next_x = (s32)state->snake_x[0] + dir_x;
    next_y = (s32)state->snake_y[0] + dir_y;
    if (next_x < 0 || next_y < 0 || next_x >= (s32)SNAKE_GRID_WIDTH || next_y >= (s32)SNAKE_GRID_HEIGHT) {
        state->snake_game_over = true;
        return true;
    }

    for (u32 index = 0; index < state->snake_length; ++index) {
        if ((s32)state->snake_x[index] == next_x && (s32)state->snake_y[index] == next_y) {
            state->snake_game_over = true;
            return true;
        }
    }

    ate_food = ((u8)next_x == state->snake_food_x && (u8)next_y == state->snake_food_y);
    old_length = state->snake_length;
    new_length = old_length;
    if (ate_food && new_length < sizeof(state->snake_x)) {
        ++new_length;
    }

    for (u32 index = new_length - 1u; index > 0u; --index) {
        u32 source = index - 1u;
        if (source >= old_length) {
            source = old_length - 1u;
        }
        state->snake_x[index] = state->snake_x[source];
        state->snake_y[index] = state->snake_y[source];
    }

    state->snake_length = new_length;
    state->snake_x[0] = (u8)next_x;
    state->snake_y[0] = (u8)next_y;
    state->snake_dir_x = dir_x;
    state->snake_dir_y = dir_y;

    if (ate_food) {
        ++state->snake_score;
        snake_spawn_food(state);
    }

    return true;
}

static void ttt_reset(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    memset(state->ttt_cells, 0, sizeof(state->ttt_cells));
    state->ttt_turn = 1u;
    state->ttt_winner = 0u;
    state->ttt_draw = false;
}

static void ttt_update_state(desktop_state_t* state) {
    static const u8 lines[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
        {0, 4, 8}, {2, 4, 6}
    };
    bool filled = true;

    if (state == NULL) {
        return;
    }

    for (u32 index = 0; index < 8u; ++index) {
        u8 a = lines[index][0];
        u8 b = lines[index][1];
        u8 c = lines[index][2];
        if (state->ttt_cells[a] != 0u &&
            state->ttt_cells[a] == state->ttt_cells[b] &&
            state->ttt_cells[b] == state->ttt_cells[c]) {
            state->ttt_winner = state->ttt_cells[a];
            state->ttt_draw = false;
            return;
        }
    }

    for (u32 index = 0; index < 9u; ++index) {
        if (state->ttt_cells[index] == 0u) {
            filled = false;
            break;
        }
    }

    state->ttt_draw = filled;
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

static bool string_equals_case_insensitive(const char* left, const char* right) {
    u32 index = 0;

    if (left == NULL || right == NULL) {
        return false;
    }

    while (left[index] != '\0' && right[index] != '\0') {
        if (to_lower_ascii(left[index]) != to_lower_ascii(right[index])) {
            return false;
        }
        ++index;
    }

    return left[index] == '\0' && right[index] == '\0';
}

static bool append_query_character(char* query, u32 capacity, char input) {
    u32 query_length;

    if (query == NULL || capacity == 0u || input == '\0') {
        return false;
    }

    query_length = (u32)strlen(query);
    if (query_length + 1u >= capacity) {
        return false;
    }

    query[query_length] = input;
    query[query_length + 1u] = '\0';
    return true;
}

static bool erase_query_character(char* query) {
    u32 query_length;

    if (query == NULL) {
        return false;
    }

    query_length = (u32)strlen(query);
    if (query_length == 0u) {
        return false;
    }

    query[query_length - 1u] = '\0';
    return true;
}

static void copy_text_limited(char* destination, u32 capacity, const char* source) {
    u32 index = 0u;

    if (destination == NULL || capacity == 0u) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    while (source[index] != '\0' && index + 1u < capacity) {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static char browser_display_char(char value) {
    u8 byte = (u8)value;
    if (byte == '\n') {
        return '\n';
    }
    if (byte == '\r' || byte == '\t') {
        return ' ';
    }
    if (byte < 32u || byte > 126u) {
        return '?';
    }
    return value;
}

static void browser_copy_display_text(char* destination, u32 capacity, const char* source) {
    u32 index = 0u;

    if (destination == NULL || capacity == 0u) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    while (source[index] != '\0' && index + 1u < capacity) {
        destination[index] = browser_display_char(source[index]);
        ++index;
    }
    destination[index] = '\0';
}

static const char* find_case_insensitive(const char* text, const char* token) {
    u32 token_length = (u32)strlen(token);

    if (text == NULL || token == NULL || token_length == 0u) {
        return NULL;
    }

    for (u32 start = 0u; text[start] != '\0'; ++start) {
        u32 index = 0u;
        while (token[index] != '\0' &&
               text[start + index] != '\0' &&
               to_lower_ascii(text[start + index]) == to_lower_ascii(token[index])) {
            ++index;
        }
        if (index == token_length) {
            return text + start;
        }
    }

    return NULL;
}

static bool string_starts_with_case_insensitive(const char* text, const char* prefix) {
    u32 index = 0u;

    if (text == NULL || prefix == NULL) {
        return false;
    }

    while (prefix[index] != '\0') {
        if (text[index] == '\0' || to_lower_ascii(text[index]) != to_lower_ascii(prefix[index])) {
            return false;
        }
        ++index;
    }

    return true;
}

static bool browser_is_active_window(const desktop_state_t* state) {
    if (state == NULL || !window_visible(state, WINDOW_BROWSER) || window_shaded(state, WINDOW_BROWSER)) {
        return false;
    }

    for (u32 index = WINDOW_STACK_SIZE; index > 0u; --index) {
        window_id_t window = state->window_order[index - 1u];
        if (window_visible(state, window) && !window_shaded(state, window)) {
            return window == WINDOW_BROWSER;
        }
    }

    return false;
}

static bool notepad_is_active_window(const desktop_state_t* state) {
    if (state == NULL || !window_visible(state, WINDOW_NOTEPAD) || window_shaded(state, WINDOW_NOTEPAD)) {
        return false;
    }

    for (u32 index = WINDOW_STACK_SIZE; index > 0u; --index) {
        window_id_t window = state->window_order[index - 1u];
        if (window_visible(state, window) && !window_shaded(state, window)) {
            return window == WINDOW_NOTEPAD;
        }
    }

    return false;
}

static void browser_reset_runtime(void) {
    memset(&g_browser, 0, sizeof(g_browser));
    copy_text_limited(g_browser.address, sizeof(g_browser.address), "");
    copy_text_limited(g_browser.status, sizeof(g_browser.status), "Ready");
    copy_text_limited(g_browser.title, sizeof(g_browser.title), "Jachron Browser");
    copy_text_limited(g_browser.content_type, sizeof(g_browser.content_type), "text/plain");
    copy_text_limited(g_browser.page_text, sizeof(g_browser.page_text), "");
    copy_text_limited(g_browser.download_text, sizeof(g_browser.download_text), "");
    copy_text_limited(g_browser_transfer_buffer, sizeof(g_browser_transfer_buffer), "");
    g_browser.page_length = 0u;
    g_browser.download_length = 0u;
    g_browser.last_status_code = 0u;
    g_browser.page_truncated = false;
    g_browser.download_truncated = false;
}

static void browser_extract_title(const char* html, char* out_title, u32 capacity) {
    const char* start;
    const char* end;
    u32 title_length = 0u;

    if (out_title == NULL || capacity == 0u) {
        return;
    }

    out_title[0] = '\0';
    if (html == NULL) {
        return;
    }

    start = find_case_insensitive(html, "<title>");
    if (start == NULL) {
        return;
    }
    start += 7;
    end = find_case_insensitive(start, "</title>");
    if (end == NULL) {
        return;
    }

    while (start[title_length] != '\0' && &start[title_length] < end && title_length + 1u < capacity) {
        char value = browser_display_char(start[title_length]);
        out_title[title_length] = value;
        ++title_length;
    }
    out_title[title_length] = '\0';
}

static void browser_push_render_char(char* destination, u32 capacity, u32* in_out_length, char value) {
    if (destination == NULL || in_out_length == NULL || capacity == 0u) {
        return;
    }
    if (*in_out_length + 1u >= capacity) {
        return;
    }
    destination[*in_out_length] = value;
    ++(*in_out_length);
    destination[*in_out_length] = '\0';
}

static void browser_push_newline(char* destination, u32 capacity, u32* in_out_length) {
    if (destination == NULL || in_out_length == NULL || capacity == 0u || *in_out_length == 0u) {
        return;
    }
    if (destination[*in_out_length - 1u] == '\n') {
        return;
    }
    browser_push_render_char(destination, capacity, in_out_length, '\n');
}

static void browser_push_bullet(char* destination, u32 capacity, u32* in_out_length) {
    browser_push_render_char(destination, capacity, in_out_length, '*');
    browser_push_render_char(destination, capacity, in_out_length, ' ');
}

static bool browser_tag_has_line_break(const char* tag_start) {
    return string_starts_with_case_insensitive(tag_start, "<br") ||
           string_starts_with_case_insensitive(tag_start, "<p") ||
           string_starts_with_case_insensitive(tag_start, "</p") ||
           string_starts_with_case_insensitive(tag_start, "<div") ||
           string_starts_with_case_insensitive(tag_start, "</div") ||
           string_starts_with_case_insensitive(tag_start, "<section") ||
           string_starts_with_case_insensitive(tag_start, "</section") ||
           string_starts_with_case_insensitive(tag_start, "<article") ||
           string_starts_with_case_insensitive(tag_start, "</article") ||
           string_starts_with_case_insensitive(tag_start, "<h") ||
           string_starts_with_case_insensitive(tag_start, "</h") ||
           string_starts_with_case_insensitive(tag_start, "<tr") ||
           string_starts_with_case_insensitive(tag_start, "</tr") ||
           string_starts_with_case_insensitive(tag_start, "<ul") ||
           string_starts_with_case_insensitive(tag_start, "</ul") ||
           string_starts_with_case_insensitive(tag_start, "<ol") ||
           string_starts_with_case_insensitive(tag_start, "</ol");
}

static const char* browser_skip_tag_close_marker(const char* tag_start) {
    if (string_starts_with_case_insensitive(tag_start, "<script")) {
        return "</script";
    }
    if (string_starts_with_case_insensitive(tag_start, "<style")) {
        return "</style";
    }
    if (string_starts_with_case_insensitive(tag_start, "<head")) {
        return "</head";
    }
    if (string_starts_with_case_insensitive(tag_start, "<svg")) {
        return "</svg";
    }
    return NULL;
}

static void browser_decode_entity(const char* source, u32* index, char* out_value) {
    if (source == NULL || index == NULL || out_value == NULL) {
        return;
    }

    *out_value = source[*index];
    if (strncmp(source + *index, "&amp;", 5u) == 0) {
        *out_value = '&';
        *index += 4u;
    } else if (strncmp(source + *index, "&lt;", 4u) == 0) {
        *out_value = '<';
        *index += 3u;
    } else if (strncmp(source + *index, "&gt;", 4u) == 0) {
        *out_value = '>';
        *index += 3u;
    } else if (strncmp(source + *index, "&nbsp;", 6u) == 0) {
        *out_value = ' ';
        *index += 5u;
    } else if (strncmp(source + *index, "&quot;", 6u) == 0) {
        *out_value = '"';
        *index += 5u;
    } else if (strncmp(source + *index, "&#39;", 5u) == 0) {
        *out_value = '\'';
        *index += 4u;
    }
}

static bool browser_text_needs_search(const char* input) {
    if (input == NULL || input[0] == '\0') {
        return false;
    }

    for (u32 index = 0u; input[index] != '\0'; ++index) {
        char value = input[index];
        if (value == ' ' || value == '\t') {
            return true;
        }
    }

    if (string_starts_with_case_insensitive(input, "http://") ||
        string_starts_with_case_insensitive(input, "https://")) {
        return false;
    }

    for (u32 index = 0u; input[index] != '\0'; ++index) {
        char value = input[index];
        if (value == '.' || value == '/' || value == ':' || value == '?') {
            return false;
        }
    }

    return true;
}

static char browser_hex_digit(u8 value) {
    return (char)(value < 10u ? ('0' + value) : ('A' + (value - 10u)));
}

static void browser_append_char(char* destination, u32 capacity, char value) {
    u32 length;

    if (destination == NULL || capacity == 0u) {
        return;
    }

    length = (u32)strlen(destination);
    if (length + 1u >= capacity) {
        return;
    }

    destination[length] = value;
    destination[length + 1u] = '\0';
}

static void browser_append_url_encoded(char* destination, u32 capacity, const char* source) {
    for (u32 index = 0u; source != NULL && source[index] != '\0'; ++index) {
        u8 value = (u8)source[index];

        if ((value >= 'a' && value <= 'z') ||
            (value >= 'A' && value <= 'Z') ||
            (value >= '0' && value <= '9') ||
            value == '-' || value == '_' || value == '.' || value == '~') {
            browser_append_char(destination, capacity, (char)value);
        } else if (value == ' ') {
            browser_append_char(destination, capacity, '+');
        } else {
            browser_append_char(destination, capacity, '%');
            browser_append_char(destination, capacity, browser_hex_digit((u8)(value >> 4u)));
            browser_append_char(destination, capacity, browser_hex_digit((u8)(value & 0x0Fu)));
        }
    }
}

static void browser_build_google_search_url(const char* query, char* out_url, u32 capacity) {
    if (out_url == NULL || capacity == 0u) {
        return;
    }

    out_url[0] = '\0';
    copy_text_limited(out_url, capacity, "http://frogfind.com/?q=");
    browser_append_url_encoded(out_url, capacity, query);
}

static void browser_normalize_url(const char* input, char* output, u32 capacity) {
    const char* start = input;
    u32 length = 0u;
    u32 prefix_length;

    if (output == NULL || capacity == 0u) {
        return;
    }

    output[0] = '\0';
    if (input == NULL) {
        return;
    }

    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        ++start;
    }
    while (start[length] != '\0') {
        ++length;
    }
    while (length > 0u &&
           (start[length - 1u] == ' ' || start[length - 1u] == '\t' ||
            start[length - 1u] == '\r' || start[length - 1u] == '\n')) {
        --length;
    }
    if (length == 0u) {
        return;
    }

    if (string_starts_with_case_insensitive(start, "http://") ||
        string_starts_with_case_insensitive(start, "https://")) {
        if (length >= capacity) {
            length = capacity - 1u;
        }
        memcpy(output, start, length);
        output[length] = '\0';
        return;
    }

    copy_text_limited(output, capacity, "http://");
    prefix_length = (u32)strlen(output);
    if (prefix_length < capacity - 1u) {
        u32 remaining = capacity - 1u - prefix_length;
        if (length > remaining) {
            length = remaining;
        }
        memcpy(output + prefix_length, start, length);
        output[prefix_length + length] = '\0';
    }
}

static void browser_render_html_text(const char* source, char* destination, u32 capacity) {
    bool in_tag = false;
    bool previous_space = true;
    bool clear_skip_after_tag = false;
    u32 out_length = 0u;
    const char* skip_tag_close_marker = NULL;

    if (destination == NULL || capacity == 0u) {
        return;
    }

    destination[0] = '\0';
    if (source == NULL) {
        return;
    }

    for (u32 index = 0u; source[index] != '\0'; ++index) {
        char value = source[index];

        if (skip_tag_close_marker != NULL && !in_tag) {
            if (value == '<' && string_starts_with_case_insensitive(source + index, skip_tag_close_marker)) {
                in_tag = true;
                clear_skip_after_tag = true;
            }
            continue;
        }

        if (!in_tag && value == '<') {
            const char* skip_marker = browser_skip_tag_close_marker(source + index);
            if (skip_marker != NULL) {
                skip_tag_close_marker = skip_marker;
                in_tag = true;
                continue;
            }
            if (string_starts_with_case_insensitive(source + index, "<li")) {
                browser_push_newline(destination, capacity, &out_length);
                browser_push_bullet(destination, capacity, &out_length);
                previous_space = false;
            } else if (browser_tag_has_line_break(source + index)) {
                browser_push_newline(destination, capacity, &out_length);
                previous_space = true;
            }
            in_tag = true;
            continue;
        }
        if (in_tag) {
            if (value == '>') {
                in_tag = false;
                if (clear_skip_after_tag) {
                    skip_tag_close_marker = NULL;
                    clear_skip_after_tag = false;
                }
            }
            continue;
        }

        if (value == '&') {
            browser_decode_entity(source, &index, &value);
        }

        if (value == '\r' || value == '\n' || value == '\t') {
            value = ' ';
        }
        if (value == ' ') {
            if (!previous_space) {
                browser_push_render_char(destination, capacity, &out_length, ' ');
                previous_space = true;
            }
            continue;
        }

        browser_push_render_char(destination, capacity, &out_length, browser_display_char(value));
        previous_space = false;
    }
}

static u32* browser_scroll_slot(u32 page) {
    if (page >= (sizeof(g_browser.scroll_lines) / sizeof(g_browser.scroll_lines[0]))) {
        return &g_browser.scroll_lines[0];
    }
    return &g_browser.scroll_lines[page];
}

static void browser_reset_scroll(u32 page) {
    u32* scroll = browser_scroll_slot(page);
    if (scroll != NULL) {
        *scroll = 0u;
    }
}

static void browser_reset_all_scroll(void) {
    for (u32 index = 0u; index < (sizeof(g_browser.scroll_lines) / sizeof(g_browser.scroll_lines[0])); ++index) {
        g_browser.scroll_lines[index] = 0u;
    }
}

static void browser_scroll_page(u32 page, s32 delta) {
    u32* scroll = browser_scroll_slot(page);
    if (scroll == NULL) {
        return;
    }
    if (delta < 0) {
        u32 amount = (u32)(-delta);
        *scroll = (*scroll > amount) ? (*scroll - amount) : 0u;
    } else {
        *scroll += (u32)delta;
    }
}

static void browser_format_status_text(char* destination, u32 capacity, const net_http_result_t* result) {
    char status_number[16];

    if (destination == NULL || capacity == 0u || result == NULL) {
        return;
    }

    destination[0] = '\0';
    append_text(destination, result->success ? "HTTP " : "HTTP issue ");
    append_uint(status_number, result->status_code);
    append_text(destination + strlen(destination), status_number);
    if (result->redirected) {
        append_text(destination + strlen(destination), " redirected");
    }
    if (result->truncated) {
        append_text(destination + strlen(destination), " truncated");
    }
}

static bool browser_navigate_input(const char* input) {
    char google_url[NET_HTTP_URL_MAX];
    char normalized_url[NET_HTTP_URL_MAX];

    if (input == NULL || input[0] == '\0') {
        return false;
    }

    if (browser_text_needs_search(input)) {
        browser_build_google_search_url(input, google_url, sizeof(google_url));
        return browser_load_url(google_url);
    }

    browser_normalize_url(input, normalized_url, sizeof(normalized_url));
    return browser_load_url(normalized_url) || normalized_url[0] != '\0';
}

static bool browser_load_url(const char* url) {
    net_http_result_t result;
    char normalized_url[NET_HTTP_URL_MAX];
    char plain_title[BROWSER_TITLE_LENGTH];

    if (url == NULL || url[0] == '\0') {
        return false;
    }

    browser_normalize_url(url, normalized_url, sizeof(normalized_url));
    if (normalized_url[0] == '\0') {
        return false;
    }

    if (!net_http_get(normalized_url, g_browser_transfer_buffer, sizeof(g_browser_transfer_buffer), &result)) {
        copy_text_limited(g_browser.status, sizeof(g_browser.status), result.message);
        copy_text_limited(g_browser.title, sizeof(g_browser.title), "Browser Error");
        copy_text_limited(g_browser.page_text,
                          sizeof(g_browser.page_text),
                          "The browser could not complete that HTTP request.");
        g_browser.page_length = (u32)strlen(g_browser.page_text);
        g_browser.page_truncated = false;
        g_browser.last_status_code = 0u;
        copy_text_limited(g_browser.address, sizeof(g_browser.address), normalized_url);
        browser_reset_scroll(BROWSER_PAGE_WEB);
        browser_reset_scroll(BROWSER_PAGE_SOURCE);
        return false;
    }

    copy_text_limited(g_browser.address, sizeof(g_browser.address), result.final_url);
    copy_text_limited(g_browser.content_type, sizeof(g_browser.content_type), result.content_type);
    browser_format_status_text(g_browser.status, sizeof(g_browser.status), &result);
    g_browser.last_status_code = result.status_code;
    g_browser.page_truncated = result.truncated;

    plain_title[0] = '\0';
    if (string_contains_case_insensitive(result.content_type, "html")) {
        browser_extract_title(g_browser_transfer_buffer, plain_title, sizeof(plain_title));
        browser_render_html_text(g_browser_transfer_buffer, g_browser.page_text, sizeof(g_browser.page_text));
    } else {
        browser_copy_display_text(g_browser.page_text, sizeof(g_browser.page_text), g_browser_transfer_buffer);
    }

    if (g_browser.page_text[0] == '\0') {
        copy_text_limited(g_browser.page_text, sizeof(g_browser.page_text), "The HTTP response body was empty.");
    }
    g_browser.page_length = (u32)strlen(g_browser.page_text);
    copy_text_limited(g_browser.title,
                      sizeof(g_browser.title),
                      plain_title[0] != '\0' ? plain_title : result.final_url);
    browser_reset_scroll(BROWSER_PAGE_WEB);
    browser_reset_scroll(BROWSER_PAGE_SOURCE);
    return result.success;
}

static bool browser_download_url(const char* url) {
    net_http_result_t result;
    char normalized_url[NET_HTTP_URL_MAX];

    if (url == NULL || url[0] == '\0') {
        return false;
    }

    browser_normalize_url(url, normalized_url, sizeof(normalized_url));
    if (normalized_url[0] == '\0') {
        return false;
    }

    if (!net_http_get(normalized_url, g_browser.download_text, sizeof(g_browser.download_text), &result)) {
        copy_text_limited(g_browser.download_url, sizeof(g_browser.download_url), normalized_url);
        copy_text_limited(g_browser.download_text, sizeof(g_browser.download_text), result.message);
        g_browser.download_length = 0u;
        g_browser.download_truncated = false;
        copy_text_limited(g_browser.status, sizeof(g_browser.status), "HTTP download failed");
        browser_reset_scroll(BROWSER_PAGE_DOWNLOADS);
        return false;
    }

    copy_text_limited(g_browser.download_url, sizeof(g_browser.download_url), result.final_url);
    g_browser.download_truncated = result.truncated;
    browser_format_status_text(g_browser.status, sizeof(g_browser.status), &result);
    if (g_browser.download_text[0] == '\0') {
        copy_text_limited(g_browser.download_text, sizeof(g_browser.download_text), "Downloaded response had no body.");
    }
    g_browser.download_length = (u32)strlen(g_browser.download_text);
    browser_reset_scroll(BROWSER_PAGE_DOWNLOADS);
    return result.success;
}

static bool find_first_launcher_match(const char* query, u32* out_index) {
    for (u32 app_index = 0; app_index < LAUNCHER_APP_COUNT; ++app_index) {
        if (g_launcher_window_ids[app_index] == WINDOW_JABVER) {
            continue;
        }
        if (string_contains_case_insensitive(g_launcher_app_names[app_index], query)) {
            if (out_index != NULL) {
                *out_index = app_index;
            }
            return true;
        }
    }

    return false;
}

static bool resolve_run_match(const char* query, window_id_t* out_window, const char** out_label) {
    if (query == NULL || query[0] == '\0') {
        return false;
    }

    if (string_equals_case_insensitive(query, "files")) {
        if (out_window != NULL) {
            *out_window = WINDOW_FILES;
        }
        if (out_label != NULL) {
            *out_label = "File Explorer";
        }
        return true;
    }

    if (string_equals_case_insensitive(query, "browser")) {
        if (out_window != NULL) {
            *out_window = WINDOW_BROWSER;
        }
        if (out_label != NULL) {
            *out_label = "Firefox";
        }
        return true;
    }

    if (string_equals_case_insensitive(query, "benchmark") ||
        string_equals_case_insensitive(query, "tasks") ||
        string_equals_case_insensitive(query, "task manager")) {
        if (out_window != NULL) {
            *out_window = WINDOW_TASKS;
        }
        if (out_label != NULL) {
            *out_label = "Task Manager";
        }
        return true;
    }

    if (string_equals_case_insensitive(query, "media") ||
        string_equals_case_insensitive(query, "jabulmedia") ||
        string_equals_case_insensitive(query, "videos")) {
        if (out_window != NULL) {
            *out_window = WINDOW_VIDEOS;
        }
        if (out_label != NULL) {
            *out_label = "jabulmedia";
        }
        return true;
    }

    if (string_equals_case_insensitive(query, "winver")) {
        if (out_window != NULL) {
            *out_window = WINDOW_JABVER;
        }
        if (out_label != NULL) {
            *out_label = "Jabver";
        }
        return true;
    }

    for (u32 index = 0; index < RUN_APP_COUNT; ++index) {
        if (string_contains_case_insensitive(g_run_app_names[index], query)) {
            if (out_window != NULL) {
                *out_window = g_run_window_ids[index];
            }
            if (out_label != NULL) {
                *out_label = g_run_app_names[index];
            }
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
    return 3u + ((kernel_size + 511u) / 512u) + ((initrd_size + 511u) / 512u);
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
        case WINDOW_RUN:
            return &state->show_run;
        case WINDOW_SETTINGS:
            return &state->show_settings;
        case WINDOW_TERMINAL:
            return &state->show_terminal;
        case WINDOW_NOTEPAD:
            return &state->show_notepad;
        case WINDOW_FILES:
            return &state->show_files;
        case WINDOW_TASKS:
            return &state->show_tasks;
        case WINDOW_VIDEOS:
            return &state->show_videos;
        case WINDOW_BROWSER:
            return &state->show_browser;
        case WINDOW_GAMES:
            return &state->show_games;
        case WINDOW_SNAKE:
            return &state->show_snake;
        case WINDOW_JABVER:
            return &state->show_jabver;
        case WINDOW_WELCOME:
            return &state->show_welcome;
        case WINDOW_LEGAL:
            return &state->show_legal;
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
        case WINDOW_RUN:
            return &state->minimized_run;
        case WINDOW_SETTINGS:
            return &state->minimized_settings;
        case WINDOW_TERMINAL:
            return &state->minimized_terminal;
        case WINDOW_NOTEPAD:
            return &state->minimized_notepad;
        case WINDOW_FILES:
            return &state->minimized_files;
        case WINDOW_TASKS:
            return &state->minimized_tasks;
        case WINDOW_VIDEOS:
            return &state->minimized_videos;
        case WINDOW_BROWSER:
            return &state->minimized_browser;
        case WINDOW_GAMES:
            return &state->minimized_games;
        case WINDOW_SNAKE:
            return &state->minimized_snake;
        case WINDOW_JABVER:
            return &state->minimized_jabver;
        case WINDOW_WELCOME:
            return &state->minimized_welcome;
        case WINDOW_LEGAL:
            return &state->minimized_legal;
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
        case WINDOW_RUN:
            return &state->shaded_run;
        case WINDOW_SETTINGS:
            return &state->shaded_settings;
        case WINDOW_TERMINAL:
            return &state->shaded_terminal;
        case WINDOW_NOTEPAD:
            return &state->shaded_notepad;
        case WINDOW_FILES:
            return &state->shaded_files;
        case WINDOW_TASKS:
            return &state->shaded_tasks;
        case WINDOW_VIDEOS:
            return &state->shaded_videos;
        case WINDOW_BROWSER:
            return &state->shaded_browser;
        case WINDOW_GAMES:
            return &state->shaded_games;
        case WINDOW_SNAKE:
            return &state->shaded_snake;
        case WINDOW_JABVER:
            return &state->shaded_jabver;
        case WINDOW_WELCOME:
            return &state->shaded_welcome;
        case WINDOW_LEGAL:
            return &state->shaded_legal;
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

static bool* get_window_fullscreen_flag(desktop_state_t* state, window_id_t window) {
    if (state == NULL || window == WINDOW_NONE || (u32)window >= WINDOW_STATE_COUNT) {
        return NULL;
    }
    return &state->window_fullscreen[(u32)window];
}

static window_rect_t* get_window_fullscreen_restore_rect(desktop_state_t* state, window_id_t window) {
    if (state == NULL || window == WINDOW_NONE || (u32)window >= WINDOW_STATE_COUNT) {
        return NULL;
    }
    return &state->fullscreen_restore_rects[(u32)window];
}

static void initialize_window_order(desktop_state_t* state) {
    static const window_id_t default_order[WINDOW_STACK_SIZE] = {
        WINDOW_SEARCH,
        WINDOW_RUN,
        WINDOW_SETTINGS,
        WINDOW_TERMINAL,
        WINDOW_NOTEPAD,
        WINDOW_FILES,
        WINDOW_TASKS,
        WINDOW_VIDEOS,
        WINDOW_BROWSER,
        WINDOW_GAMES,
        WINDOW_SNAKE,
        WINDOW_JABVER,
        WINDOW_LEGAL,
        WINDOW_WELCOME
    };

    if (state == NULL) {
        return;
    }

    memcpy(state->window_order, default_order, sizeof(default_order));
}

static s32 find_window_order_index(const desktop_state_t* state, window_id_t window) {
    if (state == NULL || window == WINDOW_NONE) {
        return -1;
    }

    for (u32 index = 0; index < WINDOW_STACK_SIZE; ++index) {
        if (state->window_order[index] == window) {
            return (s32)index;
        }
    }

    return -1;
}

static void bring_window_to_front(desktop_state_t* state, window_id_t window) {
    s32 source_index;

    if (state == NULL || window == WINDOW_NONE) {
        return;
    }

    source_index = find_window_order_index(state, window);
    if (source_index < 0 || source_index == (s32)(WINDOW_STACK_SIZE - 1u)) {
        return;
    }

    for (u32 index = (u32)source_index; index + 1u < WINDOW_STACK_SIZE; ++index) {
        state->window_order[index] = state->window_order[index + 1u];
    }
    state->window_order[WINDOW_STACK_SIZE - 1u] = window;
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
    if (window == WINDOW_SEARCH) {
        position_search_panel(state);
    }
    bring_window_to_front(state, window);
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
    if (state->dragging_window == window) {
        state->dragging_window = WINDOW_NONE;
    }
}

static bool activate_run_dialog(desktop_state_t* state) {
    window_id_t target;

    if (state == NULL) {
        return false;
    }

    if (!resolve_run_match(state->run_query, &target, NULL)) {
        return false;
    }

    open_window(state, target);
    close_window(state, WINDOW_RUN);
    state->show_welcome = false;
    state->run_query[0] = '\0';
    return true;
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

static void set_window_fullscreen_rect(window_rect_t* rect) {
    if (rect == NULL) {
        return;
    }

    rect->x = 12u;
    rect->y = TOP_BAR_HEIGHT + 4u;
    rect->width = display_driver_width() > 24u ? (display_driver_width() - 24u) : display_driver_width();
    rect->height = display_driver_height() > (TOP_BAR_HEIGHT + 92u) ?
                   (display_driver_height() - TOP_BAR_HEIGHT - 92u) : (display_driver_height() / 2u);
}

static void toggle_window_fullscreen(desktop_state_t* state, window_id_t window) {
    bool* fullscreen_flag;
    bool* shaded_flag;
    window_rect_t* rect;
    window_rect_t* restore_rect;

    if (state == NULL || window == WINDOW_NONE || window == WINDOW_SEARCH) {
        return;
    }

    fullscreen_flag = get_window_fullscreen_flag(state, window);
    shaded_flag = get_window_shaded_flag(state, window);
    rect = get_window_rect(state, window);
    restore_rect = get_window_fullscreen_restore_rect(state, window);
    if (fullscreen_flag == NULL || rect == NULL || restore_rect == NULL) {
        return;
    }

    if (*fullscreen_flag) {
        *rect = *restore_rect;
        clamp_window_rect(rect);
        *fullscreen_flag = false;
        return;
    }

    *restore_rect = *rect;
    set_window_fullscreen_rect(rect);
    *fullscreen_flag = true;
    if (shaded_flag != NULL) {
        *shaded_flag = false;
    }
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
        case APP_BROWSER:
            return WINDOW_BROWSER;
        case APP_VIDEOS:
            return WINDOW_VIDEOS;
        case APP_SNAKE:
            return WINDOW_SNAKE;
        case APP_JABVER:
            return WINDOW_JABVER;
        default:
            return WINDOW_NONE;
    }
}

static const char* dock_app_name(dock_app_t app) {
    switch (app) {
        case APP_START: return "Start";
        case APP_SETTINGS: return "Settings";
        case APP_TERMINAL: return "Terminal";
        case APP_NOTEPAD: return "Notepad";
        case APP_FILES: return "File Manager";
        case APP_TASKS: return "Task Manager";
        case APP_BROWSER: return "Browser";
        case APP_VIDEOS: return "Videos";
        case APP_SNAKE: return "Snake";
        case APP_JABVER: return "Jabver";
        default: return "App";
    }
}

static bool is_dock_app_default(dock_app_t app) {
    return app == APP_START || app == APP_SETTINGS || app == APP_FILES || app == APP_TASKS || app == APP_BROWSER;
}

static u32 get_visible_dock_apps(const desktop_state_t* state, dock_app_t* apps_out) {
    u32 count = 0;
    if (apps_out != NULL) {
        apps_out[count++] = APP_START;
    } else {
        count++;
    }

    for (int i = 1; i <= (int)APP_JABVER; i++) {
        dock_app_t app = (dock_app_t)i;
        if (is_dock_app_default(app) || (state != NULL && dock_app_active(state, app))) {
            if (apps_out != NULL) {
                apps_out[count++] = app;
            } else {
                count++;
            }
        }
    }
    return count;
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
        case 0x0C: return '-';
        case 0x0D: return '=';
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
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x39: return ' ';
        default:   return '\0';
    }
}

static void draw_progress_bar(u32 x, u32 y, u32 width, u32 height, u32 percent, u32 fill_color) {
    u32 clamped = percent > 100u ? 100u : percent;
    u32 track_inset = width > 6u && height > 6u ? 3u : 1u;
    u32 inner_width = width > track_inset * 2u ? width - track_inset * 2u : width;
    u32 inner_height = height > track_inset * 2u ? height - track_inset * 2u : height;
    u32 fill = (inner_width * clamped) / 100u;

    draw_rounded_rect(x, y, width, height, height / 2u, vga_color(28, 34, 48));
    vga_fill_rounded_rect_alpha(x + 1u, y + 1u, width > 2u ? width - 2u : width, height > 2u ? height - 2u : height,
                                height > 2u ? (height - 2u) / 2u : height / 2u, 255, 255, 255, 10);
    if (fill > 0u) {
        draw_rounded_rect(x + track_inset,
                          y + track_inset,
                          fill,
                          inner_height,
                          inner_height / 2u,
                          fill_color);
        vga_fill_rounded_rect_alpha(x + track_inset + 1u,
                                    y + track_inset + 1u,
                                    fill > 2u ? fill - 2u : fill,
                                    inner_height > 2u ? inner_height - 2u : inner_height,
                                    inner_height > 2u ? (inner_height - 2u) / 2u : inner_height / 2u,
                                    255, 255, 255,
                                    18);
    }
}

static void get_window_button_position(const desktop_state_t* state,
                                       const window_rect_t* rect,
                                       window_button_t button,
                                       u32* out_x,
                                       u32* out_y) {
    u32 button_y;
    u32 button_x;

    (void)state;

    if (rect == NULL || out_x == NULL || out_y == NULL || button == WINDOW_BUTTON_NONE) {
        return;
    }

    button_y = rect->y + 10u;
    button_x = rect->x + rect->width - 26u;
    if (button == WINDOW_BUTTON_FULLSCREEN) {
        button_x -= WINDOW_BUTTON_SIZE + WINDOW_BUTTON_SPACING;
    }

    *out_x = button_x;
    *out_y = button_y;
}

static bool point_in_window_button_hitbox(const desktop_state_t* state,
                                          const window_rect_t* rect,
                                          window_button_t button,
                                          u32 mouse_x,
                                          u32 mouse_y) {
    u32 button_x;
    u32 button_y;
    u32 hit_size = WINDOW_BUTTON_SIZE + 8u;

    if (rect == NULL || button == WINDOW_BUTTON_NONE) {
        return false;
    }

    get_window_button_position(state, rect, button, &button_x, &button_y);
    return point_in_rect(mouse_x, mouse_y, safe_subtract(button_x, 4u), safe_subtract(button_y, 4u), hit_size, hit_size);
}

static window_button_t hit_test_window_button(const desktop_state_t* state,
                                              const window_rect_t* rect,
                                              u32 mouse_x,
                                              u32 mouse_y) {
    if (rect == NULL) {
        return WINDOW_BUTTON_NONE;
    }
    if (state != NULL && rect == &state->search_rect) {
        return WINDOW_BUTTON_NONE;
    }

    if (point_in_window_button_hitbox(state, rect, WINDOW_BUTTON_CLOSE, mouse_x, mouse_y)) {
        return WINDOW_BUTTON_CLOSE;
    }
    if (point_in_window_button_hitbox(state, rect, WINDOW_BUTTON_FULLSCREEN, mouse_x, mouse_y)) {
        return WINDOW_BUTTON_FULLSCREEN;
    }
    return WINDOW_BUTTON_NONE;
}

static void draw_window_button(u32 x, u32 y, u32 base_color, u32 glyph_color, window_button_t button) {
    draw_rounded_rect(x, y, WINDOW_BUTTON_SIZE, WINDOW_BUTTON_SIZE, 5u, base_color);
    vga_fill_rect_alpha(x + 1u, y + 1u, WINDOW_BUTTON_SIZE - 2u, 5u, 255, 255, 255, 54);

    if (button == WINDOW_BUTTON_CLOSE) {
        draw_rect(x + 4u, y + 4u, 2u, 2u, glyph_color);
        draw_rect(x + 10u, y + 4u, 2u, 2u, glyph_color);
        draw_rect(x + 6u, y + 6u, 2u, 2u, glyph_color);
        draw_rect(x + 8u, y + 8u, 2u, 2u, glyph_color);
        draw_rect(x + 4u, y + 10u, 2u, 2u, glyph_color);
        draw_rect(x + 10u, y + 10u, 2u, 2u, glyph_color);
        draw_rect(x + 6u, y + 8u, 2u, 2u, glyph_color);
        draw_rect(x + 8u, y + 6u, 2u, 2u, glyph_color);
    } else if (button == WINDOW_BUTTON_FULLSCREEN) {
        draw_rect(x + 6u, y + 4u, 4u, 2u, glyph_color);
        draw_rect(x + 4u, y + 6u, 2u, 4u, glyph_color);
        draw_rect(x + 10u, y + 6u, 2u, 4u, glyph_color);
        draw_rect(x + 6u, y + 10u, 4u, 2u, glyph_color);
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
    bool dark = desktop_dark_mode(state);
    u32 frame = dark ? ui_surface_color(state, 2u) : vga_color(224, 228, 244);
    u32 stroke = dark ? ui_stroke_color(state, 1u) : vga_color(202, 208, 232);
    u32 title_text = ui_text_primary(state);
    u32 inner_fill = ui_surface_color(state, 0u);
    u32 title_fill = dark ? ui_surface_color(state, 3u) : vga_color(234, 236, 248);
    u32 glyph = vga_color(255, 255, 255);
    u32 button_x;
    u32 button_y;
    u32 title_x = x + 18u;
    u32 title_y = y + ((title_height > vga_text_height()) ? ((title_height - vga_text_height()) / 2u) : 0u);
    u32 title_width;
    u32 shadow_x = x > 16u ? x - 16u : 0u;
    u32 soft_x = x > 6u ? x - 6u : 0u;
    u32 shadow_y = y > 6u ? y - 6u : 0u;
    u32 inner_radius = WINDOW_CORNER_RADIUS > 1u ? WINDOW_CORNER_RADIUS - 1u : 0u;
    u32 body_radius = WINDOW_CONTENT_RADIUS;
    window_rect_t frame_rect = {x, y, width, height};

    if (body_radius * 2u > width - 4u) {
        body_radius = (width > 4u) ? (width - 4u) / 2u : 0u;
    }
    if (body_radius * 2u > height - title_height - 3u) {
        body_radius = (height > title_height + 3u) ? (height - title_height - 3u) / 2u : 0u;
    }

    if (state != NULL && state->transparency_enabled) {
        vga_fill_rounded_rect_alpha(shadow_x, y + 14u, width + (x - shadow_x) + 16u, height + 28u,
                                    WINDOW_CORNER_RADIUS + 14u, dark ? 8 : 56, dark ? 12 : 44, dark ? 24 : 122, dark ? 64 : 40);
        vga_fill_rounded_rect_alpha(soft_x, shadow_y, width + (x - soft_x) + 6u, height + 16u,
                                    WINDOW_CORNER_RADIUS + 6u, 255, 255, 255, dark ? 6 : 10);
    }
    draw_rounded_rect(x, y, width, height, WINDOW_CORNER_RADIUS, frame);
    draw_rounded_rect(x + 1u, y + 1u, width - 2u, height - 2u, inner_radius, inner_fill);
    if (state != NULL && state->transparency_enabled) {
        vga_fill_rounded_rect_alpha(x + 1u, y + 1u, width - 2u, height - 2u, inner_radius, 255, 255, 255, 162u);
    }
    draw_rounded_rect(x + 1u, y + 1u, width - 2u, title_height + 8u, inner_radius, title_fill);
    if (state != NULL && state->transparency_enabled) {
        vga_fill_rect_alpha(x + 12u, y + 8u, width - 24u, 12u, 255, 255, 255, 34u);
    }
    draw_rounded_rect_outline(x, y, width, height, WINDOW_CORNER_RADIUS, 1u, stroke);
    draw_rect(x + 1u, y + title_height, width - 2u, 1u, body_tint);
    draw_rounded_rect(x + 2u, y + title_height + 1u, width - 4u, height - title_height - 3u, body_radius, inner_fill);
    if (state != NULL && state->transparency_enabled) {
        vga_fill_rect_alpha(x + 2u, y + title_height + 1u, width - 4u, height - title_height - 3u,
                            255, 255, 255, 20u);
    }

    get_window_button_position(state, &frame_rect, WINDOW_BUTTON_FULLSCREEN, &button_x, &button_y);
    draw_window_button(button_x, button_y, vga_color(72, 188, 96), glyph, WINDOW_BUTTON_FULLSCREEN);
    get_window_button_position(state, &frame_rect, WINDOW_BUTTON_CLOSE, &button_x, &button_y);
    draw_window_button(button_x, button_y, vga_color(226, 76, 76), glyph, WINDOW_BUTTON_CLOSE);
    title_width = button_x > title_x + 12u ? (button_x - title_x - 12u) : 0u;
    draw_text_clipped(title_x, title_y, title_width, title, title_text);
}

static void draw_dragged_window_preview(const desktop_state_t* state,
                                        window_id_t window,
                                        const window_rect_t* rect) {
    const char* title = "Window";
    u32 accent = vga_color(86, 118, 226);
    u32 title_height;
    u32 body_y;
    u32 body_height;
    u32 body_width;
    bool dark;
    u32 panel_fill;
    u32 panel_outline;
    u32 text;

    if (rect == NULL) {
        return;
    }

    switch (window) {
        case WINDOW_SEARCH:
            title = "Launcher";
            accent = vga_color(86, 118, 226);
            break;
        case WINDOW_RUN:
            title = "Run Dialog";
            accent = vga_color(86, 118, 226);
            break;
        case WINDOW_SETTINGS:
            title = "Settings";
            accent = vga_color(92, 132, 210);
            break;
        case WINDOW_TERMINAL:
            title = "Terminal";
            accent = vga_color(120, 156, 255);
            break;
        case WINDOW_FILES:
            title = "File Manager";
            accent = vga_color(214, 170, 88);
            break;
        case WINDOW_TASKS:
            title = "Task Manager";
            accent = vga_color(142, 112, 214);
            break;
        case WINDOW_VIDEOS:
            title = "File Preview";
            accent = vga_color(86, 118, 226);
            break;
        case WINDOW_BROWSER:
            title = "Browser";
            accent = vga_color(84, 118, 226);
            break;
        case WINDOW_GAMES:
            title = "Games";
            accent = vga_color(198, 156, 255);
            break;
        case WINDOW_SNAKE:
            title = "Snake";
            accent = vga_color(72, 188, 112);
            break;
        case WINDOW_JABVER:
            title = "jabver";
            accent = vga_color(84, 118, 226);
            break;
        case WINDOW_WELCOME:
        case WINDOW_LEGAL:
            title = "Welcome";
            accent = vga_color(120, 156, 244);
            break;
        default:
            break;
    }

    draw_window_frame(state, rect->x, rect->y, rect->width, rect->height, title, accent);

    title_height = window_title_height(window);
    body_y = rect->y + title_height + 18u;
    body_height = rect->height > title_height + 40u ? rect->height - title_height - 34u : 0u;
    body_width = rect->width > 28u ? rect->width - 28u : 0u;
    dark = desktop_dark_mode(state);
    panel_fill = dark ? vga_color(20, 24, 36) : vga_color(248, 250, 255);
    panel_outline = dark ? vga_color(52, 58, 82) : vga_color(216, 224, 242);
    text = dark ? vga_color(208, 216, 236) : vga_color(86, 98, 132);

    if (body_width == 0u || body_height == 0u) {
        return;
    }

    draw_rounded_panel(rect->x + 14u,
                       body_y,
                       body_width,
                       body_height,
                       WINDOW_CONTENT_RADIUS,
                       panel_fill,
                       panel_outline);
    if (body_height > 24u) {
        for (u32 row = body_y + 18u; row + 10u < body_y + body_height; row += 22u) {
            u32 line_width = body_width > 80u ? body_width - 34u : body_width;
            vga_fill_rect_alpha(rect->x + 26u, row, line_width, 8u,
                                dark ? 188u : 160u,
                                dark ? 200u : 176u,
                                dark ? 220u : 196u,
                                dark ? 28u : 44u);
        }
    }
    draw_text_clipped(rect->x + 26u,
                      body_y + 14u,
                      body_width > 32u ? body_width - 32u : body_width,
                      "Moving window...",
                      text);
}

static void draw_top_bar(const desktop_state_t* state) {
    char clock_buffer[9];
    u32 width = display_driver_width();
    u32 bar_x = 0u;
    u32 bar_y = 0u;
    u32 bar_width = width;
    u32 bar_height = TOP_BAR_HEIGHT;
    const char* build_label = "JabulOS Insider Build";
    u32 build_width = text_width(build_label);
    bool dark = desktop_dark_mode(state);
    
    u8 r = dark ? 22u : 232u;
    u8 g = dark ? 24u : 236u;
    u8 b = dark ? 34u : 248u;
    u8 alpha = 160u; // glass effect transparency

    u32 text = dark ? vga_color(232, 236, 248) : vga_color(44, 54, 78);
    u8 highlight_alpha = dark ? 20u : 96u;

    rtc_read_time_string(clock_buffer);
    
    // glass effect background
    vga_fill_rect_alpha(bar_x, bar_y, bar_width, bar_height, r, g, b, alpha);
    
    // bar logic
    vga_fill_rect_alpha(bar_x, bar_y, bar_width, 1u, 255, 255, 255, highlight_alpha + 40u);
    vga_fill_rect_alpha(bar_x, bar_y + bar_height - 1u, bar_width, 1u, 0, 0, 0, 48);
    
    draw_text(bar_x + 14u, bar_y + (bar_height / 2u) - 6u, clock_buffer, text);
    draw_text(bar_x + bar_width - build_width - 14u, bar_y + (bar_height / 2u) - 6u, build_label, text);
}

static void draw_preview_label(void) {
}

static const char* desktop_shortcut_label(u32 shortcut_index) {
    switch (shortcut_index) {
        case 0u:
            return "Welcome";
        default:
            return "";
    }
}

static const char* desktop_shortcut_detail(u32 shortcut_index) {
    switch (shortcut_index) {
        case 0u:
            return "";
        default:
            return "";
    }
}

static window_id_t desktop_shortcut_window(u32 shortcut_index) {
    switch (shortcut_index) {
        case 0u:
            return WINDOW_LEGAL;
        default:
            return WINDOW_NONE;
    }
}

static void get_desktop_shortcut_rect(u32 shortcut_index, u32* out_x, u32* out_y, u32* out_width, u32* out_height) {
    if (out_x != NULL) {
        *out_x = 26u + shortcut_index * 104u;
    }
    if (out_y != NULL) {
        *out_y = TOP_BAR_HEIGHT + 36u;
    }
    if (out_width != NULL) {
        *out_width = 92u;
    }
    if (out_height != NULL) {
        *out_height = 104u;
    }
}

static void draw_desktop_shortcut_icon(u32 shortcut_index, u32 page_x, u32 page_y) {
    if (shortcut_index == 0u) {
        draw_rounded_rect(page_x, page_y, 42u, 52u, 12u, vga_color(255, 255, 255));
        draw_rect_outline(page_x, page_y, 42u, 52u, 1u, vga_color(188, 198, 216));
        draw_rect(page_x + 28u, page_y, 14u, 14u, vga_color(228, 234, 248));
        draw_rect(page_x + 8u, page_y + 16u, 22u, 2u, vga_color(126, 144, 188));
        draw_rect(page_x + 8u, page_y + 24u, 20u, 2u, vga_color(126, 144, 188));
        draw_rect(page_x + 8u, page_y + 32u, 18u, 2u, vga_color(126, 144, 188));
        draw_rect(page_x + 8u, page_y + 40u, 14u, 2u, vga_color(126, 144, 188));
    }
}

static void draw_desktop_shortcuts(const desktop_state_t* state) {
    bool dark = desktop_dark_mode(state);
    u32 shortcut_index = 0;

    // Fixed shortcuts
    for (u32 index = 0; index < DESKTOP_SHORTCUT_COUNT; ++index) {
        u32 x;
        u32 y;
        u32 width;
        u32 height;
        u32 page_x;
        u32 page_y;

        get_desktop_shortcut_rect(shortcut_index++, &x, &y, &width, &height);
        page_x = x + 22u;
        page_y = y + 10u;

        draw_rounded_rect(x, y, width, height, 20u, ui_surface_color(state, 1u));
        vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, 16u, 255, 255, 255, dark ? 14u : 42u);
        draw_rounded_rect_outline(x, y, width, height, 20u, 1u, ui_stroke_color(state, 0u));
        draw_rounded_rect(page_x, page_y, 42u, 52u, 12u, ui_surface_color(state, 0u));
        draw_rounded_rect_outline(page_x, page_y, 42u, 52u, 12u, 1u, ui_stroke_color(state, 1u));
        draw_desktop_shortcut_icon(index, page_x, page_y);
        draw_text(x + ((width > text_width(desktop_shortcut_label(index))) ? (width - text_width(desktop_shortcut_label(index))) / 2u : 0u), y + 76u, desktop_shortcut_label(index), ui_text_primary(state));
        draw_text(x + 10u, y + 90u, desktop_shortcut_detail(index), ui_text_muted(state));
    }

    // User files shortcuts
    for (u32 i = 0; i < USER_FILES_MAX; ++i) {
        if (g_user_files.entries[i].active) {
            u32 x;
            u32 y;
            u32 width;
            u32 height;
            u32 page_x;
            u32 page_y;

            get_desktop_shortcut_rect(shortcut_index++, &x, &y, &width, &height);
            page_x = x + 22u;
            page_y = y + 10u;

            draw_rounded_rect(x, y, width, height, 20u, ui_surface_color(state, 1u));
            vga_fill_rect_alpha(x + 1u, y + 1u, width - 2u, 16u, 255, 255, 255, dark ? 14u : 42u);
            draw_rounded_rect_outline(x, y, width, height, 20u, 1u, ui_stroke_color(state, 0u));
            
            // Draw text file icon
            draw_rounded_rect(page_x, page_y, 42u, 52u, 12u, vga_color(255, 255, 255));
            draw_rect_outline(page_x, page_y, 42u, 52u, 1u, vga_color(188, 198, 216));
            draw_rect(page_x + 8u, page_y + 12u, 26u, 2u, vga_color(126, 144, 188));
            draw_rect(page_x + 8u, page_y + 20u, 24u, 2u, vga_color(126, 144, 188));
            draw_rect(page_x + 8u, page_y + 28u, 22u, 2u, vga_color(126, 144, 188));
            draw_rect(page_x + 8u, page_y + 36u, 20u, 2u, vga_color(126, 144, 188));

            draw_text_clipped(x + 4u, y + 76u, width - 8u, g_user_files.entries[i].name, ui_text_primary(state));
            draw_text(x + 10u, y + 90u, "Text File", ui_text_muted(state));
        }
    }
}

static void draw_development_placeholder_window(const desktop_state_t* state,
                                                const window_rect_t* rect,
                                                bool shaded,
                                                window_id_t window,
                                                const char* title,
                                                u32 accent,
                                                const char* line_one,
                                                const char* line_two) {
    u32 frame_height;
    u32 panel_x;
    u32 panel_y;
    u32 panel_width;
    u32 panel_height;
    u32 inner_x;
    u32 inner_y;
    u32 inner_width;
    u32 inner_height;

    if (rect == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(window) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, title, accent);
    if (shaded) {
        return;
    }

    panel_x = rect->x + 16u;
    panel_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 16u;
    panel_width = rect->width - 32u;
    panel_height = rect->height - WINDOW_TITLEBAR_HEIGHT - 32u;

    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));
    inner_x = panel_x + 16u;
    inner_y = panel_y + 16u;
    inner_width = panel_width - 32u;
    inner_height = panel_height - 32u;
    draw_rounded_panel(inner_x, inner_y, inner_width, inner_height, 14u,
                       ui_surface_color(state, 1u), ui_stroke_color(state, 1u));

    draw_text_centered_clipped(inner_x,
                               inner_y + ((inner_height > 32u) ? (inner_height / 2u - 14u) : 12u),
                               inner_width,
                               line_one,
                               ui_text_secondary(state));
    draw_text_centered_clipped(inner_x,
                               inner_y + ((inner_height > 32u) ? (inner_height / 2u + 8u) : 34u),
                               inner_width,
                               line_two,
                               ui_text_secondary(state));
}

static void draw_legal_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;
    u32 panel_x;
    u32 panel_y;
    u32 panel_width;
    u32 panel_height;
    u32 content_x;
    u32 content_y;
    u32 content_width;

    if (rect == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_LEGAL) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Welcome", vga_color(120, 156, 244));
    if (shaded) {
        return;
    }

    panel_x = rect->x + 16u;
    panel_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 16u;
    panel_width = safe_subtract(rect->width, 32u);
    panel_height = safe_subtract(rect->height, WINDOW_TITLEBAR_HEIGHT + 32u);

    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));
    content_x = panel_x + 16u;
    content_y = panel_y + 18u;
    content_width = safe_subtract(panel_width, 32u);
    draw_text_clipped(content_x, content_y, content_width, "Welcome to jabulOS Aero, By using this operating system you must agree (still working)", ui_text_secondary(state));
    draw_text_clipped(content_x, content_y + 28u, content_width, ".", ui_text_secondary(state));
}

static bool launcher_result_at(const desktop_state_t* state,
                               const window_rect_t* rect,
                               u32 mouse_x,
                               u32 mouse_y,
                               u32* out_index) {
    bool searching;
    u32 visible_row = 0;
    u32 visible_tile = 0;

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

            if (g_launcher_window_ids[index] == WINDOW_JABVER) {
                continue;
            }
            launcher_app_tile_rect(rect, visible_tile, &tile_x, &tile_y, &tile_width, &tile_height);
            if (point_in_rect(mouse_x, mouse_y, tile_x, tile_y, tile_width, tile_height)) {
                if (out_index != NULL) {
                    *out_index = index;
                }
                return true;
            }
            ++visible_tile;
        }

        for (u32 index = 0; index < LAUNCHER_RECOMMENDED_COUNT; ++index) {
            u32 tile_x;
            u32 tile_y;
            u32 tile_width;
            u32 tile_height;
            u32 app_index = g_launcher_recommended_indices[index];

            launcher_recommended_tile_rect(rect, index, &tile_x, &tile_y, &tile_width, &tile_height);
            if (point_in_rect(mouse_x, mouse_y, tile_x, tile_y, tile_width, tile_height)) {
                if (out_index != NULL) {
                    *out_index = app_index;
                }
                return true;
            }
        }

        return false;
    }

    for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
        u32 row_x;
        u32 row_y;
        u32 row_width;
        u32 row_height;

        if (g_launcher_window_ids[index] == WINDOW_JABVER) {
            continue;
        }
        if (!string_contains_case_insensitive(g_launcher_app_names[index], state->launcher_query)) {
            continue;
        }

        launcher_row_rect(rect, visible_row, &row_x, &row_y, &row_width, &row_height);
        if (point_in_rect(mouse_x, mouse_y, row_x, row_y, row_width, row_height)) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return true;
        }
        ++visible_row;
    }

    return false;
}

static void read_icon_image_pixel(const image_t* image, u32 source_x, u32 source_y, u8* out_red, u8* out_green, u8* out_blue) {
    u32 bytes_per_pixel;
    const u8* pixel;
    u8 red;
    u8 green;
    u8 blue;

    if (image == NULL || out_red == NULL || out_green == NULL || out_blue == NULL ||
        image->pixels == NULL || image->width == 0u || image->height == 0u || image->bpp < 24u) {
        if (out_red != NULL) {
            *out_red = 0u;
        }
        if (out_green != NULL) {
            *out_green = 0u;
        }
        if (out_blue != NULL) {
            *out_blue = 0u;
        }
        return;
    }

    if (source_x >= image->width) {
        source_x = image->width - 1u;
    }
    if (source_y >= image->height) {
        source_y = image->height - 1u;
    }
    if (image->bottom_up) {
        source_y = image->height - 1u - source_y;
    }

    bytes_per_pixel = image->bpp / 8u;
    pixel = image->pixels + source_y * image->pitch + source_x * bytes_per_pixel;
    blue = pixel[0];
    green = pixel[1];
    red = pixel[2];

    if (!image->bgr) {
        red = pixel[0];
        green = pixel[1];
        blue = pixel[2];
    }

    *out_red = red;
    *out_green = green;
    *out_blue = blue;
}

static u32 channel_delta(u8 left, u8 right) {
    return left > right ? (u32)(left - right) : (u32)(right - left);
}

static bool icon_pixel_matches_background(u8 red, u8 green, u8 blue, const u8 background[4][3]) {
    u8 darkest = red;
    u8 brightest = red;
    u32 chroma_span;

    if (green < darkest) {
        darkest = green;
    }
    if (blue < darkest) {
        darkest = blue;
    }
    if (green > brightest) {
        brightest = green;
    }
    if (blue > brightest) {
        brightest = blue;
    }
    chroma_span = (u32)(brightest - darkest);

    for (u32 index = 0u; index < 4u; ++index) {
        u32 delta = channel_delta(red, background[index][0]) +
                    channel_delta(green, background[index][1]) +
                    channel_delta(blue, background[index][2]);
        if (delta <= 84u) {
            return true;
        }
    }

    if (brightest <= 124u && chroma_span <= 20u) {
        return true;
    }

    return false;
}

static void icon_content_bounds(const image_t* image,
                                const u8 background[4][3],
                                u32* out_left,
                                u32* out_top,
                                u32* out_right,
                                u32* out_bottom) {
    u32 min_x;
    u32 min_y;
    u32 max_x;
    u32 max_y;
    bool found = false;

    if (out_left == NULL || out_top == NULL || out_right == NULL || out_bottom == NULL) {
        return;
    }
    if (image == NULL || image->pixels == NULL || image->width == 0u || image->height == 0u) {
        *out_left = 0u;
        *out_top = 0u;
        *out_right = 0u;
        *out_bottom = 0u;
        return;
    }

    min_x = image->width - 1u;
    min_y = image->height - 1u;
    max_x = 0u;
    max_y = 0u;

    for (u32 source_y = 0u; source_y < image->height; ++source_y) {
        for (u32 source_x = 0u; source_x < image->width; ++source_x) {
            u8 red;
            u8 green;
            u8 blue;

            read_icon_image_pixel(image, source_x, source_y, &red, &green, &blue);
            if (icon_pixel_matches_background(red, green, blue, background)) {
                continue;
            }

            if (!found || source_x < min_x) {
                min_x = source_x;
            }
            if (!found || source_y < min_y) {
                min_y = source_y;
            }
            if (!found || source_x > max_x) {
                max_x = source_x;
            }
            if (!found || source_y > max_y) {
                max_y = source_y;
            }
            found = true;
        }
    }

    if (!found) {
        *out_left = 0u;
        *out_top = 0u;
        *out_right = image->width - 1u;
        *out_bottom = image->height - 1u;
        return;
    }

    *out_left = min_x;
    *out_top = min_y;
    *out_right = max_x;
    *out_bottom = max_y;
}

static void draw_loaded_app_icon(u32 x, u32 y, u32 size, const image_t* image, bool trim_edges) {
    u32 source_left;
    u32 source_top;
    u32 source_right;
    u32 source_bottom;
    u32 source_width;
    u32 source_height;
    u32 available_size;
    u32 draw_width;
    u32 draw_height;
    u32 draw_x;
    u32 draw_y;
    u8 background[4][3];

    if (image == NULL || image->pixels == NULL || image->width == 0u || image->height == 0u || size == 0u) {
        return;
    }

    read_icon_image_pixel(image, 0u, 0u, &background[0][0], &background[0][1], &background[0][2]);
    read_icon_image_pixel(image, image->width - 1u, 0u, &background[1][0], &background[1][1], &background[1][2]);
    read_icon_image_pixel(image, 0u, image->height - 1u, &background[2][0], &background[2][1], &background[2][2]);
    read_icon_image_pixel(image, image->width - 1u, image->height - 1u, &background[3][0], &background[3][1], &background[3][2]);

    icon_content_bounds(image, background, &source_left, &source_top, &source_right, &source_bottom);
    source_width = source_right - source_left + 1u;
    source_height = source_bottom - source_top + 1u;

    if (trim_edges && source_width > 12u && source_height > 12u) {
        u32 trim_x = source_width / 16u;
        u32 trim_y = source_height / 16u;
        if (trim_x < 1u) {
            trim_x = 1u;
        }
        if (trim_y < 1u) {
            trim_y = 1u;
        }
        if (trim_x > 4u) {
            trim_x = 4u;
        }
        if (trim_y > 4u) {
            trim_y = 4u;
        }
        if (source_left + trim_x < source_right) {
            source_left += trim_x;
            source_right -= trim_x;
        }
        if (source_top + trim_y < source_bottom) {
            source_top += trim_y;
            source_bottom -= trim_y;
        }
        source_width = source_right - source_left + 1u;
        source_height = source_bottom - source_top + 1u;
    }

    available_size = size;
    draw_width = available_size;
    draw_height = (available_size * source_height) / source_width;
    if (draw_height > available_size) {
        draw_height = available_size;
        draw_width = (available_size * source_width) / source_height;
    }
    if (draw_width == 0u) {
        draw_width = 1u;
    }
    if (draw_height == 0u) {
        draw_height = 1u;
    }

    draw_x = x + ((size > draw_width) ? (size - draw_width) / 2u : 0u);
    draw_y = y + ((size > draw_height) ? (size - draw_height) / 2u : 0u);

    for (u32 dst_y = 0u; dst_y < draw_height; ++dst_y) {
        u32 source_y = source_top + (dst_y * source_height) / draw_height;

        for (u32 dst_x = 0u; dst_x < draw_width; ++dst_x) {
            u32 source_x = source_left + (dst_x * source_width) / draw_width;
            u8 red;
            u8 green;
            u8 blue;

            read_icon_image_pixel(image, source_x, source_y, &red, &green, &blue);
            if (icon_pixel_matches_background(red, green, blue, background)) {
                continue;
            }
            put_pixel(draw_x + dst_x, draw_y + dst_y, vga_color(red, green, blue));
        }
    }
}

static void draw_app_icon(u32 x, u32 y, u32 size, dock_app_t app) {
    u32 ink = vga_color(36, 44, 62);
    u32 dark = vga_color(232, 238, 246);
    u32 blue = vga_color(98, 168, 255);
    u32 gold = vga_color(255, 196, 92);
    u32 mint = vga_color(104, 224, 172);
    u32 purple = vga_color(198, 156, 255);

    if (app < DOCK_APP_MAX &&
        g_app_icon_loaded[app] &&
        app != APP_TERMINAL) {
        draw_loaded_app_icon(x, y, size, &g_app_icon_images[app], app != APP_SETTINGS);
        return;
    }

    if (app == APP_START) {
        draw_rect(x + 8, y + 8, 8, 8, blue);
        draw_rect(x + 18, y + 8, 8, 8, mint);
        draw_rect(x + 8, y + 18, 8, 8, gold);
        draw_rect(x + 18, y + 18, 8, 8, purple);
    } else if (app == APP_SETTINGS) {
        draw_rounded_rect(x + 8, y + 8, 18, 18, 9, dark);
        draw_rect(x + 5, y + 14, 5, 5, blue);
        draw_rect(x + 24, y + 14, 5, 5, blue);
        draw_rect(x + 14, y + 5, 5, 5, blue);
        draw_rect(x + 14, y + 24, 5, 5, blue);
        draw_rect(x + 13, y + 13, 8, 8, ink);
    } else if (app == APP_TERMINAL) {
        draw_rounded_rect(x + 4, y + 7, 24, 18, 6, dark);
        draw_rect(x + 8, y + 11, 5, 2, ink);
        draw_rect(x + 11, y + 13, 5, 2, ink);
        draw_rect(x + 10, y + 19, 13, 2, ink);
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
    } else if (app == APP_BROWSER) {
        draw_rounded_rect(x + 6u, y + 8u, 22u, 18u, 8u, vga_color(246, 132, 88));
        draw_rect(x + 9u, y + 12u, 16u, 2u, vga_color(255, 236, 226));
        draw_rect(x + 11u, y + 17u, 12u, 2u, vga_color(255, 255, 255));
        draw_rect(x + 13u, y + 21u, 8u, 2u, vga_color(255, 255, 255));
    } else if (app == APP_VIDEOS) {
        draw_rounded_rect(x + 6u, y + 10u, 20u, 14u, 6u, blue);
        draw_rect(x + 14u, y + 13u, 3u, 8u, vga_color(255, 255, 255));
        draw_rect(x + 18u, y + 14u, 2u, 6u, vga_color(255, 255, 255));
        draw_rect(x + 22u, y + 15u, 2u, 4u, vga_color(255, 255, 255));
        draw_rect(x + 10u, y + 24u, 12u, 2u, dark);
    } else if (app == APP_SNAKE) {
        draw_rounded_rect(x + 7u, y + 8u, 18u, 18u, 9u, mint);
        draw_rounded_rect(x + 18u, y + 12u, 8u, 8u, 4u, vga_color(56, 174, 102));
        draw_rect(x + 12u, y + 13u, 2u, 2u, ink);
        draw_rect(x + 24u, y + 15u, 2u, 2u, vga_color(244, 110, 110));
    } else if (app == APP_JABVER) {
        draw_rounded_rect(x + 6u, y + 6u, 20u, 20u, 6u, blue);
        draw_text(x + 11u, y + 11u, "J", vga_color(255, 255, 255));
        draw_rect(x + 9u, y + 26u, 14u, 2u, dark);
    }
}

static void dock_tile_rect(const dock_layout_t* layout,
                           u32 index,
                           u32 total_count,
                           u32* out_x,
                           u32* out_y,
                           u32* out_width,
                           u32* out_height) {
    u32 tile_x;
    u32 tile_y = 0u;
    u32 tile_width;
    u32 tile_height = 40u;
    bool is_start = (index == 0u);

    if (layout == NULL) {
        return;
    }

    if (layout->style == TASKBAR_STYLE_TENUI) {
        tile_width = is_start ? 64u : 40u;
        tile_height = 40u;
        tile_y = layout->dock_y + 4u;
        if (is_start) {
            tile_x = layout->dock_x + 8u;
        } else {
            tile_x = layout->dock_x + 84u + (index - 1u) * 44u;
        }
    } else {
        tile_width = is_start ? 56u : 40u;
        tile_y = layout->dock_y + 11u;
        if (is_start) {
            tile_x = layout->dock_x + 18u;
        } else {
            const u32 icon_area_x = layout->dock_x + 226u;
            const u32 icon_area_width = safe_subtract(layout->dock_width, 346u);
            const u32 icon_gap = 12u;
            const u32 icon_count = total_count > 0u ? total_count - 1u : 0u;
            const u32 icon_row_width = icon_count * 40u + (icon_count > 0u ? (icon_count - 1u) * icon_gap : 0u);
            const u32 row_x = icon_area_x + ((icon_area_width > icon_row_width) ? (icon_area_width - icon_row_width) / 2u : 0u);
            const u32 icon_row_left_shift = 16u;

            tile_x = (row_x > icon_row_left_shift ? row_x - icon_row_left_shift : row_x) +
                     (index - 1u) * (40u + icon_gap);
        }
    }

    if (out_x != NULL) {
        *out_x = tile_x;
    }
    if (out_y != NULL) {
        *out_y = tile_y;
    }
    if (out_width != NULL) {
        *out_width = tile_width;
    }
    if (out_height != NULL) {
        *out_height = tile_height;
    }
}

static void get_dock_layout(const desktop_state_t* state, dock_layout_t* out_layout) {
    if (out_layout == NULL) {
        return;
    }

    out_layout->style = (state != NULL) ? state->taskbar_style : (u32)TASKBAR_STYLE_MACXWIN;
    if (out_layout->style == TASKBAR_STYLE_TENUI) {
        out_layout->dock_width = display_driver_width();
        out_layout->dock_height = 48u;
        out_layout->icon_size = 28u;
        out_layout->dock_x = 0u;
        out_layout->dock_y = display_driver_height() > out_layout->dock_height ?
                             (display_driver_height() - out_layout->dock_height) : 0u;
        return;
    }

    u32 app_count = get_visible_dock_apps(state, NULL);
    u32 icons_count = app_count > 0u ? app_count - 1u : 0u;
    u32 icons_width = icons_count * 40u + (icons_count > 0u ? (icons_count - 1u) * 12u : 0u);
    
    out_layout->dock_width = 226u + icons_width + 120u;
    if (out_layout->dock_width < 600u) {
        out_layout->dock_width = 600u;
    }
    
    out_layout->dock_height = 62u;
    out_layout->icon_size = 30u;
    out_layout->dock_x = (display_driver_width() > out_layout->dock_width) ? (display_driver_width() - out_layout->dock_width) / 2u : 0u;
    out_layout->dock_y = (display_driver_height() > out_layout->dock_height + 20u) ?
                         (display_driver_height() - out_layout->dock_height - 20u) : 0u;
}

static u32 window_title_height(window_id_t window) {
    (void)window;
    return WINDOW_TITLEBAR_HEIGHT;
}

static void position_run_dialog(desktop_state_t* state) {
    u32 run_width = 320u;
    u32 run_height = 146u;
    u32 bottom_margin = 92u;

    if (state == NULL) {
        return;
    }

    state->run_rect.width = run_width;
    state->run_rect.height = run_height;
    state->run_rect.x = 24u;
    state->run_rect.y = display_driver_height() > run_height + bottom_margin ?
                        (display_driver_height() - run_height - bottom_margin) : 48u;
    clamp_window_rect(&state->run_rect);
}

static void position_search_panel(desktop_state_t* state) {
    dock_layout_t layout;
    const u32 panel_width = 560u;
    const u32 panel_height = 560u;

    if (state == NULL) {
        return;
    }

    get_dock_layout(state, &layout);
    state->search_rect.width = panel_width;
    state->search_rect.height = panel_height;
    if (layout.style == TASKBAR_STYLE_TENUI) {
        state->search_rect.x = 12u;
    } else {
        state->search_rect.x = (display_driver_width() > panel_width) ? (display_driver_width() - panel_width) / 2u : 12u;
    }
    state->search_rect.y = layout.dock_y > panel_height + 16u ? (layout.dock_y - panel_height - 16u) : 12u;
}

static bool point_in_window_title(const desktop_state_t* state, u32 x, u32 y, const window_rect_t* rect, window_id_t window) {
    if (window == WINDOW_SEARCH) {
        return false;
    }
    if (rect == NULL ||
        x < rect->x || x >= rect->x + rect->width ||
        y < rect->y || y >= rect->y + window_title_height(window)) {
        return false;
    }

    if (point_in_window_button_hitbox(state, rect, WINDOW_BUTTON_CLOSE, x, y) ||
        point_in_window_button_hitbox(state, rect, WINDOW_BUTTON_FULLSCREEN, x, y)) {
        return false;
    }

    return true;
}

static void clamp_window_rect(window_rect_t* rect) {
    u32 min_x = 12u;
    u32 min_y = 40u;
    u32 max_x;
    u32 max_y;

    if (rect == NULL) {
        return;
    }

    max_x = (display_driver_width() > rect->width + 12u) ? (display_driver_width() - rect->width - 12u) : min_x;
    max_y = (display_driver_height() > rect->height + 88u) ? (display_driver_height() - rect->height - 88u) : min_y;

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
        case WINDOW_RUN:
            return &state->run_rect;
        case WINDOW_SETTINGS:
            return &state->settings_rect;
        case WINDOW_TERMINAL:
            return &state->terminal_rect;
        case WINDOW_NOTEPAD:
            return &state->notepad_rect;
        case WINDOW_FILES:
            return &state->files_rect;
        case WINDOW_TASKS:
            return &state->tasks_rect;
        case WINDOW_VIDEOS:
            return &state->videos_rect;
        case WINDOW_BROWSER:
            return &state->browser_rect;
        case WINDOW_GAMES:
            return &state->games_rect;
        case WINDOW_SNAKE:
            return &state->snake_rect;
        case WINDOW_JABVER:
            return &state->jabver_rect;
        case WINDOW_WELCOME:
            return &state->welcome_rect;
        case WINDOW_LEGAL:
            return &state->legal_rect;
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

static bool window_contains_point(const desktop_state_t* state, window_id_t window, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 height;

    if (state == NULL || !window_visible(state, window)) {
        return false;
    }

    rect = get_window_rect_const(state, window);
    if (rect == NULL) {
        return false;
    }

    height = window_shaded(state, window) ? shaded_window_height(window) : rect->height;
    return point_in_rect(mouse_x, mouse_y, rect->x, rect->y, rect->width, height);
}

static window_id_t find_top_window_at_point(const desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    if (state == NULL) {
        return WINDOW_NONE;
    }

    for (u32 index = WINDOW_STACK_SIZE; index > 0u; --index) {
        window_id_t window = state->window_order[index - 1u];

        if (window_contains_point(state, window, mouse_x, mouse_y)) {
            return window;
        }
    }

    return WINDOW_NONE;
}

static bool get_top_visible_window_composite_rect(const desktop_state_t* state, window_rect_t* out_rect) {
    if (state == NULL || out_rect == NULL) {
        return false;
    }

    for (u32 index = WINDOW_STACK_SIZE; index > 0u; --index) {
        window_id_t window = state->window_order[index - 1u];

        if (!window_visible(state, window)) {
            continue;
        }
        if (get_window_composite_rect(state, window, out_rect)) {
            return true;
        }
    }

    return false;
}

static bool dock_icon_contains(const desktop_state_t* state, u32 mouse_x, u32 mouse_y, u32 index, u32 total_count) {
    dock_layout_t layout;
    u32 tile_x;
    u32 tile_y;
    u32 tile_width;
    u32 tile_height;

    get_dock_layout(state, &layout);
    dock_tile_rect(&layout, index, total_count, &tile_x, &tile_y, &tile_width, &tile_height);

    return mouse_x >= tile_x && mouse_x < tile_x + tile_width &&
           mouse_y >= tile_y && mouse_y < tile_y + tile_height;
}

static bool dock_hit_test(const desktop_state_t* state, u32 mouse_x, u32 mouse_y, dock_app_t* out_app) {
    dock_app_t visible_apps[DOCK_APP_MAX];
    u32 count = get_visible_dock_apps(state, visible_apps);

    for (u32 index = 0; index < count; ++index) {
        if (dock_icon_contains(state, mouse_x, mouse_y, index, count)) {
            if (out_app != NULL) {
                *out_app = visible_apps[index];
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

static bool move_dragged_window_to_mouse(desktop_state_t* state, const mouse_state_t* mouse) {
    window_rect_t* rect;
    s32 next_x;
    s32 next_y;

    if (state == NULL || mouse == NULL || state->dragging_window == WINDOW_NONE) {
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

static void desktop_initialize(desktop_state_t* state,
                               const boot_info_t* boot_info,
                               const image_t* boot_splash_image,
                               const image_t* splash_logo_image,
                               bool ata_ready,
                               bool live_media_boot,
                               u32 current_second) {
    install_state_sector_t install_state;

    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->boot_splash_image = boot_splash_image;
    state->splash_logo_image = splash_logo_image;
    state->live_media_boot = live_media_boot;
    state->desktop_started_second = current_second;
    state->oobe_storage_ready = ata_ready;
    if (ata_ready) {
        (void)ata_read_primary_master_info(&state->storage_info);
    }
    state->oobe_disk_has_install = ata_ready && installer_header_present();
    state->oobe_stage = ata_ready ? OOBE_STAGE_INTRO : OOBE_STAGE_STORAGE_MISSING;
    state->desktop_theme = DESKTOP_THEME_JABULXP;
    state->window_theme = WINDOW_THEME_WINDOWS;
    state->transparency_enabled = false;
    state->taskbar_style = TASKBAR_STYLE_MACXWIN;
    state->wallpaper_theme = WALLPAPER_THEME_ALANBLISS;
    state->settings_page = SETTINGS_PAGE_SYSTEM_SPECS;
    state->task_manager_page = TASK_MANAGER_PAGE_CPU;
    set_setup_phase(state,
                    (live_media_boot && !state->oobe_disk_has_install) ? SETUP_PHASE_WELCOME : SETUP_PHASE_NONE,
                    current_second);
    state->auth_field_focus = AUTH_FIELD_USERNAME;
    state->show_search = false;
    state->show_run = false;
    state->show_settings = false;
    state->show_notepad = false;
    state->show_files = false;
    state->show_tasks = false;
    state->show_videos = false;
    state->show_browser = false;
    state->show_games = false;
    state->show_snake = false;
    state->show_jabver = false;
    state->show_welcome = false;
    state->show_legal = false;
    state->search_rect = {168u, 96u, 584u, 424u};
    state->run_rect = {24u, 0u, 320u, 146u};
    state->settings_rect = {148u, 72u, 728u, 446u};
    state->notepad_rect = {200u, 100u, 640u, 360u};
    state->files_rect = {188u, 88u, 620u, 452u};
    state->tasks_rect = {160u, 92u, 704u, 540u};
    state->videos_rect = {168u, 104u, 664u, 424u};
    state->browser_rect = {122u, 92u, 664u, 390u};
    state->games_rect = {232u, 154u, 454u, 336u};
    state->snake_rect = {256u, 128u, 454u, 336u};
    state->jabver_rect = {232u, 96u, 560u, 420u};
    state->welcome_rect = {324u, 82u, 392u, 286u};
    state->legal_rect = {88u, 74u, 544u, 424u};
    position_run_dialog(state);
    initialize_window_order(state);
    open_window(state, WINDOW_LEGAL);
    state->dragging_window = WINDOW_NONE;
    state->launcher_query[0] = '\0';
    state->run_query[0] = '\0';
    state->install_target_index = 0u;
    state->browser_page = BROWSER_PAGE_WEB;
    browser_reset_runtime();
    browser_reset_all_scroll();
    state->games_screen = GAMES_SCREEN_HOME;
    state->notepad_user_file_index = -1;
    state->context_menu_open = false;
    state->context_menu_renaming = false;
    state->context_menu_clipboard_entry = -1;
    snake_reset_state(state);
    ttt_reset(state);
    cpu_query_identity(state);
    cpu_run_benchmark(state);
    notepad_load_from_disk(state);
    user_files_load_table(state);

    state->videos_item_count = 0u;
    state->videos_selected_item = 0u;
    state->videos_playing = false;
    state->videos_play_start_tsc = 0u;
    state->videos_last_frame_index = 0u;
    state->videos_audio_data = NULL;
    state->videos_audio_sample_count = 0u;
    state->videos_audio_sample_rate = 0u;
    state->videos_audio_last_sample_index = 0xFFFFFFFFu;
    state->videos_sb16_available = sb16_detect();
    state->videos_audio_use_sb16 = false;
    startup_sound_reset(state);
    (void)startup_sound_load(state, boot_info);
    videos_refresh_media_list(state, boot_info);
    file_manager_initialize(state, boot_info);

    if (boot_info != NULL && boot_info->initrd_start != 0 && boot_info->initrd_size != 0) {
        initrd_file_t pe_file;
        if (initrd_find_file((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, "pe-lab.exe", &pe_file)) {
            state->pe_lab_available = true;
            (void)pe_parse_image(pe_file.data, pe_file.size, &state->pe_lab_image);
        }
    }

    if (ata_ready && find_pending_install_state(&state->storage_info, &install_state)) {
        state->install_state_stage = install_state.stage;
        state->install_state_target_lba = install_state.target_lba;
        state->install_state_target_sectors = install_state.target_sector_count;
        set_setup_phase(state, SETUP_PHASE_INSTALLING, current_second);
        state->oobe_install_complete = false;
        state->oobe_install_failed = false;
        state->oobe_install_write_started = false;
        return;
    }

    if (ata_ready && load_existing_install_state(&install_state)) {
        state->install_state_stage = install_state.stage;
        state->install_state_target_lba = install_state.target_lba;
        state->install_state_target_sectors = install_state.target_sector_count;
        strcpy(state->account_username, install_state.username);
        strcpy(state->account_password, install_state.password);
        if (install_state.stage == INSTALL_STATE_STAGE_NEEDS_OOBE) {
            set_setup_phase(state, SETUP_PHASE_USER_OOBE, current_second);
            state->auth_field_focus = AUTH_FIELD_USERNAME;
        } else if (install_state.stage == INSTALL_STATE_STAGE_READY) {
            set_setup_phase(state, SETUP_PHASE_LOCKSCREEN, current_second);
            state->auth_field_focus = AUTH_FIELD_PASSWORD;
        }
    }
}

static bool toggle_dock_app(desktop_state_t* state, dock_app_t app) {
    window_id_t window;

    if (state == NULL) {
        return false;
    }

    state->show_welcome = false;
    if (app == APP_START) {
        state->launcher_power_menu_open = false;
        close_window(state, WINDOW_RUN);
    }
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

static bool begin_oobe_account_creation(desktop_state_t* state, u32 current_second) {
    if (state == NULL ||
        state->account_input_username[0] == '\0' ||
        state->account_input_password[0] == '\0') {
        if (state != NULL) {
            state->oobe_install_failed = true;
        }
        return false;
    }

    if (!persist_state_from_desktop(state,
                                    INSTALL_STATE_STAGE_READY,
                                    state->account_input_username,
                                    state->account_input_password)) {
        state->oobe_install_failed = true;
        return false;
    }

    strcpy(state->account_username, state->account_input_username);
    strcpy(state->account_password, state->account_input_password);
    state->oobe_install_failed = false;
    state->install_state_stage = INSTALL_STATE_STAGE_READY;
    set_setup_phase(state, SETUP_PHASE_OOBE_HI, current_second);
    return true;
}

static bool try_unlock_lockscreen(desktop_state_t* state) {
    if (state == NULL || state->setup_phase != SETUP_PHASE_LOCKSCREEN) {
        return false;
    }

    if (strcmp(state->login_password_input, state->account_password) != 0) {
        state->login_error = true;
        state->login_password_input[0] = '\0';
        return true;
    }

    state->login_error = false;
    state->login_password_input[0] = '\0';
    set_setup_phase(state, SETUP_PHASE_NONE, rtc_read_seconds_of_day());
    return true;
}

static bool handle_setup_keyboard_scancode(desktop_state_t* state, u8 scancode) {
    char input;
    char* active_buffer;
    u32 active_capacity;

    if (state == NULL || !setup_active(state) || (scancode & 0x80u) != 0u) {
        return false;
    }

    if (state->setup_phase == SETUP_PHASE_USER_OOBE) {
        if (scancode == 0x0F) {
            state->auth_field_focus = state->auth_field_focus == AUTH_FIELD_USERNAME ? AUTH_FIELD_PASSWORD : AUTH_FIELD_USERNAME;
            return true;
        }
        if (scancode == 0x1C) {
            if (state->auth_field_focus == AUTH_FIELD_USERNAME && state->account_input_username[0] != '\0') {
                state->auth_field_focus = AUTH_FIELD_PASSWORD;
                return true;
            }
            return begin_oobe_account_creation(state, rtc_read_seconds_of_day());
        }

        active_buffer = state->auth_field_focus == AUTH_FIELD_USERNAME ? state->account_input_username : state->account_input_password;
        active_capacity = state->auth_field_focus == AUTH_FIELD_USERNAME ? sizeof(state->account_input_username)
                                                                        : sizeof(state->account_input_password);
        if (scancode == 0x0E) {
            state->oobe_install_failed = false;
            return erase_query_character(active_buffer);
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            state->oobe_install_failed = false;
            return append_query_character(active_buffer, active_capacity, input);
        }
        return false;
    }

    if (state->setup_phase == SETUP_PHASE_LOCKSCREEN) {
        if (!state->lockscreen_prompt_visible) {
            input = scancode_to_ascii(scancode);
            if (scancode == 0x1C || scancode == 0x0E || input != '\0') {
                state->lockscreen_prompt_visible = true;
                state->lockscreen_prompt_started_ms = timer_ticks_ms();
                state->login_error = false;
                if (input != '\0') {
                    return append_query_character(state->login_password_input, sizeof(state->login_password_input), input);
                }
                return true;
            }
            return false;
        }
        if (scancode == 0x1C) {
            return try_unlock_lockscreen(state);
        }
        if (scancode == 0x0E) {
            state->login_error = false;
            return erase_query_character(state->login_password_input);
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            state->login_error = false;
            return append_query_character(state->login_password_input, sizeof(state->login_password_input), input);
        }
        return false;
    }

    return state->setup_phase == SETUP_PHASE_INSTALLING ||
           state->setup_phase == SETUP_PHASE_OOBE_HI ||
           state->setup_phase == SETUP_PHASE_OOBE_SETTING_UP ||
           state->setup_phase == SETUP_PHASE_OOBE_PREPARE ||
           state->setup_phase == SETUP_PHASE_OOBE_ALMOST_THERE;
}

static bool handle_keyboard_scancode(desktop_state_t* state, u8 scancode) {
    char input;
    u32 launcher_index;

    if (state->context_menu_renaming) {
        if (scancode == 0x01) { // Esc
            state->context_menu_renaming = false;
            return true;
        }
        if (scancode == 0x1C) { // Enter
            // Finalize rename
            if (state->context_menu_target_entry > 0) {
                u32 user_file_index = (u32)state->context_menu_target_entry - 1;
                u32 active_count = 0;
                for (u32 i = 0; i < USER_FILES_MAX; ++i) {
                    if (g_user_files.entries[i].active) {
                        if (active_count == user_file_index) {
                            strcpy(g_user_files.entries[i].name, state->context_menu_rename_buffer);
                            user_files_save_table(state);
                            file_manager_refresh_entries(state, NULL); // NULL boot_info should be okay if already initialized
                            break;
                        }
                        active_count++;
                    }
                }
            }
            state->context_menu_renaming = false;
            return true;
        }
        if (scancode == 0x0E) { // Backspace
            u32 len = (u32)strlen(state->context_menu_rename_buffer);
            if (len > 0) {
                state->context_menu_rename_buffer[len - 1] = '\0';
                return true;
            }
            return false;
        }
        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            u32 len = (u32)strlen(state->context_menu_rename_buffer);
            if (len + 1 < sizeof(state->context_menu_rename_buffer)) {
                state->context_menu_rename_buffer[len] = input;
                state->context_menu_rename_buffer[len + 1] = '\0';
                return true;
            }
        }
        return false;
    }

    if ((scancode & 0x80u) != 0 || state == NULL) {
        return false;
    }

    if (state->power_action != POWER_ACTION_NONE) {
        return false;
    }

    if (handle_setup_keyboard_scancode(state, scancode)) {
        return true;
    }
    if (setup_active(state)) {
        return false;
    }

    if ((window_visible(state, WINDOW_SNAKE) &&
         !window_shaded(state, WINDOW_SNAKE)) ||
        (window_visible(state, WINDOW_GAMES) &&
         !window_shaded(state, WINDOW_GAMES) &&
         state->games_screen == GAMES_SCREEN_SNAKE)) {
        switch (scancode) {
            case 0x48:
            case 0x11:
                return snake_move(state, 0, -1);
            case 0x50:
            case 0x1F:
                return snake_move(state, 0, 1);
            case 0x4B:
            case 0x1E:
                return snake_move(state, -1, 0);
            case 0x4D:
            case 0x20:
                return snake_move(state, 1, 0);
            case 0x39:
            case 0x1C:
                snake_reset_state(state);
                return true;
            default:
                break;
        }
    }

    if ((state->ctrl_down || state->left_super_down || state->right_super_down) && scancode == 0x13) {
        state->show_welcome = false;
        close_window(state, WINDOW_SEARCH);
        position_run_dialog(state);
        open_window(state, WINDOW_RUN);
        state->run_query[0] = '\0';
        return true;
    }

    if (window_open(state, WINDOW_RUN) && !window_minimized(state, WINDOW_RUN)) {
        if (scancode == 0x01) {
            close_window(state, WINDOW_RUN);
            state->run_query[0] = '\0';
            return true;
        }

        if (scancode == 0x0E) {
            return erase_query_character(state->run_query);
        }

        if (scancode == 0x1C) {
            return activate_run_dialog(state);
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            return append_query_character(state->run_query, sizeof(state->run_query), input);
        }
    }

    if (window_open(state, WINDOW_SEARCH) && !window_minimized(state, WINDOW_SEARCH)) {
        if (scancode == 0x01) {
            if (state->launcher_power_menu_open) {
                state->launcher_power_menu_open = false;
                return true;
            }
            close_window(state, WINDOW_SEARCH);
            state->show_welcome = false;
            return true;
        }

        if (scancode == 0x0E) {
            state->launcher_power_menu_open = false;
            return erase_query_character(state->launcher_query);
        }

        if (scancode == 0x1C) {
            if (find_first_launcher_match(state->launcher_query, &launcher_index)) {
                open_window(state, g_launcher_window_ids[launcher_index]);
                close_window(state, WINDOW_SEARCH);
                state->show_welcome = false;
                state->launcher_query[0] = '\0';
                return true;
            }
            return false;
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            state->launcher_power_menu_open = false;
            return append_query_character(state->launcher_query, sizeof(state->launcher_query), input);
        }
    }

    if (browser_is_active_window(state)) {
        if (scancode == 0x49) {
            browser_scroll_page(state->browser_page, -8);
            return true;
        }
        if (scancode == 0x51) {
            browser_scroll_page(state->browser_page, 8);
            return true;
        }
        if (scancode == 0x47) {
            browser_reset_scroll(state->browser_page);
            return true;
        }
        if (scancode == 0x0E) {
            return erase_query_character(g_browser.address);
        }
        if (scancode == 0x1C) {
            state->browser_page = BROWSER_PAGE_WEB;
            return browser_navigate_input(g_browser.address) || g_browser.address[0] != '\0';
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            return append_query_character(g_browser.address, sizeof(g_browser.address), input);
        }
    }

    if (notepad_is_active_window(state)) {
        if ((state->ctrl_down || state->left_super_down || state->right_super_down) && scancode == 0x1F) {
            notepad_save_to_disk(state);
            return true;
        }

        if (scancode == 0x0E) {
            if (state->notepad_cursor > 0u) {
                state->notepad_buffer[--state->notepad_cursor] = '\0';
                state->notepad_dirty = true;
            }
            return true;
        }

        if (scancode == 0x1C) {
            if (state->notepad_cursor + 1u < sizeof(state->notepad_buffer)) {
                state->notepad_buffer[state->notepad_cursor++] = '\n';
                state->notepad_buffer[state->notepad_cursor] = '\0';
                state->notepad_dirty = true;
            }
            return true;
        }

        input = scancode_to_ascii(scancode);
        if (input != '\0') {
            if (state->notepad_cursor + 1u < sizeof(state->notepad_buffer)) {
                state->notepad_buffer[state->notepad_cursor++] = input;
                state->notepad_buffer[state->notepad_cursor] = '\0';
                state->notepad_dirty = true;
            }
            return true;
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

static bool poll_input(desktop_state_t* state, mouse_state_t* mouse) {
    bool changed = mouse_driver_poll(mouse);
    bool extended_prefix = false;

    while ((inb(0x64) & 0x01u) != 0) {
        u8 status = inb(0x64);
        u8 scancode;
        bool key_released;
        u8 key_code;

        if ((status & 0x20u) != 0) {
            break;
        }

        scancode = inb(0x60);
        if (scancode == 0xE1u) {
            extended_prefix = false;
            continue;
        }
        if (scancode == 0xE0u) {
            extended_prefix = true;
            continue;
        }

        key_released = (scancode & 0x80u) != 0u;
        key_code = (u8)(scancode & 0x7Fu);
        if (state != NULL) {
            if (extended_prefix && (key_code == 0x5Bu || key_code == 0x5Cu)) {
                if (key_code == 0x5Bu) {
                    state->left_super_down = !key_released;
                } else {
                    state->right_super_down = !key_released;
                }
            } else if (key_code == 0x1Du) {
                state->ctrl_down = !key_released;
            } else if (key_code == 0x2Au || key_code == 0x36u) {
                state->shift_down = !key_released;
            }

            if (!key_released &&
                !state->crash_hotkey_triggered &&
                state->ctrl_down &&
                state->shift_down &&
                (key_code == 0x1Du || key_code == 0x2Au || key_code == 0x36u)) {
                state->crash_hotkey_triggered = true;
                changed = true;
                continue;
            }
        }

        if (handle_keyboard_scancode(state, scancode)) {
            changed = true;
        }
        extended_prefix = false;
    }

    return changed;
}

static void draw_dock_tooltip(const desktop_state_t* state, u32 x, u32 y, const char* text) {
    u32 text_width = (u32)strlen(text) * vga_text_advance_x();
    u32 width = text_width + 16u;
    u32 height = 24u;
    u32 tooltip_x = x - (width / 2u);
    u32 tooltip_y = y - height - 8u;
    bool dark = desktop_dark_mode(state);

    u8 r = dark ? 30u : 255u;
    u8 g = dark ? 34u : 255u;
    u8 b = dark ? 48u : 255u;

    draw_rounded_panel(tooltip_x, tooltip_y, width, height, 6u, vga_color(r, g, b), ui_stroke_color(state, 1u));
    draw_text(tooltip_x + 8u, tooltip_y + 5u, text, ui_text_primary(state));
}

static void draw_dock(const desktop_state_t* state, const mouse_state_t* mouse) {
    dock_layout_t layout;
    bool dark = desktop_dark_mode(state);
    s32 hovered_index = -1;
    u32 hovered_x = 0;
    u32 hovered_y = 0;
    dock_app_t hovered_app = APP_START;

    get_dock_layout(state, &layout);

    if (layout.style == TASKBAR_STYLE_TENUI) {
        u8 r = dark ? 22u : 232u;
        u8 g = dark ? 24u : 236u;
        u8 b = dark ? 34u : 248u;
        u8 alpha = 160u;
        u8 highlight_alpha = dark ? 20u : 96u;

        vga_fill_rounded_rect_alpha(layout.dock_x, layout.dock_y, layout.dock_width, layout.dock_height, 10u, r, g, b, alpha);
        
        // Depth effects
        vga_fill_rect_alpha(layout.dock_x, layout.dock_y, layout.dock_width, 1u, 255, 255, 255, highlight_alpha + 40u);
        vga_fill_rect_alpha(layout.dock_x, layout.dock_y + layout.dock_height - 1u, layout.dock_width, 1u, 0, 0, 0, 48);

        draw_rounded_rect_outline(layout.dock_x,
                                  layout.dock_y,
                                  layout.dock_width,
                                  layout.dock_height,
                                  10u,
                                  1u,
                                  dark ? vga_color(58, 64, 86) : vga_color(196, 204, 220));

        dock_app_t visible_apps[DOCK_APP_MAX];
        u32 count = get_visible_dock_apps(state, visible_apps);

        for (u32 index = 0; index < count; ++index) {
            dock_app_t app = visible_apps[index];
            u32 tile_x;
            u32 tile_y;
            u32 tile_width;
            u32 tile_height;
            u32 icon_x;
            u32 icon_y;
            bool active = dock_app_active(state, app);
            bool visible = window_visible(state, dock_app_to_window(app));
            u32 tile_fill;
            u32 tile_outline;

            dock_tile_rect(&layout, index, count, &tile_x, &tile_y, &tile_width, &tile_height);
            
            if (mouse != NULL && point_in_rect(mouse->x, mouse->y, tile_x, tile_y, tile_width, tile_height)) {
                hovered_index = (s32)index;
                hovered_x = tile_x + (tile_width / 2u);
                hovered_y = tile_y;
                hovered_app = app;
            }

            if (index == 0u) {
                tile_fill = vga_color(138, 92, 255);
                tile_outline = vga_color(224, 210, 255);
            } else {
                tile_fill = active ? (visible ? vga_color(24, 24, 28) : vga_color(16, 16, 18))
                                   : vga_color(0, 0, 0);
                tile_outline = active ? vga_color(90, 92, 104) : vga_color(54, 54, 60);
            }

            if (index == 0u) {
                draw_rounded_rect(tile_x, tile_y, tile_width, tile_height, 8u, tile_fill);
                vga_fill_rect_alpha(tile_x + 2u, tile_y + 2u, tile_width - 4u, 9u, 255, 255, 255, 34u);
            } else {
                // Transparent tiles for apps
                vga_fill_rounded_rect_alpha(tile_x, tile_y, tile_width, tile_height, 8u, 0, 0, 0, active ? 64u : 32u);
                vga_fill_rect_alpha(tile_x + 2u, tile_y + 2u, tile_width - 4u, 9u, 255, 255, 255, active ? 12u : 4u);
            }
            draw_rounded_rect_outline(tile_x, tile_y, tile_width, tile_height, 8u, 1u, tile_outline);

            if (index == 0u) {
                draw_text(tile_x + 13u, tile_y + 15u, "Start", vga_color(250, 248, 255));
            } else {
                icon_x = tile_x + ((tile_width > layout.icon_size) ? (tile_width - layout.icon_size) / 2u : 0u);
                icon_y = tile_y + ((tile_height > layout.icon_size) ? (tile_height - layout.icon_size) / 2u : 0u);
                draw_app_icon(icon_x, icon_y, layout.icon_size, app);
            }

            if (active) {
                draw_rounded_rect(tile_x + 8u, layout.dock_y + layout.dock_height - 5u, tile_width - 16u, 2u, 1u,
                                  dark ? vga_color(150, 184, 255) : vga_color(66, 120, 236));
            }
        }

    
        vga_fill_rounded_rect_alpha(layout.dock_width - 118u, layout.dock_y + 6u, 108u, 36u, 8u, r, g, b, 48u);
        draw_rounded_rect_outline(layout.dock_width - 118u,
                                  layout.dock_y + 6u,
                                  108u,
                                  36u,
                                  8u,
                                  1u,
                                  dark ? vga_color(68, 74, 98) : vga_color(192, 198, 214));
        {
            char clock_buffer[9];
            rtc_read_time_string(clock_buffer);
            draw_text(layout.dock_width - 101u,
                      layout.dock_y + 19u,
                      clock_buffer,
                      dark ? vga_color(232, 236, 248) : vga_color(36, 44, 68));
        }

        if (hovered_index != -1) {
            draw_dock_tooltip(state, hovered_x, hovered_y, dock_app_name(hovered_app));
        }

        return;
    }

    // Default style transparency
    u8 def_r = dark ? 22u : 232u;
    u8 def_g = dark ? 24u : 236u;
    u8 def_b = dark ? 34u : 248u;
    vga_fill_rounded_rect_alpha(layout.dock_x, layout.dock_y, layout.dock_width, layout.dock_height, 28u, def_r, def_g, def_b, 160u);
    vga_fill_rounded_rect_alpha(layout.dock_x + 1u,
                                layout.dock_y + 1u,
                                layout.dock_width - 2u,
                                layout.dock_height - 2u,
                                27u,
                                255,
                                255,
                                255,
                                dark ? 28u : 60u);
    vga_fill_rounded_rect_alpha(layout.dock_x + 10u,
                                layout.dock_y + 6u,
                                layout.dock_width - 20u,
                                12u,
                                6u,
                                255,
                                255,
                                255,
                                dark ? 10u : 36u);
    draw_rounded_rect_outline(layout.dock_x,
                              layout.dock_y,
                              layout.dock_width,
                              layout.dock_height,
                              28u,
                              1u,
                              dark ? vga_color(56, 60, 82) : vga_color(238, 236, 252));

    dock_app_t visible_apps[DOCK_APP_MAX];
    u32 count = get_visible_dock_apps(state, visible_apps);

    for (u32 index = 0; index < count; ++index) {
        dock_app_t app = visible_apps[index];
        u32 tile_x;
        u32 tile_y;
        u32 tile_width;
        u32 tile_height;
        u32 icon_x;
        u32 icon_y;
        bool active = dock_app_active(state, app);
        bool visible = window_visible(state, dock_app_to_window(app));

        dock_tile_rect(&layout, index, count, &tile_x, &tile_y, &tile_width, &tile_height);

        if (mouse != NULL && point_in_rect(mouse->x, mouse->y, tile_x, tile_y, tile_width, tile_height)) {
            hovered_index = (s32)index;
            hovered_x = tile_x + (tile_width / 2u);
            hovered_y = tile_y;
            hovered_app = app;
        }

        icon_x = tile_x + ((tile_width > layout.icon_size) ? (tile_width - layout.icon_size) / 2u : 0u);
        icon_y = tile_y + ((tile_height > layout.icon_size) ? (tile_height - layout.icon_size) / 2u : 0u);

        if (index == 0u) {
            draw_rounded_rect(tile_x, tile_y, tile_width, tile_height, 18u, vga_color(152, 160, 244));
            vga_fill_rounded_rect_alpha(tile_x + 2u, tile_y + 2u, tile_width - 4u, 12u, 8u, 255, 255, 255, 54);
            draw_rounded_rect_outline(tile_x, tile_y, tile_width, tile_height, 18u, 1u, vga_color(232, 236, 255));
        } else {
            draw_rounded_rect(tile_x, tile_y, tile_width, tile_height, 14u,
                              active ? (visible ? vga_color(32, 34, 42) : vga_color(22, 24, 30))
                                     : vga_color(12, 14, 18));
            vga_fill_rounded_rect_alpha(tile_x + 2u, tile_y + 2u, tile_width - 4u, 9u, 7u, 255, 255, 255,
                                        active ? 18u : 10u);
            draw_rounded_rect_outline(tile_x, tile_y, tile_width, tile_height, 14u, 1u,
                                      active ? vga_color(76, 82, 106) : vga_color(42, 46, 58));
        }

        if (index == 0u) {
            draw_text(tile_x + 10u, tile_y + 15u, "Start", vga_color(250, 252, 255));
        } else {
            draw_app_icon(icon_x, icon_y, layout.icon_size, app);
        }
        if (active) {
            draw_rounded_rect(tile_x + 10u, layout.dock_y + layout.dock_height - 8u, tile_width - 20u, 3u, 1u,
                              vga_color(136, 126, 255));
        }
    }

    draw_rounded_rect(layout.dock_x + layout.dock_width - 104u,
                      layout.dock_y + 11u,
                      86u,
                      40u,
                      16u,
                      dark ? vga_color(40, 44, 66) : vga_color(202, 206, 255));
    vga_fill_rounded_rect_alpha(layout.dock_x + layout.dock_width - 102u,
                                layout.dock_y + 13u,
                                82u,
                                12u,
                                6u,
                                255,
                                255,
                                255,
                                dark ? 10u : 26u);
    draw_rounded_rect_outline(layout.dock_x + layout.dock_width - 104u, layout.dock_y + 11u, 86u, 40u, 16u, 1u,
                              dark ? vga_color(64, 68, 96) : vga_color(242, 244, 255));
    {
        char clock_buffer[9];
        rtc_read_time_string(clock_buffer);
        draw_text(layout.dock_x + layout.dock_width - 95u,
                  layout.dock_y + 25u,
                  clock_buffer,
                  dark ? vga_color(232, 236, 248) : vga_color(34, 40, 64));
    }

    if (hovered_index != -1) {
        draw_dock_tooltip(state, hovered_x, hovered_y, dock_app_name(hovered_app));
    }
}

static void launcher_row_rect(const window_rect_t* rect, u32 row_index, u32* out_x, u32* out_y, u32* out_width, u32* out_height) {
    if (rect == NULL) {
        return;
    }

    if (out_x != NULL) {
        *out_x = rect->x + 24u;
    }
    if (out_y != NULL) {
        *out_y = rect->y + 88u + row_index * 36u;
    }
    if (out_width != NULL) {
        *out_width = rect->width - 48u;
    }
    if (out_height != NULL) {
        *out_height = 30u;
    }
}

static void launcher_app_tile_rect(const window_rect_t* rect,
                                   u32 app_index,
                                   u32* out_x,
                                   u32* out_y,
                                   u32* out_width,
                                   u32* out_height) {
    if (rect == NULL) {
        return;
    }
    if (out_x != NULL) {
        *out_x = rect->x + 24u;
    }
    if (out_y != NULL) {
        *out_y = rect->y + 88u + app_index * 36u;
    }
    if (out_width != NULL) {
        *out_width = rect->width - 48u;
    }
    if (out_height != NULL) {
        *out_height = 30u;
    }
}

static void launcher_recommended_tile_rect(const window_rect_t* rect,
                                           u32 item_index,
                                           u32* out_x,
                                           u32* out_y,
                                           u32* out_width,
                                           u32* out_height) {
    u32 tile_width;
    u32 tile_height = 52u;
    u32 gap_x = 24u;
    u32 start_x;

    if (rect == NULL) {
        return;
    }

    start_x = rect->x + 26u;
    tile_width = safe_subtract(rect->width, 64u) / 2u;
    if (out_x != NULL) {
        *out_x = start_x + item_index * (tile_width + gap_x);
    }
    if (out_y != NULL) {
        *out_y = rect->y + 430u;
    }
    if (out_width != NULL) {
        *out_width = tile_width;
    }
    if (out_height != NULL) {
        *out_height = tile_height;
    }
}

static void launcher_power_button_rect(const window_rect_t* rect,
                                       u32* out_x,
                                       u32* out_y,
                                       u32* out_width,
                                       u32* out_height) {
    if (rect == NULL) {
        return;
    }

    if (out_x != NULL) {
        *out_x = rect->x + rect->width - 64u;
    }
    if (out_y != NULL) {
        *out_y = rect->y + rect->height - 50u;
    }
    if (out_width != NULL) {
        *out_width = 32u;
    }
    if (out_height != NULL) {
        *out_height = 32u;
    }
}

static void launcher_power_menu_rect(const window_rect_t* rect,
                                     u32* out_x,
                                     u32* out_y,
                                     u32* out_width,
                                     u32* out_height) {
    u32 button_x;
    u32 button_y;

    if (rect == NULL) {
        return;
    }

    launcher_power_button_rect(rect, &button_x, &button_y, NULL, NULL);
    if (out_x != NULL) {
        *out_x = button_x - 38u;
    }
    if (out_y != NULL) {
        *out_y = button_y - 94u;
    }
    if (out_width != NULL) {
        *out_width = 162u;
    }
    if (out_height != NULL) {
        *out_height = 84u;
    }
}

static void draw_launcher_row(const desktop_state_t* state, u32 x, u32 y, u32 width, u32 height, dock_app_t app, const char* label, const char* detail) {
    bool dark = desktop_dark_mode(state);
    bool show_detail = detail != NULL && detail[0] != '\0';
    u32 icon_size = 26u;
    u32 badge_size = icon_size + 10u;
    u32 badge_x = x + 5u;
    u32 badge_y = y + ((height > badge_size) ? (height - badge_size) / 2u : 0u);
    u32 icon_x = badge_x + ((badge_size > icon_size) ? (badge_size - icon_size) / 2u : 0u);
    u32 icon_y = badge_y + ((badge_size > icon_size) ? (badge_size - icon_size) / 2u : 0u);

    draw_rounded_panel(x, y, width, height, 12u, ui_surface_color(state, 1u), ui_stroke_color(state, 1u));
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 255, 255, 255, dark ? 10u : 18u);
    draw_rounded_rect(badge_x, badge_y, badge_size, badge_size, 8u, vga_color(12, 14, 20));
    draw_app_icon(icon_x, icon_y, icon_size, app);
    draw_text_clipped(x + 46u,
                      show_detail ? (y + 7u) : (y + ((height > 16u) ? (height - 16u) / 2u : 8u)),
                      width - 60u,
                      label,
                      ui_text_primary(state));
    if (show_detail) {
        draw_text_clipped(x + 46u, y + 19u, width - 60u, detail, ui_text_secondary(state));
    }
}

static void draw_launcher_app_tile(const desktop_state_t* state, u32 x, u32 y, u32 width, u32 height, dock_app_t app, const char* label) {
    draw_launcher_row(state, x, y, width, height, app, label, "");
}

static void draw_launcher_recommended_tile(const desktop_state_t* state, u32 x, u32 y, u32 width, u32 height, dock_app_t app, const char* label) {
    bool dark = desktop_dark_mode(state);
    u32 icon_size = 32u;
    u32 badge_size = icon_size + 10u;
    u32 badge_x = x + 7u;
    u32 badge_y = y + 5u;
    u32 icon_x = badge_x + ((badge_size > icon_size) ? (badge_size - icon_size) / 2u : 0u);
    u32 icon_y = badge_y + ((badge_size > icon_size) ? (badge_size - icon_size) / 2u : 0u);

    draw_rounded_panel(x, y, width, height, 16u, ui_surface_color(state, 2u), ui_stroke_color(state, 1u));
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 14u, 255, 255, 255, dark ? 8u : 16u);
    draw_rounded_rect(badge_x, badge_y, badge_size, badge_size, 9u, vga_color(12, 14, 20));
    draw_app_icon(icon_x, icon_y, icon_size, app);
    draw_text_clipped(x + 56u, y + 19u, width - 72u, label, ui_text_primary(state));
}

static void draw_launcher_power_button(const desktop_state_t* state, u32 x, u32 y, u32 width, u32 height) {
    u32 color = ui_text_primary(state);

    draw_rounded_panel(x, y, width, height, width / 2u, ui_surface_color(state, 2u), ui_stroke_color(state, 1u));
    draw_text_clipped(x + 6u, y + 10u, width - 8u, "OI", color);
}

static void draw_launcher_panel(const desktop_state_t* state, const window_rect_t* rect) {
    bool searching;
    u32 visible_row = 0;
    u32 visible_tile = 0;
    u32 power_x;
    u32 power_y;
    u32 power_width;
    u32 power_height;
    u32 search_width;
    u32 text_width_limit;

    if (rect == NULL || state == NULL) {
        return;
    }

    searching = state->launcher_query[0] != '\0';

    (void)state;

    bool dark = desktop_dark_mode(state);
    u8 r = dark ? 22u : 232u;
    u8 g = dark ? 24u : 236u;
    u8 b = dark ? 34u : 248u;
    u8 highlight_alpha = dark ? 20u : 96u;

    // Shadow layers
    vga_fill_rounded_rect_alpha(rect->x + 8u, rect->y + 10u, rect->width, rect->height, 28u, 0, 0, 0, 28u);
    vga_fill_rounded_rect_alpha(rect->x + 4u, rect->y + 5u, rect->width, rect->height, 28u, 0, 0, 0, 18u);
    
    // Main Solid Panel
    draw_rounded_rect(rect->x, rect->y, rect->width, rect->height, 28u, vga_color(r, g, b));
    
    // Depth highlights
    vga_fill_rect_alpha(rect->x + 10u, rect->y + 2u, rect->width - 20u, 1u, 255, 255, 255, highlight_alpha + 60u);
    draw_rounded_rect_outline(rect->x, rect->y, rect->width, rect->height, 28u, 1u, ui_stroke_color(state, 0u));

    // Ded search bar
    draw_rounded_panel(rect->x + 20u, rect->y + 18u, rect->width - 40u, 34u, 14u, ui_input_fill(state), ui_input_stroke(state));
    search_width = safe_subtract(rect->width, 64u);
    text_width_limit = safe_subtract(rect->width, 44u);
    if (state->launcher_query[0] != '\0') {
        draw_text_clipped(rect->x + 32u, rect->y + 31u, search_width, state->launcher_query, ui_text_primary(state));
    } else {
        draw_text_clipped(rect->x + 32u, rect->y + 31u, search_width, "Search apps", ui_text_muted(state));
    }

    if (!searching) {
        draw_text_clipped(rect->x + 24u, rect->y + 62u, text_width_limit, "Apps", ui_text_secondary(state));
        for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
            u32 tile_x;
            u32 tile_y;
            u32 tile_width;
            u32 tile_height;

            if (g_launcher_window_ids[index] == WINDOW_JABVER) {
                continue;
            }
            launcher_app_tile_rect(rect, visible_tile, &tile_x, &tile_y, &tile_width, &tile_height);
            draw_launcher_app_tile(state, tile_x, tile_y, tile_width, tile_height, g_launcher_dock_apps[index], g_launcher_app_names[index]);
            ++visible_tile;
        }
        draw_text_clipped(rect->x + 24u, rect->y + 402u, text_width_limit, "Recommended apps", ui_text_secondary(state));
        for (u32 index = 0; index < LAUNCHER_RECOMMENDED_COUNT; ++index) {
            u32 tile_x;
            u32 tile_y;
            u32 tile_width;
            u32 tile_height;
            u32 app_index = g_launcher_recommended_indices[index];

            launcher_recommended_tile_rect(rect, index, &tile_x, &tile_y, &tile_width, &tile_height);
            draw_launcher_recommended_tile(state, tile_x, tile_y, tile_width, tile_height,
                                           g_launcher_dock_apps[app_index],
                                           g_launcher_app_names[app_index]);
        }
    } else {
        draw_text_clipped(rect->x + 24u, rect->y + 62u, text_width_limit, "Search results", ui_text_secondary(state));
        for (u32 index = 0; index < LAUNCHER_APP_COUNT; ++index) {
            u32 row_x;
            u32 row_y;
            u32 row_width;
            u32 row_height;

            if (g_launcher_window_ids[index] == WINDOW_JABVER) {
                continue;
            }
            if (!string_contains_case_insensitive(g_launcher_app_names[index], state->launcher_query)) {
                continue;
            }

            launcher_row_rect(rect, visible_row, &row_x, &row_y, &row_width, &row_height);
            draw_launcher_row(state, row_x, row_y, row_width, row_height, g_launcher_dock_apps[index], g_launcher_app_names[index], "");
            ++visible_row;
        }

        if (visible_row == 0u) {
            draw_text_clipped(rect->x + 24u, rect->y + 102u, text_width_limit, "No apps match your search.", ui_text_muted(state));
        }
    }

    // Bottom utility bar glass
    vga_fill_rounded_rect_alpha(rect->x + 20u, rect->y + rect->height - 58u, rect->width - 40u, 40u, 16u, 255, 255, 255, dark ? 8u : 32u);
    draw_rounded_rect_outline(rect->x + 20u, rect->y + rect->height - 58u, rect->width - 40u, 40u, 16u, 1u, ui_stroke_color(state, 0u));

    launcher_power_button_rect(rect, &power_x, &power_y, &power_width, &power_height);
    draw_launcher_power_button(state, power_x, power_y, power_width, power_height);

    if (state->launcher_power_menu_open) {
        u32 menu_x;
        u32 menu_y;
        u32 menu_width;
        u32 menu_height;

        launcher_power_menu_rect(rect, &menu_x, &menu_y, &menu_width, &menu_height);
        
        // Power menu (Opaque)
        draw_rounded_rect(menu_x, menu_y, menu_width, menu_height, 14u, vga_color(r, g, b));
        draw_rounded_rect_outline(menu_x, menu_y, menu_width, menu_height, 14u, 1u, ui_stroke_color(state, 0u));
        
        draw_rounded_panel(menu_x + 10u, menu_y + 10u, menu_width - 20u, 28u, 10u, ui_surface_color(state, 2u), ui_stroke_color(state, 1u));
        draw_rounded_panel(menu_x + 10u, menu_y + 46u, menu_width - 20u, 28u, 10u, ui_surface_color(state, 2u), ui_stroke_color(state, 1u));
        draw_text_clipped(menu_x + 20u, menu_y + 18u, menu_width - 40u, "Shut down", ui_text_primary(state));
        draw_text_clipped(menu_x + 20u, menu_y + 54u, menu_width - 40u, "Restart", ui_text_primary(state));
    }
}

static void draw_run_dialog_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;
    u32 body_x;
    u32 body_y;
    u32 body_width;
    u32 text_width_limit;
    u32 input_y;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_RUN) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Run Dialog", vga_color(86, 118, 226));
    if (shaded) {
        return;
    }

    body_x = rect->x + 18u;
    body_y = rect->y + 56u;
    body_width = rect->width - 36u;
    text_width_limit = safe_subtract(body_width, 24u);
    input_y = rect->y + 94u;

    draw_rounded_panel(body_x, body_y, body_width, rect->height - 74u, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 1u), ui_stroke_color(state, 0u));
    draw_text_clipped(body_x + 12u, body_y + 12u, text_width_limit,
                      "Search for an app and press Enter to open it.", ui_text_secondary(state));

    draw_rounded_panel(body_x + 12u, input_y, body_width - 24u, 34u, 12u,
                       ui_input_fill(state), ui_input_stroke(state));
    if (state->run_query[0] != '\0') {
        draw_text_clipped(body_x + 24u, input_y + 11u, safe_subtract(body_width, 52u), state->run_query, ui_text_primary(state));
    } else {
        draw_text_clipped(body_x + 24u, input_y + 11u, safe_subtract(body_width, 52u), "Search", ui_text_muted(state));
    }
}

static void draw_settings_option_row(const desktop_state_t* state, u32 x, u32 y, u32 width, const char* label, const char* value) {
    u32 value_width = width > 180u ? width / 2u : width / 3u;
    u32 label_width = width > value_width + 34u ? width - value_width - 34u : width / 2u;

    draw_rounded_panel(x, y, width, 34u, 12u, ui_surface_color(state, 2u), ui_stroke_color(state, 1u));
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 255, 255, 255, desktop_dark_mode(state) ? 8u : 20u);
    draw_text_clipped(x + 14u, y + 9u, label_width, label, ui_text_primary(state));
    draw_text_right_clipped(x + width - 16u - value_width, y + 9u, value_width, value, ui_text_primary(state));
}

static void draw_settings_tab(const desktop_state_t* state, u32 x, u32 y, const char* label, bool active) {
    u32 label_width = text_width(label);
    u32 tab_width = 126u;
    u32 text_x = x + ((tab_width > label_width) ? (tab_width - label_width) / 2u : 8u);
    u32 active_fill = desktop_dark_mode(state) ? ui_surface_color(state, 3u) : vga_color(230, 236, 248);
    u32 inactive_fill = desktop_dark_mode(state) ? ui_surface_color(state, 2u) : vga_color(236, 238, 244);
    u32 active_stroke = desktop_dark_mode(state) ? ui_stroke_color(state, 3u) : vga_color(86, 108, 164);
    u32 inactive_stroke = desktop_dark_mode(state) ? ui_stroke_color(state, 1u) : vga_color(84, 88, 100);

    draw_rounded_panel(x,
                       y,
                       126u,
                       28u,
                       12u,
                       active ? active_fill : inactive_fill,
                       active ? active_stroke : inactive_stroke);
    vga_fill_rect_alpha(x + 2u, y + 2u, 122u, 9u, 255, 255, 255, active ? (desktop_dark_mode(state) ? 10u : 14u) : (desktop_dark_mode(state) ? 6u : 10u));
    draw_text_clipped(text_x, y + 6u, tab_width - 16u, label, active ? ui_text_accent(state) : ui_text_primary(state));
}

static void draw_settings_nav_item(const desktop_state_t* state, u32 x, u32 y, u32 width, const char* label, bool active) {
    u32 bg = active ? ui_surface_color(state, 3u) : ui_surface_color(state, 1u);
    u32 outline = active ? ui_stroke_color(state, 3u) : ui_stroke_color(state, 1u);
    u32 text = active ? ui_text_accent(state) : ui_text_primary(state);

    draw_rounded_panel(x, y, width, 34u, 12u, bg, outline);
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 255, 255, 255, active ? (desktop_dark_mode(state) ? 10u : 18u) : (desktop_dark_mode(state) ? 6u : 10u));
    draw_text_clipped(x + 14u, y + 9u, safe_subtract(width, 28u), label, text);
}

static void draw_settings_primary_button(const desktop_state_t* state, u32 x, u32 y, u32 width, const char* label, bool enabled) {
    u32 fill = enabled ? (desktop_dark_mode(state) ? vga_color(78, 122, 224) : vga_color(92, 132, 210))
                       : (desktop_dark_mode(state) ? vga_color(70, 76, 92) : vga_color(120, 126, 142));
    u32 outline = enabled ? (desktop_dark_mode(state) ? vga_color(142, 178, 255) : vga_color(232, 236, 255))
                          : (desktop_dark_mode(state) ? ui_stroke_color(state, 1u) : vga_color(190, 196, 214));
    u32 text = enabled ? vga_color(250, 252, 255) : vga_color(230, 232, 240);
    u32 label_width = text_width(label);
    u32 text_x = x + ((width > label_width) ? (width - label_width) / 2u : 8u);

    draw_rounded_rect(x, y, width, 34u, 14u, fill);
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 255, 255, 255, enabled ? (desktop_dark_mode(state) ? 18u : 42u) : (desktop_dark_mode(state) ? 8u : 18u));
    draw_rounded_rect_outline(x, y, width, 34u, 14u, 1u, outline);
    draw_text_clipped(text_x, y + 9u, safe_subtract(width, 16u), label, text);
}

static void update_prompt_rect(window_rect_t* out_rect) {
    u32 width = 540u;
    u32 height = 224u;
    u32 screen_w = display_driver_width();
    u32 screen_h = display_driver_height();
    window_rect_t rect = {0u, 0u, width, height};

    if (screen_w > width) {
        rect.x = (screen_w - width) / 2u;
    }
    if (screen_h > height) {
        rect.y = (screen_h - height) / 2u;
    }

    if (out_rect != NULL) {
        *out_rect = rect;
    }
}

static void draw_secondary_button(const desktop_state_t* state, u32 x, u32 y, u32 width, const char* label) {
    u32 fill = ui_surface_color(state, 2u);
    u32 outline = ui_stroke_color(state, 1u);
    u32 text = ui_text_primary(state);
    u32 label_width = text_width(label);
    u32 text_x = x + ((width > label_width) ? (width - label_width) / 2u : 8u);

    draw_rounded_panel(x, y, width, 34u, 14u, fill, outline);
    vga_fill_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 255, 255, 255, desktop_dark_mode(state) ? 10u : 22u);
    draw_text_clipped(text_x, y + 9u, safe_subtract(width, 16u), label, text);
}

static void draw_update_prompt_overlay(const desktop_state_t* state, const dirty_rect_list_t* dirty_rects) {
    window_rect_t rect = {0u, 0u, 0u, 0u};
    u32 screen_w;
    u32 screen_h;
    u32 panel_x;
    u32 panel_y;
    u32 panel_width;
    u32 panel_height;
    u32 content_x;
    u32 content_y;
    u32 content_width;
    u32 button_y;
    u32 secondary_width = 120u;
    u32 primary_width = 180u;
    const char* title = "Update available";

    if (state == NULL || !state->update_prompt_open || !state->update_available) {
        return;
    }

    screen_w = display_driver_width();
    screen_h = display_driver_height();

    rect = (window_rect_t){0u, 0u, screen_w, screen_h};
    if (!dirty_rect_list_intersects_rect(dirty_rects, &rect)) {
        return;
    }

    // Backdrop.
    vga_fill_rect_alpha(0u, 0u, screen_w, screen_h, 0, 0, 0, 70u);

    update_prompt_rect(&rect);
    panel_x = rect.x;
    panel_y = rect.y;
    panel_width = rect.width;
    panel_height = rect.height;

    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, 26u, ui_surface_color(state, 1u), ui_stroke_color(state, 0u));
    vga_fill_rect_alpha(panel_x + 14u, panel_y + 10u, panel_width - 28u, 12u, 255, 255, 255, desktop_dark_mode(state) ? 12u : 36u);

    draw_text(panel_x + 20u, panel_y + 18u, title, ui_text_primary(state));

    content_x = panel_x + 20u;
    content_y = panel_y + 54u;
    content_width = safe_subtract(panel_width, 40u);

    draw_text_clipped(content_x, content_y, content_width, state->update_label, ui_text_accent(state));
    {
        char version_line[48];
        version_line[0] = 'v';
        version_line[1] = '\0';
        append_uint(version_line + 1u, state->update_version);
        draw_text_clipped(content_x, content_y + 22u, content_width, version_line, ui_text_secondary(state));
    }

    draw_text_wrapped(content_x,
                      content_y + 52u,
                      content_width,
                      54u,
                      "A new Global JabulOS Update was published. Install it now?",
                      ui_text_secondary(state));

    button_y = panel_y + panel_height - 52u;

    if (state->update_notification_started_ms != 0u) {
        u64 elapsed = timer_ticks_ms() - state->update_notification_started_ms;
        if (elapsed < 5000u) {
            char countdown[64];
            u32 remaining = 5u - (u32)(elapsed / 1000u);
            strcpy(countdown, "Installing automatically in ");
            append_uint(countdown + strlen(countdown), remaining);
            append_text(countdown + strlen(countdown), "s...");
            draw_text_clipped(content_x, button_y - 24u, content_width, countdown, vga_color(226, 76, 76));
        }
    }

    draw_secondary_button(state, panel_x + 20u, button_y, secondary_width, "Later");
    draw_settings_primary_button(state, panel_x + panel_width - 20u - primary_width, button_y, primary_width, "Install update", true);

    (void)screen_h;
}

static bool handle_update_prompt_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    window_rect_t rect;
    u32 panel_x;
    u32 panel_y;
    u32 panel_width;
    u32 panel_height;
    u32 button_y;
    u32 secondary_width = 120u;
    u32 primary_width = 180u;
    u32 primary_x;

    if (state == NULL || !state->update_prompt_open || !state->update_available) {
        return false;
    }

    update_prompt_rect(&rect);
    panel_x = rect.x;
    panel_y = rect.y;
    panel_width = rect.width;
    panel_height = rect.height;
    button_y = panel_y + panel_height - 52u;
    primary_x = panel_x + panel_width - 20u - primary_width;

    // Modal: clicking outside dismisses until a newer staged version appears.
    if (!point_in_rect(mouse_x, mouse_y, panel_x, panel_y, panel_width, panel_height)) {
        state->update_prompt_open = false;
        state->update_prompt_dismissed_version = state->update_version;
        state->update_notification_started_ms = 0u;
        return true;
    }

    if (point_in_rect(mouse_x, mouse_y, panel_x + 20u, button_y, secondary_width, 34u)) {
        state->update_prompt_open = false;
        state->update_prompt_dismissed_version = state->update_version;
        state->update_notification_started_ms = 0u;
        return true;
    }

    if (point_in_rect(mouse_x, mouse_y, primary_x, button_y, primary_width, 34u)) {
        state->update_prompt_open = false;
        state->update_notification_started_ms = 0u;
        set_setup_phase(state, SETUP_PHASE_UPDATE_PROGRESS, rtc_read_seconds_of_day());
        return true;
    }

    return true;
}

static void draw_settings_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    char memory_total[24];
    char display_mode[24];
    char storage_size[24];
    char gpu_model[48];
    const char* kiosk_name = "JabulKiosk";
    u32 frame_height;
    u32 nav_x;
    u32 nav_y;
    u32 nav_width;
    u32 nav_height;
    u32 content_x;
    u32 content_y;
    u32 content_width;
    u32 content_height;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_SETTINGS) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Settings", vga_color(92, 132, 210));
    if (shaded) {
        return;
    }

    nav_width = 196u;
    nav_x = rect->x + 18u;
    nav_y = rect->y + 56u;
    nav_height = rect->height > 74u ? rect->height - 74u : 0u;
    draw_rounded_panel(nav_x, nav_y, nav_width, nav_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 1u), ui_stroke_color(state, 0u));

    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 14u,
                           nav_width - 20u,
                           "System Specs",
                           state->settings_page == SETTINGS_PAGE_SYSTEM_SPECS);
    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 54u,
                           nav_width - 20u,
                           "Personalize",
                           state->settings_page == SETTINGS_PAGE_PERSONALIZE);
    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 94u,
                           nav_width - 20u,
                           "Themes",
                           state->settings_page == SETTINGS_PAGE_THEMES);
    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 134u,
                           nav_width - 20u,
                           "Update",
                           state->settings_page == SETTINGS_PAGE_UPDATE);

    content_x = nav_x + nav_width + 16u;
    content_y = nav_y;
    content_width = rect->width > (content_x - rect->x + 18u) ? rect->width - (content_x - rect->x) - 18u : 0u;
    content_height = nav_height;
    draw_rounded_panel(content_x, content_y, content_width, content_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));

    if (state->settings_page == SETTINGS_PAGE_SYSTEM_SPECS) {
        format_megabytes(memory_total, pmm_total_memory());
        format_display_mode(display_mode);
        format_storage_size(storage_size, state->storage_info.total_sectors);
        strcpy(gpu_model, "UEFI Framebuffer ");
        append_text(gpu_model + strlen(gpu_model), display_mode);

        draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "System Specs", ui_text_secondary(state));
        draw_settings_option_row(state, content_x + 16u, content_y + 50u, safe_subtract(content_width, 32u), "CPU", state->cpu_brand);
        draw_settings_option_row(state, content_x + 16u, content_y + 90u, safe_subtract(content_width, 32u), "GPU", gpu_model);
        draw_settings_option_row(state, content_x + 16u, content_y + 130u, safe_subtract(content_width, 32u), "Kiosk name", kiosk_name);
        return;
    }

    if (state->settings_page == SETTINGS_PAGE_PERSONALIZE) {
        u32 row_x = content_x + 16u;
        u32 row_y = content_y + 50u;
        u32 row_width = safe_subtract(content_width, 32u);
        u32 row_height = 60u;
        u32 style_button_width = 98u;
        u32 style_button_gap = 10u;
        u32 tenui_button_x = row_x + safe_subtract(row_width, style_button_width + 14u);
        u32 macxwin_button_x = tenui_button_x - style_button_gap - style_button_width;
        u32 style_button_y = row_y + 13u;
        bool macxwin_active = state->taskbar_style != TASKBAR_STYLE_TENUI;
        bool tenui_active = state->taskbar_style == TASKBAR_STYLE_TENUI;

        draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "Personalize", ui_text_secondary(state));

        draw_rounded_panel(row_x, row_y, row_width, row_height, 16u, ui_surface_color(state, 2u), ui_stroke_color(state, 1u));
        vga_fill_rect_alpha(row_x + 2u, row_y + 2u, row_width - 4u, 10u, 255, 255, 255, desktop_dark_mode(state) ? 8u : 20u);
        draw_text_clipped(row_x + 14u,
                          row_y + 12u,
                          safe_subtract(row_width, style_button_width * 2u + style_button_gap + 124u),
                          "Taskbar Style",
                          ui_text_primary(state));

        draw_rounded_panel(macxwin_button_x,
                           style_button_y,
                           style_button_width,
                           34u,
                           12u,
                           macxwin_active ? ui_surface_color(state, 3u) : ui_surface_color(state, 1u),
                           macxwin_active ? ui_text_accent(state) : ui_stroke_color(state, 1u));
        vga_fill_rect_alpha(macxwin_button_x + 2u,
                            style_button_y + 2u,
                            style_button_width - 4u,
                            9u,
                            255,
                            255,
                            255,
                            macxwin_active ? (desktop_dark_mode(state) ? 14u : 24u) : (desktop_dark_mode(state) ? 6u : 10u));
        draw_text_clipped(macxwin_button_x + 18u,
                          style_button_y + 10u,
                          style_button_width - 24u,
                          "MACxWIN",
                          macxwin_active ? ui_text_accent(state) : ui_text_primary(state));

        draw_rounded_panel(tenui_button_x,
                           style_button_y,
                           style_button_width,
                           34u,
                           12u,
                           tenui_active ? ui_surface_color(state, 3u) : ui_surface_color(state, 1u),
                           tenui_active ? ui_text_accent(state) : ui_stroke_color(state, 1u));
        vga_fill_rect_alpha(tenui_button_x + 2u,
                            style_button_y + 2u,
                            style_button_width - 4u,
                            9u,
                            255,
                            255,
                            255,
                            tenui_active ? (desktop_dark_mode(state) ? 14u : 24u) : (desktop_dark_mode(state) ? 6u : 10u));
        draw_text_clipped(tenui_button_x + 30u,
                          style_button_y + 10u,
                          style_button_width - 36u,
                          "TenUI",
                          tenui_active ? ui_text_accent(state) : ui_text_primary(state));
        return;
    }

    if (state->settings_page == SETTINGS_PAGE_THEMES) {
        draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "Themes", ui_text_secondary(state));
        draw_text_wrapped(content_x + 16u,
                          content_y + 44u,
                          safe_subtract(content_width, 32u),
                          36u,
                          " ",
                          ui_text_secondary(state));

        for (u32 index = 0u; index < WALLPAPER_THEME_COUNT; ++index) {
            window_rect_t card_rect;
            if (!settings_theme_card_rect(content_x, content_y, content_width, index, &card_rect)) {
                continue;
            }
            draw_settings_theme_card(state, &card_rect, index, state->wallpaper_theme == index);
        }
        return;
    }

    // Its dead, Update page
    draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "Update", ui_text_secondary(state));
    if (!state->storage_info.present) {
        draw_text_wrapped(content_x + 16u,
                          content_y + 50u,
                          safe_subtract(content_width, 32u),
                          44u,
                          "No ATA disk was detected. Updates require a disk image.",
                          ui_danger_color(state));
        return;
    }

    if (state->update_installing) {
        draw_text_wrapped(content_x + 16u,
                          content_y + 50u,
                          safe_subtract(content_width, 32u),
                          44u,
                          "Installing update... please wait.",
                          ui_text_secondary(state));
        return;
    }

    if (state->update_install_complete) {
        draw_text_wrapped(content_x + 16u,
                          content_y + 50u,
                          safe_subtract(content_width, 32u),
                          60u,
                          "The staged JabulOS build was installed to disk successfully.",
                          ui_success_color(state));
        draw_text_wrapped(content_x + 16u,
                          content_y + 118u,
                          safe_subtract(content_width, 32u),
                          44u,
                          "Import another ISO into the updater to stage a newer build.",
                          ui_text_secondary(state));
        return;
    }

    if (state->update_install_failed) {
        draw_text_wrapped(content_x + 16u,
                          content_y + 50u,
                          safe_subtract(content_width, 32u),
                          60u,
                          "The update could not be installed. Make sure a valid staged ISO is available and the ATA disk has enough space.",
                          ui_danger_color(state));
        return;
    }

    if (!state->update_available) {
        draw_text_wrapped(content_x + 16u,
                          content_y + 50u,
                          safe_subtract(content_width, 32u),
                          60u,
                          "No updates found.",
                          ui_text_secondary(state));
        return;
    }

    draw_settings_option_row(state, content_x + 16u, content_y + 56u, safe_subtract(content_width, 32u), "Available update", state->update_label);
    {
        char version[24];
        version[0] = 'v';
        version[1] = '\0';
        append_uint(version + 1u, state->update_version);
        draw_settings_option_row(state, content_x + 16u, content_y + 96u, safe_subtract(content_width, 32u), "Version", version);
    }

    {
        u32 button_width = 180u;
        u32 button_x = content_x + safe_subtract(content_width, button_width + 16u);
        u32 button_y = content_y + safe_subtract(content_height, 52u);
        draw_settings_primary_button(state, button_x, button_y, button_width, "Install update", true);
    }
}

static void draw_notepad_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;
    u32 panel_x, panel_y, panel_width, panel_height;
    u32 toolbar_height = 34u;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_NOTEPAD) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Notepad", vga_color(200, 200, 200));
    if (shaded) {
        return;
    }

    panel_x = rect->x + 8u;
    panel_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 8u;
    panel_width = rect->width - 16u;
    panel_height = rect->height - WINDOW_TITLEBAR_HEIGHT - 16u;

    // Draw toolbar
    draw_rounded_panel(panel_x, panel_y, panel_width, toolbar_height, 12u, ui_surface_color(state, 1u), ui_stroke_color(state, 0u));
    draw_text(panel_x + 12u, panel_y + 10u, "Documents/note.txt (Ctrl+S to save)", ui_text_secondary(state));

    if (state->notepad_dirty) {
        draw_text(panel_x + panel_width - 80u, panel_y + 10u, "Unsaved*", vga_color(200, 80, 80));
    } else {
        draw_text(panel_x + panel_width - 60u, panel_y + 10u, "Saved", vga_color(80, 200, 80));
    }

    // Draw main text area
    u32 text_area_y = panel_y + toolbar_height;
    u32 text_area_height = panel_height - toolbar_height;
    draw_rounded_panel(panel_x, text_area_y, panel_width, text_area_height, 12u, vga_color(255, 255, 255), vga_color(180, 180, 180));
    
    draw_text_wrapped(panel_x + 8u, text_area_y + 8u, panel_width - 16u, text_area_height - 16u, state->notepad_buffer, vga_color(0, 0, 0));

   
    if (notepad_is_active_window(state)) {
        u64 ticks = timer_ticks_ms();
        if ((ticks / 500) % 2 == 0) {
            
        }
    }

    draw_secondary_button(state, panel_x + panel_width - 80u, panel_y + panel_height - 48u, 70u, "Save");
}

static char ascii_lower(char value);

#define NOTEPAD_STORAGE_LBA 20000u
#define NOTEPAD_STORAGE_SECTORS 4u

static void user_files_save_table(const desktop_state_t* state) {
    if (state == NULL || !state->oobe_storage_ready) {
        return;
    }
    g_user_files.magic = USER_FILES_MAGIC;
    storage_write_sectors(USER_FILES_TABLE_LBA, USER_FILES_TABLE_SECTORS, &g_user_files);
}

static void user_files_load_table(const desktop_state_t* state) {
    if (state == NULL || !state->oobe_storage_ready) {
        return;
    }
    if (storage_read_sectors(USER_FILES_TABLE_LBA, USER_FILES_TABLE_SECTORS, &g_user_files)) {
        if (g_user_files.magic != USER_FILES_MAGIC) {
            memset(&g_user_files, 0, sizeof(g_user_files));
            g_user_files.magic = USER_FILES_MAGIC;
            user_files_save_table(state);
        }
    }
}

static void notepad_save_to_disk(desktop_state_t* state) {
    if (state == NULL || !state->oobe_storage_ready) {
        return;
    }

    u64 lba = NOTEPAD_STORAGE_LBA;
    if (state->notepad_user_file_index >= 0 && (u32)state->notepad_user_file_index < USER_FILES_MAX) {
        u32 idx = (u32)state->notepad_user_file_index;
        if (g_user_files.entries[idx].active) {
            lba = g_user_files.entries[idx].lba;
            g_user_files.entries[idx].size = (u32)strlen(state->notepad_buffer);
            user_files_save_table(state);
        }
    }

    if (storage_write_sectors(lba, NOTEPAD_STORAGE_SECTORS, state->notepad_buffer)) {
        state->notepad_dirty = false;
    }
}

static void notepad_load_from_disk(desktop_state_t* state) {
    if (state == NULL || !state->oobe_storage_ready) {
        return;
    }

    u64 lba = NOTEPAD_STORAGE_LBA;
    if (state->notepad_user_file_index >= 0 && (u32)state->notepad_user_file_index < USER_FILES_MAX) {
        u32 idx = (u32)state->notepad_user_file_index;
        if (g_user_files.entries[idx].active) {
            lba = g_user_files.entries[idx].lba;
        }
    }

    if (storage_read_sectors(lba, NOTEPAD_STORAGE_SECTORS, state->notepad_buffer)) {
        state->notepad_buffer[sizeof(state->notepad_buffer) - 1] = '\0';
        state->notepad_cursor = (u32)strlen(state->notepad_buffer);
        state->notepad_dirty = false;
    }
}

static bool string_has_extension(const char* name, const char* extension) {
    size_t name_len;
    size_t ext_len;

    if (name == NULL || extension == NULL) {
        return false;
    }

    name_len = strlen(name);
    ext_len = strlen(extension);
    if (name_len < ext_len) {
        return false;
    }

    for (size_t index = 0u; index < ext_len; ++index) {
        if (ascii_lower(name[name_len - ext_len + index]) != ascii_lower(extension[index])) {
            return false;
        }
    }

    return true;
}

static bool media_is_audio_file(const char* name) {
    return string_has_extension(name, ".mp3") ||
           string_has_extension(name, ".wav") ||
           string_has_extension(name, ".flac") ||
           string_has_extension(name, ".aac") ||
           string_has_extension(name, ".ogg") ||
           string_has_extension(name, ".m4a");
}

static bool media_is_wav_file(const char* name) {
    return string_has_extension(name, ".wav");
}

[[maybe_unused]] static const char* media_display_name(const char* name) {
    const char* display_name = name;

    if (name == NULL) {
        return "";
    }

    for (const char* cursor = name; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            display_name = cursor + 1;
        }
    }
    return display_name;
}

static void file_manager_copy_string(char* destination, u32 capacity, const char* source) {
    u32 index = 0u;

    if (destination == NULL || capacity == 0u) {
        return;
    }
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    while (index + 1u < capacity && source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static void file_manager_append_string(char* destination, u32 capacity, const char* suffix) {
    u32 length;

    if (destination == NULL || capacity == 0u || suffix == NULL) {
        return;
    }

    length = (u32)strlen(destination);
    if (length >= capacity) {
        destination[capacity - 1u] = '\0';
        return;
    }
    file_manager_copy_string(destination + length, capacity - length, suffix);
}

static void file_manager_format_file_size(char* buffer, u32 size_bytes) {
    char number[16];

    if (buffer == NULL) {
        return;
    }

    if (size_bytes >= 1024u * 1024u) {
        append_uint(number, size_bytes / (1024u * 1024u));
        strcpy(buffer, number);
        file_manager_append_string(buffer, FILE_MANAGER_DETAIL_LENGTH, " MiB");
        return;
    }
    if (size_bytes >= 1024u) {
        append_uint(number, size_bytes / 1024u);
        strcpy(buffer, number);
        file_manager_append_string(buffer, FILE_MANAGER_DETAIL_LENGTH, " KiB");
        return;
    }

    append_uint(number, size_bytes);
    strcpy(buffer, number);
    file_manager_append_string(buffer, FILE_MANAGER_DETAIL_LENGTH, " B");
}

static bool file_manager_is_image_file(const char* name) {
    return string_has_extension(name, ".bmp") ||
           string_has_extension(name, ".tga") ||
           string_has_extension(name, ".png") ||
           string_has_extension(name, ".jpg") ||
           string_has_extension(name, ".jpeg") ||
           string_has_extension(name, ".gif") ||
           string_has_extension(name, ".webp");
}

static const char* file_manager_sidebar_prefix(void) {
    switch ((file_manager_sidebar_t)g_file_manager.sidebar) {
        case FILE_MANAGER_SIDEBAR_DOWNLOADS:
            return "Downloads";
        case FILE_MANAGER_SIDEBAR_MUSIC:
            return "assets/music";
        case FILE_MANAGER_SIDEBAR_PHOTOS:
            return "photos";
        case FILE_MANAGER_SIDEBAR_THIS_PC:
        default:
            return "";
    }
}

static void file_manager_build_archive_path(const char* relative_path, char* out_path, u32 out_capacity) {
    const char* sidebar_prefix;

    if (out_path == NULL || out_capacity == 0u) {
        return;
    }

    sidebar_prefix = file_manager_sidebar_prefix();
    out_path[0] = '\0';
    if (sidebar_prefix[0] != '\0') {
        file_manager_copy_string(out_path, out_capacity, sidebar_prefix);
    }
    if (relative_path != NULL && relative_path[0] != '\0') {
        if (out_path[0] != '\0') {
            file_manager_append_string(out_path, out_capacity, "/");
        }
        file_manager_append_string(out_path, out_capacity, relative_path);
    }
}

static void file_manager_step_up_path(void) {
    u32 length = (u32)strlen(g_file_manager.path);

    while (length > 0u) {
        --length;
        if (g_file_manager.path[length] == '/') {
            g_file_manager.path[length] = '\0';
            return;
        }
    }

    g_file_manager.path[0] = '\0';
}

static bool file_manager_ensure_browsable_source(const desktop_state_t* state, const boot_info_t* boot_info) {
    if (g_file_manager.source_kind == FILE_MANAGER_SOURCE_LIVE_MEDIA ||
        g_file_manager.source_kind == FILE_MANAGER_SOURCE_PARTITION_ARCHIVE) {
        return true;
    }

    if (boot_info != NULL && boot_info->initrd_start != 0u && boot_info->initrd_size != 0u) {
        return file_manager_open_source(state, boot_info, FILE_MANAGER_SOURCE_LIVE_MEDIA, 0u);
    }

    file_manager_set_status("No browsable archive is available.");
    return false;
}

static void file_manager_select_sidebar(const desktop_state_t* state,
                                        const boot_info_t* boot_info,
                                        file_manager_sidebar_t sidebar) {
    g_file_manager.sidebar = (u32)sidebar;
    g_file_manager.path[0] = '\0';
    g_file_manager.scroll = 0u;
    g_file_manager.selected_entry = 0u;

    if (sidebar == FILE_MANAGER_SIDEBAR_THIS_PC) {
        g_file_manager.show_this_pc_root = true;
        file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), "This PC");
        file_manager_set_status("Pick a partition or browse the live media.");
    } else {
        g_file_manager.show_this_pc_root = false;
        (void)file_manager_ensure_browsable_source(state, boot_info);
    }

    file_manager_refresh_entries(state, boot_info);
}

static bool file_manager_find_archive_file(const boot_info_t* boot_info,
                                           const char* relative_path,
                                           initrd_file_t* out_file) {
    const void* archive;
    u32 archive_size;
    char archive_path[FILE_MANAGER_PATH_LENGTH];

    if (out_file == NULL || relative_path == NULL) {
        return false;
    }
    if (!file_manager_get_archive_view(boot_info, &archive, &archive_size)) {
        return false;
    }

    file_manager_build_archive_path(relative_path, archive_path, sizeof(archive_path));
    return initrd_find_file(archive, archive_size, archive_path, out_file);
}

static bool file_manager_open_file(desktop_state_t* state,
                                   const boot_info_t* boot_info,
                                   const file_manager_entry_t* entry) {
    initrd_file_t file;
    const void* archive;
    u32 archive_size;
    char archive_path[FILE_MANAGER_PATH_LENGTH];

    if (state == NULL || entry == NULL || entry->directory || entry->source_entry) {
        return false;
    }

    if (strcmp(entry->path, "Documents/note.txt") == 0) {
        state->notepad_user_file_index = -1;
        notepad_load_from_disk(state);
        open_window(state, WINDOW_NOTEPAD);
        file_manager_set_status("Opened note.txt in Notepad.");
        return true;
    }

    if (strncmp(entry->path, "Documents/", 10) == 0) {
        const char* filename = entry->path + 10;
        for (u32 i = 0; i < USER_FILES_MAX; ++i) {
            if (g_user_files.entries[i].active && strcmp(g_user_files.entries[i].name, filename) == 0) {
                state->notepad_user_file_index = (s32)i;
                notepad_load_from_disk(state);
                open_window(state, WINDOW_NOTEPAD);
                char status[128];
                strcpy(status, "Opened ");
                strcat(status, filename);
                strcat(status, " in Notepad.");
                file_manager_set_status(status);
                return true;
            }
        }
    }

    if (!file_manager_find_archive_file(boot_info, entry->path, &file)) {
        file_manager_set_status("That file could not be read from the archive.");
        return true;
    }

    if (entry->image_file) {
        g_file_manager.audio_error_popup = false;
        g_file_manager.preview_data = file.data;
        g_file_manager.preview_size = file.size;
        file_manager_copy_string(g_file_manager.preview_name, sizeof(g_file_manager.preview_name), media_display_name(file.name));
        open_window(state, WINDOW_VIDEOS);
        file_manager_set_status("Opened the selected file in preview.");
        return true;
    }

    if (string_has_extension(entry->name, ".wav") || string_has_extension(entry->name, ".mp3")) {
        g_file_manager.audio_error_popup = true;
        g_file_manager.preview_data = NULL;
        g_file_manager.preview_size = 0u;
        g_file_manager.preview_name[0] = '\0';
        open_window(state, WINDOW_VIDEOS);
        file_manager_set_status("Unsupported audio format.");
        return true;
    }

    if (string_has_extension(entry->name, ".exe")) {
        pe_launch_result_t result;

        if (!file_manager_get_archive_view(boot_info, &archive, &archive_size)) {
            file_manager_set_status("The mounted archive is no longer available.");
            return true;
        }

        file_manager_build_archive_path(entry->path, archive_path, sizeof(archive_path));
        if (pe_launch_initrd_app(archive, archive_size, archive_path, &result) && result.launched) {
            if (result.message[0] != '\0') {
                file_manager_copy_string(g_file_manager.status, sizeof(g_file_manager.status), result.message);
            } else {
                file_manager_set_status("Launched the selected application.");
            }
        } else {
            file_manager_set_status("That executable could not be launched.");
        }
        return true;
    }

    if (string_has_extension(entry->name, ".txt")) {
        u32 copy_size = file.size;
        if (copy_size >= sizeof(state->notepad_buffer)) {
            copy_size = sizeof(state->notepad_buffer) - 1;
        }
        memcpy(state->notepad_buffer, file.data, copy_size);
        state->notepad_buffer[copy_size] = '\0';
        state->notepad_cursor = copy_size;
        state->notepad_dirty = false;
        open_window(state, WINDOW_NOTEPAD);
        file_manager_set_status("Opened text file in Notepad.");
        return true;
    }

    file_manager_set_status("That file type does not have an opener yet.");
    return true;
}

static bool file_manager_activate_entry(desktop_state_t* state,
                                        const boot_info_t* boot_info,
                                        const file_manager_entry_t* entry) {
    if (entry == NULL) {
        return false;
    }

    if (entry->source_entry) {
        if (!file_manager_open_source(state, boot_info, entry->source_kind, entry->partition_index)) {
            return true;
        }

        g_file_manager.show_this_pc_root = false;
        g_file_manager.path[0] = '\0';
        g_file_manager.scroll = 0u;
        g_file_manager.selected_entry = 0u;
        file_manager_refresh_entries(state, boot_info);
        return true;
    }

    if (entry->directory) {
        g_file_manager.show_this_pc_root = false;
        file_manager_copy_string(g_file_manager.path, sizeof(g_file_manager.path), entry->path);
        g_file_manager.scroll = 0u;
        g_file_manager.selected_entry = 0u;
        file_manager_refresh_entries(state, boot_info);
        return true;
    }

    return file_manager_open_file(state, boot_info, entry);
}

static bool file_manager_get_layout(const window_rect_t* rect, file_manager_layout_t* out_layout) {
    if (rect == NULL || out_layout == NULL) {
        return false;
    }

    memset(out_layout, 0, sizeof(*out_layout));
    out_layout->nav_x = rect->x + 18u;
    out_layout->body_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 14u;
    out_layout->nav_y = out_layout->body_y;
    out_layout->nav_width = 144u;
    out_layout->body_height = rect->height - WINDOW_TITLEBAR_HEIGHT - 28u;
    out_layout->content_x = out_layout->nav_x + out_layout->nav_width + 16u;
    out_layout->content_width = rect->width - 36u - out_layout->nav_width - 16u;
    out_layout->rows_y = out_layout->body_y + 84u;
    out_layout->row_height = 40u;
    out_layout->rows_visible = safe_subtract(out_layout->body_height, 138u) / out_layout->row_height;
    if (out_layout->rows_visible == 0u) {
        out_layout->rows_visible = 1u;
    }

    return true;
}

static void file_manager_clear_entries(void) {
    g_file_manager.entry_count = 0u;
    g_file_manager.selected_entry = 0u;
    if (g_file_manager.scroll > 0u) {
        g_file_manager.scroll = 0u;
    }
    memset(g_file_manager.entries, 0, sizeof(g_file_manager.entries));
}

static void file_manager_close_preview(desktop_state_t* state) {
    g_file_manager.preview_data = NULL;
    g_file_manager.preview_size = 0u;
    g_file_manager.preview_name[0] = '\0';
    g_file_manager.audio_error_popup = false;
    if (state != NULL) {
        state->show_videos = false;
    }
}

static void file_manager_release_archive(void) {
    if (g_file_manager.archive_owned && g_file_manager.archive_buffer != NULL && g_file_manager.archive_size != 0u) {
        free_surface_buffer(g_file_manager.archive_buffer, g_file_manager.archive_size);
    }
    g_file_manager.archive_buffer = NULL;
    g_file_manager.archive_size = 0u;
    g_file_manager.archive_owned = false;
}

static void file_manager_set_status(const char* message) {
    file_manager_copy_string(g_file_manager.status, sizeof(g_file_manager.status), message);
}

static bool file_manager_partition_read_header(u32 start_lba, file_manager_install_header_t* out_header) {
    file_manager_install_header_t header;

    if (out_header == NULL || start_lba == 0u || !ata_pio_read_sectors(start_lba, 1, &header)) {
        return false;
    }
    if (memcmp(header.magic, "JBAERO1", 7) != 0 || header.initrd_lba == 0u || header.initrd_size == 0u) {
        return false;
    }

    *out_header = header;
    return true;
}

static bool file_manager_get_archive_view(const boot_info_t* boot_info, const void** out_archive, u32* out_size) {
    if (out_archive != NULL) {
        *out_archive = NULL;
    }
    if (out_size != NULL) {
        *out_size = 0u;
    }

    if (g_file_manager.source_kind == FILE_MANAGER_SOURCE_LIVE_MEDIA) {
        if (boot_info == NULL || boot_info->initrd_start == 0u || boot_info->initrd_size == 0u) {
            return false;
        }
        if (out_archive != NULL) {
            *out_archive = (const void*)boot_info->initrd_start;
        }
        if (out_size != NULL) {
            *out_size = (u32)boot_info->initrd_size;
        }
        return true;
    }

    if (g_file_manager.source_kind == FILE_MANAGER_SOURCE_PARTITION_ARCHIVE &&
        g_file_manager.archive_buffer != NULL &&
        g_file_manager.archive_size != 0u) {
        if (out_archive != NULL) {
            *out_archive = g_file_manager.archive_buffer;
        }
        if (out_size != NULL) {
            *out_size = g_file_manager.archive_size;
        }
        return true;
    }

    return false;
}

static bool file_manager_open_source(const desktop_state_t* state,
                                     const boot_info_t* boot_info,
                                     u32 source_kind,
                                     u32 partition_index) {
    (void)state;

    file_manager_close_preview(NULL);
    file_manager_release_archive();
    g_file_manager.source_kind = FILE_MANAGER_SOURCE_NONE;
    g_file_manager.source_partition_index = partition_index;
    g_file_manager.path[0] = '\0';

    if (source_kind == FILE_MANAGER_SOURCE_LIVE_MEDIA) {
        if (boot_info == NULL || boot_info->initrd_start == 0u || boot_info->initrd_size == 0u) {
            file_manager_set_status("Live media archive is unavailable.");
            return false;
        }
        g_file_manager.source_kind = FILE_MANAGER_SOURCE_LIVE_MEDIA;
        file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), "Live Media");
        file_manager_set_status("Browsing the live JabulOS archive.");
        return true;
    }

    if (source_kind == FILE_MANAGER_SOURCE_PARTITION_ARCHIVE) {
        file_manager_install_header_t header;
        u8* buffer;
        char label[FILE_MANAGER_NAME_LENGTH];

        if (!file_manager_partition_read_header(state->storage_info.partitions[partition_index].start_lba, &header)) {
            file_manager_set_status("That partition does not contain a JabulOS archive.");
            return false;
        }

        buffer = allocate_surface_buffer(header.initrd_size);
        if (buffer == NULL) {
            file_manager_set_status("Not enough memory to mount that partition.");
            return false;
        }
        if (!read_disk_blob(header.initrd_lba, header.initrd_size, buffer)) {
            free_surface_buffer(buffer, header.initrd_size);
            file_manager_set_status("The partition archive could not be read.");
            return false;
        }

        g_file_manager.archive_buffer = buffer;
        g_file_manager.archive_size = header.initrd_size;
        g_file_manager.archive_owned = true;
        g_file_manager.source_kind = FILE_MANAGER_SOURCE_PARTITION_ARCHIVE;
        strcpy(label, "Partition ");
        append_uint(label + strlen(label), partition_index + 1u);
        file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), label);
        file_manager_set_status("Mounted the JabulOS archive from disk.");
        return true;
    }

    if (source_kind == FILE_MANAGER_SOURCE_UNSUPPORTED_PARTITION) {
        char label[FILE_MANAGER_NAME_LENGTH];
        strcpy(label, "Partition ");
        append_uint(label + strlen(label), partition_index + 1u);
        file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), label);
        g_file_manager.source_kind = FILE_MANAGER_SOURCE_UNSUPPORTED_PARTITION;
        file_manager_set_status("This partition filesystem is not supported yet.");
        return true;
    }

    return false;
}

static bool file_manager_entry_exists(const char* path) {
    for (u32 index = 0u; index < g_file_manager.entry_count; ++index) {
        if (strcmp(g_file_manager.entries[index].path, path) == 0) {
            return true;
        }
    }
    return false;
}

static void file_manager_add_entry(const char* name,
                                   const char* detail,
                                   const char* path,
                                   u32 size,
                                   bool directory,
                                   bool image_file,
                                   bool source_entry,
                                   bool browsable,
                                   u32 source_kind,
                                   u32 partition_index) {
    file_manager_entry_t* entry;

    if (name == NULL || path == NULL || g_file_manager.entry_count >= FILE_MANAGER_MAX_ENTRIES || file_manager_entry_exists(path)) {
        return;
    }

    entry = &g_file_manager.entries[g_file_manager.entry_count++];
    memset(entry, 0, sizeof(*entry));
    file_manager_copy_string(entry->name, sizeof(entry->name), name);
    file_manager_copy_string(entry->detail, sizeof(entry->detail), detail);
    file_manager_copy_string(entry->path, sizeof(entry->path), path);
    entry->size = size;
    entry->directory = directory;
    entry->image_file = image_file;
    entry->source_entry = source_entry;
    entry->browsable = browsable;
    entry->source_kind = source_kind;
    entry->partition_index = partition_index;
}

typedef struct {
    char prefix[FILE_MANAGER_PATH_LENGTH];
    bool audio_only;
    bool image_only;
} file_manager_scan_context_t;

static bool file_manager_scan_archive_callback(const initrd_file_t* file, void* user) {
    file_manager_scan_context_t* context = (file_manager_scan_context_t*)user;
    const char* relative;
    const char* separator;
    char child_name[FILE_MANAGER_NAME_LENGTH];
    char child_path[FILE_MANAGER_PATH_LENGTH];
    char detail[FILE_MANAGER_DETAIL_LENGTH];
    u32 copy_length;

    if (file == NULL || file->name == NULL || context == NULL) {
        return true;
    }
    if (context->audio_only && !media_is_audio_file(file->name)) {
        return true;
    }
    if (context->image_only && !file_manager_is_image_file(file->name)) {
        return true;
    }

    if (context->prefix[0] != '\0') {
        u32 prefix_length = (u32)strlen(context->prefix);
        if (strncmp(file->name, context->prefix, prefix_length) != 0 || file->name[prefix_length] != '/') {
            return true;
        }
        relative = file->name + prefix_length + 1u;
    } else {
        relative = file->name;
    }

    if (relative[0] == '\0') {
        return true;
    }

    separator = NULL;
    for (const char* cursor = relative; *cursor != '\0'; ++cursor) {
        if (*cursor == '/') {
            separator = cursor;
            break;
        }
    }
    copy_length = separator != NULL ? (u32)(separator - relative) : (u32)strlen(relative);
    if (copy_length == 0u || copy_length + 1u > sizeof(child_name)) {
        return true;
    }

    memcpy(child_name, relative, copy_length);
    child_name[copy_length] = '\0';
    if (g_file_manager.path[0] != '\0') {
        file_manager_copy_string(child_path, sizeof(child_path), g_file_manager.path);
        file_manager_append_string(child_path, sizeof(child_path), "/");
        file_manager_append_string(child_path, sizeof(child_path), child_name);
    } else {
        file_manager_copy_string(child_path, sizeof(child_path), child_name);
    }

    if (separator != NULL) {
        file_manager_add_entry(child_name,
                               "Folder",
                               child_path,
                               0u,
                               true,
                               false,
                               false,
                               true,
                               g_file_manager.source_kind,
                               g_file_manager.source_partition_index);
        return true;
    }

    file_manager_format_file_size(detail, file->size);
    file_manager_add_entry(child_name,
                           detail,
                           child_path,
                           file->size,
                           false,
                           file_manager_is_image_file(file->name),
                           false,
                           false,
                           g_file_manager.source_kind,
                           g_file_manager.source_partition_index);
    return true;
}

static void file_manager_refresh_entries(const desktop_state_t* state, const boot_info_t* boot_info) {
    const void* archive = NULL;
    u32 archive_size = 0u;
    file_manager_scan_context_t context;

    file_manager_clear_entries();
    memset(&context, 0, sizeof(context));

    if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_THIS_PC && g_file_manager.show_this_pc_root) {
        char detail[FILE_MANAGER_DETAIL_LENGTH];

        if (boot_info != NULL && boot_info->initrd_start != 0u && boot_info->initrd_size != 0u) {
            file_manager_format_file_size(detail, (u32)boot_info->initrd_size);
            file_manager_add_entry("Live Media",
                                   detail,
                                   "live-media",
                                   (u32)boot_info->initrd_size,
                                   true,
                                   false,
                                   true,
                                   true,
                                   FILE_MANAGER_SOURCE_LIVE_MEDIA,
                                   0u);
        }

        if (state != NULL && state->storage_info.present) {
            for (u32 index = 0u; index < ATA_PRIMARY_MASTER_PARTITION_COUNT; ++index) {
                const ata_partition_info_t* partition = &state->storage_info.partitions[index];
                char name[FILE_MANAGER_NAME_LENGTH];
                char size_text[24];
                bool browsable = false;
                u32 source_kind = FILE_MANAGER_SOURCE_UNSUPPORTED_PARTITION;

                if (!partition->present) {
                    continue;
                }

                strcpy(name, "Partition ");
                append_uint(name + strlen(name), index + 1u);
                format_storage_size(size_text, partition->sector_count);
                if (file_manager_partition_read_header(partition->start_lba, NULL)) {
                    browsable = true;
                    source_kind = FILE_MANAGER_SOURCE_PARTITION_ARCHIVE;
                    strcpy(detail, "JabulOS archive ");
                    file_manager_append_string(detail, sizeof(detail), size_text);
                } else {
                    strcpy(detail, "Unsupported filesystem ");
                    file_manager_append_string(detail, sizeof(detail), size_text);
                }
                file_manager_add_entry(name,
                                       detail,
                                       name,
                                       partition->sector_count,
                                       true,
                                       false,
                                       true,
                                       browsable,
                                       source_kind,
                                       index);
            }
        }
        return;
    }

    if (!file_manager_get_archive_view(boot_info, &archive, &archive_size)) {
        if (g_file_manager.source_kind == FILE_MANAGER_SOURCE_UNSUPPORTED_PARTITION) {
            file_manager_set_status("This partition needs a filesystem driver before files can be listed.");
        } else {
            file_manager_set_status("No archive is mounted for File Manager.");
        }
        return;
    }

    if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_DOWNLOADS) {
        file_manager_build_archive_path(g_file_manager.path, context.prefix, sizeof(context.prefix));
    } else if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_DOCUMENTS) {
        strcpy(g_file_manager.path, "Documents/");
        file_manager_build_archive_path(g_file_manager.path, context.prefix, sizeof(context.prefix));
    } else if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_MUSIC) {
        file_manager_build_archive_path(g_file_manager.path, context.prefix, sizeof(context.prefix));
        context.audio_only = true;
    } else if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_PHOTOS) {
        file_manager_build_archive_path(g_file_manager.path, context.prefix, sizeof(context.prefix));
        context.image_only = true;
    } else {
        file_manager_build_archive_path(g_file_manager.path, context.prefix, sizeof(context.prefix));
    }

    initrd_list_files(archive, archive_size, file_manager_scan_archive_callback, &context);

    if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_DOCUMENTS) {
        char detail[64];
        file_manager_format_file_size(detail, (u32)strlen(state->notepad_buffer));
        file_manager_add_entry("note.txt", detail, "Documents/note.txt", (u32)strlen(state->notepad_buffer), false, false, false, false, 0, 0);

        for (u32 i = 0; i < USER_FILES_MAX; ++i) {
            if (g_user_files.entries[i].active) {
                file_manager_format_file_size(detail, g_user_files.entries[i].size);
                char path[FILE_MANAGER_PATH_LENGTH];
                strcpy(path, "Documents/");
                strcat(path, g_user_files.entries[i].name);
                file_manager_add_entry(g_user_files.entries[i].name, detail, path, g_user_files.entries[i].size, false, false, false, false, 0, 0);
            }
        }
    }

    if (g_file_manager.entry_count == 0u) {
        file_manager_set_status("This location is empty.");
    }
}

static void file_manager_initialize(const desktop_state_t* state, const boot_info_t* boot_info) {
    memset(&g_file_manager, 0, sizeof(g_file_manager));
    g_file_manager.sidebar = FILE_MANAGER_SIDEBAR_THIS_PC;
    g_file_manager.show_this_pc_root = true;
    file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), "This PC");
    file_manager_set_status("Pick a partition or browse the live media.");
    if (boot_info != NULL && boot_info->initrd_start != 0u && boot_info->initrd_size != 0u) {
        (void)file_manager_open_source(state, boot_info, FILE_MANAGER_SOURCE_LIVE_MEDIA, 0u);
        g_file_manager.show_this_pc_root = true;
        file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), "This PC");
    }
    file_manager_refresh_entries(state, boot_info);
}

static void file_manager_refresh_entries(const desktop_state_t* state, const boot_info_t* boot_info);

static bool handle_context_menu_click(desktop_state_t* state, const boot_info_t* boot_info, u32 mouse_x, u32 mouse_y) {
    if (!state->context_menu_open) return false;

    u32 x = state->context_menu_x;
    u32 y = state->context_menu_y;
    u32 width = 160u;
    u32 item_height = 32u;
    u32 padding = 8u;

    u32 option_count;
    if (state->context_menu_target_entry == -1) {
        option_count = (state->context_menu_clipboard_entry != -1) ? 2 : 1;
    } else {
        option_count = 3;
    }

    u32 height = option_count * item_height + padding * 2;

    if (x + width > 1024) x = 1024 - width;
    if (y + height > 768) y = 768 - height;

    if (point_in_rect(mouse_x, mouse_y, x, y, width, height)) {
        u32 clicked_option = (mouse_y - y - padding) / item_height;
        if (clicked_option >= option_count) return false;

        if (state->context_menu_target_entry == -1) {
            if (clicked_option == 0) {
                // New File
                for (u32 i = 0; i < USER_FILES_MAX; ++i) {
                    if (!g_user_files.entries[i].active) {
                        strcpy(g_user_files.entries[i].name, "untitled.txt");
                        g_user_files.entries[i].lba = USER_FILES_DATA_START_LBA + i * USER_FILES_SECTORS_PER_FILE;
                        g_user_files.entries[i].size = 0;
                        g_user_files.entries[i].active = true;
                        user_files_save_table(state);
                        file_manager_refresh_entries(state, boot_info);
                        break;
                    }
                }
            } else if (clicked_option == 1) {
                // Paste
                if (state->context_menu_clipboard_entry >= 0 && state->context_menu_clipboard_entry < (s32)USER_FILES_MAX) {
                    u32 src_idx = (u32)state->context_menu_clipboard_entry;
                    if (g_user_files.entries[src_idx].active) {
                        for (u32 i = 0; i < USER_FILES_MAX; ++i) {
                            if (!g_user_files.entries[i].active) {
                                // Found a slot for the copy
                                strcpy(g_user_files.entries[i].name, "copy_of_");
                                strcat(g_user_files.entries[i].name, g_user_files.entries[src_idx].name);
                                g_user_files.entries[i].lba = USER_FILES_DATA_START_LBA + i * USER_FILES_SECTORS_PER_FILE;
                                g_user_files.entries[i].size = g_user_files.entries[src_idx].size;
                                g_user_files.entries[i].active = true;
                                
                                // Copy data on disk
                                u8 copy_buffer[USER_FILES_SECTORS_PER_FILE * 512];
                                if (storage_read_sectors(g_user_files.entries[src_idx].lba, USER_FILES_SECTORS_PER_FILE, copy_buffer)) {
                                    storage_write_sectors(g_user_files.entries[i].lba, USER_FILES_SECTORS_PER_FILE, copy_buffer);
                                }

                                user_files_save_table(state);
                                file_manager_refresh_entries(state, boot_info);
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            // Rename, Copy or Delete
            // target_entry 0 is note.txt, 1+ is user files
            if (state->context_menu_target_entry > 0) {
                u32 user_file_index = (u32)state->context_menu_target_entry - 1;
                // Find which active user file this corresponds to
                u32 active_count = 0;
                for (u32 i = 0; i < USER_FILES_MAX; ++i) {
                    if (g_user_files.entries[i].active) {
                        if (active_count == user_file_index) {
                            if (clicked_option == 0) {
                                // Rename
                                state->context_menu_renaming = true;
                                strcpy(state->context_menu_rename_buffer, g_user_files.entries[i].name);
                                return true;
                            } else if (clicked_option == 1) {
                                // Copy
                                state->context_menu_clipboard_entry = (s32)i;
                            } else {
                                // Delete
                                g_user_files.entries[i].active = false;
                                user_files_save_table(state);
                                file_manager_refresh_entries(state, boot_info);
                            }
                            break;
                        }
                        active_count++;
                    }
                }
            }
        }
        state->context_menu_open = false;
        return true;
    }

    state->context_menu_open = false;
    state->context_menu_renaming = false;
    return false;
}

static void draw_context_menu(const desktop_state_t* state, const mouse_state_t* mouse) {
    if (!state->context_menu_open) return;

    u32 x = state->context_menu_x;
    u32 y = state->context_menu_y;
    u32 width = 160u;
    u32 item_height = 32u;
    u32 padding = 8u;

    const char* options_empty_no_clip[] = {"New File"};
    const char* options_empty_with_clip[] = {"New File", "Paste"};
    const char* options_file[] = {"Rename", "Copy", "Delete"};

    const char** options;
    u32 option_count;

    if (state->context_menu_target_entry == -1) {
        if (state->context_menu_clipboard_entry != -1) {
            options = options_empty_with_clip;
            option_count = 2;
        } else {
            options = options_empty_no_clip;
            option_count = 1;
        }
    } else {
        options = options_file;
        option_count = 3;
    }

    u32 height = option_count * item_height + padding * 2;

    // Adjust position if it goes off screen
    if (x + width > 1024) x = 1024 - width;
    if (y + height > 768) y = 768 - height;

    draw_rounded_panel(x, y, width, height, 12u, ui_surface_color(state, 2u), ui_stroke_color(state, 3u));

    for (u32 i = 0; i < option_count; ++i) {
        u32 item_y = y + padding + i * item_height;
        bool hovered = point_in_rect(mouse->x, mouse->y, x, item_y, width, item_height);

        if (hovered) {
            draw_rounded_panel(x + 4u, item_y, width - 8u, item_height, 8u, ui_surface_color(state, 3u), 0);
        }

        draw_text_clipped(x + 12u, item_y + 8u, width - 24u, options[i], hovered ? ui_text_accent(state) : ui_text_primary(state));
    }

    if (state->context_menu_renaming) {
        // Draw a rename input box if renaming
        u32 rx = x;
        u32 ry = y + height + 4u;
        u32 rw = 200u;
        u32 rh = 40u;
        draw_rounded_panel(rx, ry, rw, rh, 12u, ui_surface_color(state, 1u), ui_stroke_color(state, 3u));
        draw_text_clipped(rx + 8u, ry + 12u, rw - 16u, state->context_menu_rename_buffer, ui_text_primary(state));
        // Add a blinking cursor effect
        if ((timer_ticks_ms() / 500) % 2 == 0) {
            u32 tw = (u32)strlen(state->context_menu_rename_buffer) * 8u;
            draw_rect(rx + 8u + tw, ry + 10u, 2u, 20u, ui_text_primary(state));
        }
    }
}

static void draw_file_manager_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;
    file_manager_layout_t layout;
    const char* sidebar_labels[5] = {"Downloads", "Boobies", "Music", "Photos", "This PC"};

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_FILES) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "File Manager", vga_color(214, 170, 88));
    if (shaded) {
        return;
    }

    if (!file_manager_get_layout(rect, &layout)) {
        return;
    }

    draw_rounded_panel(layout.nav_x, layout.nav_y, layout.nav_width, layout.body_height, WINDOW_CONTENT_RADIUS, ui_surface_color(state, 1u), ui_stroke_color(state, 0u));
    draw_text_clipped(layout.nav_x + 14u, layout.nav_y + 16u, layout.nav_width - 28u, "Browse", ui_text_secondary(state));
    for (u32 index = 0u; index < 5u; ++index) {
        bool active = g_file_manager.sidebar == index;
        draw_rounded_panel(layout.nav_x + 10u,
                           layout.nav_y + 46u + index * 42u,
                           layout.nav_width - 20u,
                           32u,
                           12u,
                           active ? ui_surface_color(state, 2u) : ui_surface_color(state, 1u),
                           active ? ui_stroke_color(state, 3u) : ui_stroke_color(state, 1u));
        draw_text_clipped(layout.nav_x + 22u,
                          layout.nav_y + 57u + index * 42u,
                          layout.nav_width - 42u,
                          sidebar_labels[index],
                          active ? ui_text_accent(state) : ui_text_primary(state));
    }

    draw_rounded_panel(layout.content_x, layout.body_y, layout.content_width, layout.body_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));
    draw_text_clipped(layout.content_x + 16u, layout.body_y + 18u, layout.content_width - 32u, g_file_manager.source_label, ui_text_secondary(state));
    draw_text_clipped(layout.content_x + 16u, layout.body_y + 42u, layout.content_width - 262u,
                      g_file_manager.path[0] != '\0' ? g_file_manager.path : "Root",
                      ui_text_primary(state));
    draw_secondary_button(state, layout.content_x + layout.content_width - 252u, layout.body_y + 14u, 70u, "Prev");
    draw_secondary_button(state, layout.content_x + layout.content_width - 174u, layout.body_y + 14u, 70u, "Next");
    draw_secondary_button(state, layout.content_x + layout.content_width - 96u, layout.body_y + 14u, 80u, "Up");

    for (u32 row = 0u; row < layout.rows_visible; ++row) {
        u32 entry_index = g_file_manager.scroll + row;
        file_manager_entry_t* entry;
        u32 row_y;

        if (entry_index >= g_file_manager.entry_count) {
            break;
        }
        entry = &g_file_manager.entries[entry_index];
        row_y = layout.rows_y + row * layout.row_height;
        draw_rounded_panel(layout.content_x + 14u,
                           row_y,
                           layout.content_width - 28u,
                           34u,
                           14u,
                           entry_index == g_file_manager.selected_entry ? ui_surface_color(state, 2u) : ui_surface_color(state, 1u),
                           entry_index == g_file_manager.selected_entry ? ui_stroke_color(state, 3u) : ui_stroke_color(state, 1u));
        draw_text_clipped(layout.content_x + 30u, row_y + 10u, layout.content_width - 180u, entry->name, ui_text_primary(state));
        draw_text_right_clipped(layout.content_x + 30u, row_y + 10u, layout.content_width - 60u, entry->detail, ui_text_secondary(state));
    }

    if (g_file_manager.entry_count == 0u) {
        draw_text_wrapped(layout.content_x + 18u,
                          layout.rows_y + 8u,
                          layout.content_width - 36u,
                          72u,
                          "There is nothing to show here yet.",
                          ui_text_muted(state));
    }

    draw_text_wrapped(layout.content_x + 16u,
                      layout.body_y + layout.body_height - 46u,
                      layout.content_width - 32u,
                      34u,
                      g_file_manager.status,
                      ui_text_muted(state));
}

static bool media_strip_assets_prefix(const char* name, char* out_name, u32 out_capacity) {
    const char* prefix = "assets/";
    u32 prefix_length = 7u;
    u32 name_length;

    if (name == NULL || out_name == NULL || out_capacity == 0u) {
        return false;
    }

    if (strncmp(name, prefix, prefix_length) != 0) {
        return false;
    }

    name_length = (u32)strlen(name + prefix_length);
    if (name_length + 1u > out_capacity) {
        return false;
    }

    memcpy(out_name, name + prefix_length, name_length + 1u);
    return true;
}

static u16 media_read_le16(const u8* data) {
    return (u16)((u16)data[0] | ((u16)data[1] << 8));
}

static u32 media_read_le32(const u8* data) {
    return (u32)data[0] |
           ((u32)data[1] << 8) |
           ((u32)data[2] << 16) |
           ((u32)data[3] << 24);
}

static char ascii_lower(char value) {
    if (value >= 'A' && value <= 'Z') {
        return (char)(value - 'A' + 'a');
    }
    return value;
}

static bool media_make_token_from_name(const char* name, char* out_token, u32 out_capacity) {
    u32 out_index = 0u;
    u32 last_dot = 0u;
    bool saw_dot = false;

    if (name == NULL || out_token == NULL || out_capacity < 2u) {
        return false;
    }

    for (u32 index = 0u; name[index] != '\0'; ++index) {
        if (name[index] == '.') {
            last_dot = index;
            saw_dot = true;
        }
    }

    u32 limit = saw_dot ? last_dot : (u32)strlen(name);
    for (u32 index = 0u; index < limit && out_index + 1u < out_capacity; ++index) {
        char value = ascii_lower(name[index]);

        if ((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9')) {
            out_token[out_index++] = value;
        } else if (value == ' ' || value == '-' || value == '_') {
            if (out_index != 0u && out_token[out_index - 1u] != '_') {
                out_token[out_index++] = '_';
            }
        }
    }

    if (out_index == 0u) {
        if (out_capacity < 6u) {
            return false;
        }
        strcpy(out_token, "media");
        return true;
    }

    if (out_token[out_index - 1u] == '_') {
        --out_index;
    }
    out_token[out_index] = '\0';
    return true;
}

static bool videos_parse_generated_frame_name(const char* name, char* out_token, u32 out_capacity, u32* out_index) {
    u32 prefix_length = 4u;
    u32 name_length;
    u32 marker_index = 0u;
    u32 token_length;
    u32 frame_index = 0u;

    if (name == NULL || out_index == NULL) {
        return false;
    }

    if (strncmp(name, "jmv_", prefix_length) != 0 || !string_has_extension(name, ".bmp")) {
        return false;
    }

    name_length = (u32)strlen(name);
    if (name_length <= prefix_length + 8u) {
        return false;
    }

    for (u32 index = prefix_length; index + 8u <= name_length; ++index) {
        if (name[index] == '_' &&
            name[index + 1u] >= '0' && name[index + 1u] <= '9' &&
            name[index + 2u] >= '0' && name[index + 2u] <= '9' &&
            name[index + 3u] >= '0' && name[index + 3u] <= '9' &&
            name[index + 4u] == '.' &&
            name[index + 5u] == 'b' &&
            name[index + 6u] == 'm' &&
            name[index + 7u] == 'p') {
            marker_index = index;
            break;
        }
    }

    if (marker_index == 0u) {
        return false;
    }

    token_length = marker_index - prefix_length;
    if (token_length == 0u) {
        return false;
    }

    if (out_token != NULL && out_capacity != 0u) {
        u32 copy_length = token_length;
        if (copy_length + 1u > out_capacity) {
            copy_length = out_capacity - 1u;
        }
        memcpy(out_token, name + prefix_length, copy_length);
        out_token[copy_length] = '\0';
    }

    for (u32 digit = 0u; digit < 3u; ++digit) {
        frame_index = frame_index * 10u + (u32)(name[marker_index + 1u + digit] - '0');
    }
    *out_index = frame_index;
    return true;
}

static void videos_reset_active_frames(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    state->videos_active_frame_count = 0u;
    memset(state->videos_active_frame_data, 0, sizeof(state->videos_active_frame_data));
    memset(state->videos_active_frame_size, 0, sizeof(state->videos_active_frame_size));
}

static void videos_reset_audio_state(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    state->videos_audio_data = NULL;
    state->videos_audio_sample_count = 0u;
    state->videos_audio_sample_rate = 0u;
    state->videos_audio_last_sample_index = 0xFFFFFFFFu;
}

static void videos_stop_playback(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    state->videos_playing = false;
    state->videos_play_start_tsc = 0u;
    state->videos_last_frame_index = 0u;
    state->videos_audio_last_sample_index = 0xFFFFFFFFu;
    if (state->videos_audio_use_sb16) {
        sb16_end_stream();
        state->videos_audio_use_sb16 = false;
    } else {
        pc_speaker_stop();
    }
}

static void startup_sound_reset(desktop_state_t* state) {
    if (state == NULL) {
        return;
    }

    state->startup_audio_data = NULL;
    state->startup_audio_sample_count = 0u;
    state->startup_audio_sample_rate = 0u;
    state->startup_audio_last_sample_index = 0xFFFFFFFFu;
    state->startup_audio_start_tsc = 0u;
    state->startup_audio_armed = false;
    state->startup_audio_playing = false;
    state->startup_audio_played = false;
}

static bool load_processed_audio_asset_file(const initrd_file_t* file,
                                            const u8** out_data,
                                            u32* out_sample_count,
                                            u32* out_sample_rate) {
    const audio_asset_header_t* header;

    if (file == NULL || file->data == NULL || out_data == NULL || out_sample_count == NULL || out_sample_rate == NULL) {
        return false;
    }
    if (file->size < sizeof(audio_asset_header_t)) {
        return false;
    }

    header = (const audio_asset_header_t*)file->data;
    if (memcmp(header->magic, AUDIO_ASSET_MAGIC, AUDIO_ASSET_MAGIC_LENGTH) != 0) {
        return false;
    }
    if (header->sample_rate == 0u || header->sample_rate > AUDIO_SAMPLE_MAX_RATE) {
        return false;
    }
    if (header->sample_count == 0u || header->sample_count > file->size - sizeof(audio_asset_header_t)) {
        return false;
    }

    *out_data = (const u8*)file->data + sizeof(audio_asset_header_t);
    *out_sample_count = header->sample_count;
    *out_sample_rate = header->sample_rate;
    return true;
}

static bool media_make_companion_name(const char* name, const char* extension, char* out_name, u32 out_capacity) {
    u32 name_length;
    u32 dot_index = 0u;
    bool found_dot = false;
    u32 extension_length;

    if (name == NULL || extension == NULL || out_name == NULL || out_capacity < 2u) {
        return false;
    }

    name_length = (u32)strlen(name);
    extension_length = (u32)strlen(extension);
    for (u32 index = 0u; index < name_length; ++index) {
        if (name[index] == '.') {
            dot_index = index;
            found_dot = true;
        }
    }

    if (!found_dot) {
        dot_index = name_length;
    }

    if (dot_index + extension_length + 1u > out_capacity) {
        return false;
    }

    memcpy(out_name, name, dot_index);
    memcpy(out_name + dot_index, extension, extension_length);
    out_name[dot_index + extension_length] = '\0';
    return true;
}

static bool startup_sound_load(desktop_state_t* state, const boot_info_t* boot_info) {
    char processed_name[128];
    initrd_file_t file;

    if (state == NULL || boot_info == NULL || boot_info->initrd_start == 0u || boot_info->initrd_size == 0u) {
        return false;
    }

    startup_sound_reset(state);
    if (!media_make_companion_name(STARTUP_SOUND_ASSET, ".jba", processed_name, sizeof(processed_name))) {
        return false;
    }
    if (!initrd_find_file((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, processed_name, &file)) {
        return false;
    }
    if (!load_processed_audio_asset_file(&file,
                                         &state->startup_audio_data,
                                         &state->startup_audio_sample_count,
                                         &state->startup_audio_sample_rate)) {
        return false;
    }

    state->startup_audio_last_sample_index = 0xFFFFFFFFu;
    state->startup_audio_armed = true;
    return true;
}

static bool startup_sound_start(desktop_state_t* state) {
    if (state == NULL ||
        state->startup_audio_played ||
        !state->startup_audio_armed ||
        state->startup_audio_data == NULL ||
        state->startup_audio_sample_count == 0u ||
        state->startup_audio_sample_rate == 0u ||
        state->cpu_clock_mhz == 0u) {
        return false;
    }

    state->startup_audio_start_tsc = cpu_read_tsc();
    state->startup_audio_last_sample_index = 0xFFFFFFFFu;
    state->startup_audio_playing = true;
    state->startup_audio_played = true;
    state->startup_audio_armed = false;
    return true;
}

static void startup_sound_tick(desktop_state_t* state) {
    u64 delta;
    u64 elapsed_us;
    u32 sample_index;

    if (state == NULL ||
        !state->startup_audio_playing ||
        state->cpu_clock_mhz == 0u ||
        state->startup_audio_data == NULL ||
        state->startup_audio_sample_count == 0u ||
        state->startup_audio_sample_rate == 0u) {
        return;
    }

    delta = cpu_read_tsc() - state->startup_audio_start_tsc;
    elapsed_us = delta / (u64)state->cpu_clock_mhz;
    sample_index = (u32)((elapsed_us * (u64)state->startup_audio_sample_rate) / 1000000ull);
    if (sample_index >= state->startup_audio_sample_count) {
        state->startup_audio_playing = false;
        pc_speaker_stop();
        return;
    }
    if (sample_index == state->startup_audio_last_sample_index) {
        return;
    }

    pc_speaker_play_pwm_sample(state->startup_audio_data[sample_index]);
    state->startup_audio_last_sample_index = sample_index;
}

typedef struct {
    desktop_state_t* state;
} videos_scan_context_t;

static bool videos_initrd_scan_callback(const initrd_file_t* file, void* user) {
    videos_scan_context_t* context = (videos_scan_context_t*)user;
    desktop_state_t* state;
    u32 frame_index;

    if (context == NULL || file == NULL) {
        return false;
    }

    state = context->state;
    if (state == NULL || file->name == NULL) {
        return true;
    }

    if (videos_parse_generated_frame_name(file->name, NULL, 0u, &frame_index)) {
        return true;
    }

    if (state->videos_item_count >= VIDEOS_MAX_ITEMS) {
        return true;
    }

    if (media_is_audio_file(file->name)) {
        state->videos_item_name[state->videos_item_count] = file->name;
        state->videos_item_type[state->videos_item_count] = MEDIA_ITEM_MP3;
        state->videos_item_frame_count[state->videos_item_count] = 0u;
        ++state->videos_item_count;
        return true;
    }

    return true;
}

typedef struct {
    desktop_state_t* state;
    char token[MEDIA_TOKEN_MAX];
} videos_collect_frames_context_t;

static bool videos_collect_generated_frames_callback(const initrd_file_t* file, void* user) {
    videos_collect_frames_context_t* context = (videos_collect_frames_context_t*)user;
    u32 frame_index;
    char token[MEDIA_TOKEN_MAX];

    if (context == NULL || context->state == NULL || file == NULL || file->name == NULL) {
        return true;
    }

    if (!videos_parse_generated_frame_name(file->name, token, sizeof(token), &frame_index)) {
        return true;
    }
    if (strcmp(token, context->token) != 0 || frame_index >= VIDEOS_MAX_FRAMES) {
        return true;
    }

    context->state->videos_active_frame_data[frame_index] = file->data;
    context->state->videos_active_frame_size[frame_index] = file->size;
    if (frame_index + 1u > context->state->videos_active_frame_count) {
        context->state->videos_active_frame_count = frame_index + 1u;
    }
    return true;
}

static bool videos_load_processed_audio_asset(desktop_state_t* state, const initrd_file_t* file) {
    if (state == NULL || file == NULL || file->data == NULL) {
        return false;
    }

    if (!load_processed_audio_asset_file(file,
                                         &state->videos_audio_data,
                                         &state->videos_audio_sample_count,
                                         &state->videos_audio_sample_rate)) {
        return false;
    }

    state->videos_audio_last_sample_index = 0xFFFFFFFFu;
    return true;
}

static bool videos_try_load_processed_audio(desktop_state_t* state, const boot_info_t* boot_info, const char* item_name) {
    char processed_name[128];
    char legacy_name[128];
    initrd_file_t file;

    if (state == NULL || boot_info == NULL || item_name == NULL) {
        return false;
    }

    if (!media_make_companion_name(item_name, ".jba", processed_name, sizeof(processed_name))) {
        return false;
    }
    if (initrd_find_file((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, processed_name, &file)) {
        return videos_load_processed_audio_asset(state, &file);
    }

    if (media_strip_assets_prefix(processed_name, legacy_name, sizeof(legacy_name)) &&
        initrd_find_file((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, legacy_name, &file)) {
        return videos_load_processed_audio_asset(state, &file);
    }

    return false;
}

static bool videos_try_load_wav_audio(desktop_state_t* state, const boot_info_t* boot_info, const char* item_name) {
    initrd_file_t file;
    const u8* bytes;
    u32 offset = 12u;
    const u8* data_chunk = NULL;
    u32 data_chunk_size = 0u;
    u16 audio_format = 0u;
    u16 channel_count = 0u;
    u16 bits_per_sample = 0u;
    u32 sample_rate = 0u;
    bool have_format = false;

    if (state == NULL || boot_info == NULL || item_name == NULL) {
        return false;
    }

    if (!initrd_find_file((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, item_name, &file)) {
        return false;
    }
    if (file.size < 44u) {
        return false;
    }

    bytes = (const u8*)file.data;
    if (memcmp(bytes, "RIFF", 4u) != 0 || memcmp(bytes + 8u, "WAVE", 4u) != 0) {
        return false;
    }

    while (offset + 8u <= file.size) {
        u32 chunk_size = media_read_le32(bytes + offset + 4u);
        u32 chunk_data_offset = offset + 8u;
        u32 padded_chunk_size = chunk_size + (chunk_size & 1u);

        if (chunk_data_offset > file.size || chunk_size > file.size - chunk_data_offset) {
            return false;
        }

        if (memcmp(bytes + offset, "fmt ", 4u) == 0) {
            if (chunk_size < 16u) {
                return false;
            }
            audio_format = media_read_le16(bytes + chunk_data_offset);
            channel_count = media_read_le16(bytes + chunk_data_offset + 2u);
            sample_rate = media_read_le32(bytes + chunk_data_offset + 4u);
            bits_per_sample = media_read_le16(bytes + chunk_data_offset + 14u);
            have_format = true;
        } else if (memcmp(bytes + offset, "data", 4u) == 0) {
            data_chunk = bytes + chunk_data_offset;
            data_chunk_size = chunk_size;
        }

        if (padded_chunk_size > file.size - chunk_data_offset) {
            return false;
        }
        offset = chunk_data_offset + padded_chunk_size;
    }

    if (!have_format || data_chunk == NULL || data_chunk_size == 0u) {
        return false;
    }

    if (audio_format != 1u || channel_count != 1u || bits_per_sample != 8u) {
        return false;
    }
    if (sample_rate == 0u || sample_rate > AUDIO_SAMPLE_MAX_RATE) {
        return false;
    }

    state->videos_audio_data = data_chunk;
    state->videos_audio_sample_count = data_chunk_size;
    state->videos_audio_sample_rate = sample_rate;
    state->videos_audio_last_sample_index = 0xFFFFFFFFu;
    return true;
}

[[maybe_unused]] static bool videos_prepare_selected_media(desktop_state_t* state, const boot_info_t* boot_info) {
    media_item_type_t type;

    if (state == NULL || state->videos_item_count == 0u || state->videos_selected_item >= state->videos_item_count) {
        return false;
    }

    videos_reset_active_frames(state);
    videos_reset_audio_state(state);
    type = state->videos_item_type[state->videos_selected_item];

    if (boot_info == NULL || boot_info->initrd_start == 0u || boot_info->initrd_size == 0u) {
        return false;
    }

    if (type == MEDIA_ITEM_MP3) {
        const char* item_name = state->videos_item_name[state->videos_selected_item];

        if (media_is_wav_file(item_name) && videos_try_load_wav_audio(state, boot_info, item_name)) {
            return true;
        }
        return videos_try_load_processed_audio(state, boot_info, item_name);
    }

    if (type == MEDIA_ITEM_MP4) {
        videos_collect_frames_context_t context = {{0}, ""};
        context.state = state;
        if (!media_make_token_from_name(state->videos_item_name[state->videos_selected_item], context.token, sizeof(context.token))) {
            return false;
        }
        initrd_list_files((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, videos_collect_generated_frames_callback, &context);
        return state->videos_active_frame_count != 0u;
    }

    if (type == MEDIA_ITEM_IMAGE) {
        for (u32 index = 0u; index < state->videos_item_count && state->videos_active_frame_count < VIDEOS_MAX_FRAMES; ++index) {
            initrd_file_t file;

            if (state->videos_item_type[index] != MEDIA_ITEM_IMAGE) {
                continue;
            }
            if (!initrd_find_file((const void*)boot_info->initrd_start,
                                  (u32)boot_info->initrd_size,
                                  state->videos_item_name[index],
                                  &file)) {
                continue;
            }

            state->videos_active_frame_data[state->videos_active_frame_count] = file.data;
            state->videos_active_frame_size[state->videos_active_frame_count] = file.size;
            ++state->videos_active_frame_count;
        }
        return state->videos_active_frame_count != 0u;
    }

    return false;
}

static u32 videos_current_audio_sample_index(const desktop_state_t* state) {
    u64 delta;
    u64 elapsed_us;

    if (state == NULL ||
        !state->videos_playing ||
        state->cpu_clock_mhz == 0u ||
        state->videos_audio_data == NULL ||
        state->videos_audio_sample_rate == 0u) {
        return 0u;
    }

    delta = cpu_read_tsc() - state->videos_play_start_tsc;
    elapsed_us = delta / (u64)state->cpu_clock_mhz;
    return (u32)((elapsed_us * (u64)state->videos_audio_sample_rate) / 1000000ull);
}

static void videos_audio_tick(desktop_state_t* state) {
    u32 sample_index;
    u32 next_sample_index;

    if (state == NULL ||
        !state->videos_playing ||
        state->videos_item_count == 0u ||
        state->videos_selected_item >= state->videos_item_count ||
        state->videos_item_type[state->videos_selected_item] != MEDIA_ITEM_MP3) {
        return;
    }

    if (state->videos_audio_data == NULL || state->videos_audio_sample_count == 0u || state->videos_audio_sample_rate == 0u) {
        videos_stop_playback(state);
        return;
    }

    sample_index = videos_current_audio_sample_index(state);
    if (sample_index >= state->videos_audio_sample_count) {
        videos_stop_playback(state);
        return;
    }
    if (sample_index == state->videos_audio_last_sample_index) {
        return;
    }

    if (state->videos_audio_use_sb16) {
        next_sample_index = state->videos_audio_last_sample_index == 0xFFFFFFFFu
                                ? 0u
                                : state->videos_audio_last_sample_index + 1u;

        if (next_sample_index < sample_index &&
            sample_index - next_sample_index + 1u > AUDIO_SB16_CATCHUP_LIMIT) {
            /* Bound catch-up work so audio recovery does not stall the desktop loop. */
            next_sample_index = sample_index - AUDIO_SB16_CATCHUP_LIMIT + 1u;
        }

        while (next_sample_index <= sample_index) {
            if (!sb16_play_sample(state->videos_audio_data[next_sample_index])) {
                state->videos_audio_use_sb16 = false;
                pc_speaker_play_pwm_sample(state->videos_audio_data[sample_index]);
                break;
            }
            ++next_sample_index;
        }
    } else {
        pc_speaker_play_pwm_sample(state->videos_audio_data[sample_index]);
    }
    state->videos_audio_last_sample_index = sample_index;
}

static void videos_refresh_media_list(desktop_state_t* state, const boot_info_t* boot_info) {
    if (state == NULL) {
        return;
    }

    state->videos_item_count = 0u;
    state->videos_selected_item = 0u;
    videos_stop_playback(state);
    videos_reset_active_frames(state);
    videos_reset_audio_state(state);

    if (boot_info == NULL || boot_info->initrd_start == 0u || boot_info->initrd_size == 0u) {
        return;
    }

    videos_scan_context_t context = {state};
    initrd_list_files((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, videos_initrd_scan_callback, &context);
}

[[maybe_unused]] static const char* videos_type_label(media_item_type_t type) {
    switch (type) {
        case MEDIA_ITEM_MP3:
            return "Audio";
        case MEDIA_ITEM_IMAGE:
        case MEDIA_ITEM_MP4:
        default:
            return "";
    }
}

static u32 videos_current_frame_index(const desktop_state_t* state) {
    u32 frame_time_us = 16666u;
    u64 elapsed_us;
    u64 delta;
    u32 index;
    u32 count = 0u;

    if (state == NULL || !state->videos_playing || state->cpu_clock_mhz == 0u) {
        return 0u;
    }

    delta = cpu_read_tsc() - state->videos_play_start_tsc;
    elapsed_us = delta / (u64)state->cpu_clock_mhz;
    index = (u32)(elapsed_us / (u64)frame_time_us);

    if (state->videos_item_count == 0u || state->videos_selected_item >= state->videos_item_count) {
        return 0u;
    }

    if (state->videos_item_type[state->videos_selected_item] == MEDIA_ITEM_IMAGE) {
        frame_time_us = 2000000u;
    }

    if (state->videos_active_frame_count != 0u) {
        count = state->videos_active_frame_count;
    } else {
        count = state->videos_item_frame_count[state->videos_selected_item];
    }
    if (count == 0u) {
        return 0u;
    }
    return index % count;
}

[[maybe_unused]] static u32 videos_display_frame_index(const desktop_state_t* state) {
    u32 count = 0u;

    if (state == NULL) {
        return 0u;
    }

    if (state->videos_playing) {
        return videos_current_frame_index(state);
    }

    if (state->videos_active_frame_count != 0u) {
        count = state->videos_active_frame_count;
    } else if (state->videos_item_count != 0u && state->videos_selected_item < state->videos_item_count) {
        count = state->videos_item_frame_count[state->videos_selected_item];
    }

    if (count == 0u) {
        return 0u;
    }
    return state->videos_last_frame_index % count;
}

static bool videos_get_preview_present_rect(const desktop_state_t* state, window_rect_t* out_rect) {
    const window_rect_t* rect;
    u32 body_x;
    u32 body_y;
    u32 body_width;
    u32 body_height;
    u32 list_width = 220u;
    u32 controls_height = 54u;
    u32 content_x;
    u32 content_width;
    u32 content_y;
    u32 preview_x;
    u32 preview_y;
    u32 preview_width;
    u32 preview_height;

    if (state == NULL || out_rect == NULL || !window_visible(state, WINDOW_VIDEOS) || window_shaded(state, WINDOW_VIDEOS)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_VIDEOS);
    if (rect == NULL) {
        return false;
    }

    body_x = rect->x + 18u;
    body_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 16u;
    body_width = safe_subtract(rect->width, 36u);
    body_height = safe_subtract(rect->height, WINDOW_TITLEBAR_HEIGHT + 34u);
    content_x = body_x + 12u + list_width + 16u;
    content_width = safe_subtract(body_width, (content_x - body_x) + 12u);
    content_y = body_y + 12u;
    preview_x = content_x;
    preview_y = content_y + 62u;
    preview_width = content_width;
    preview_height = safe_subtract(body_height, controls_height + 48u);

    out_rect->x = preview_x;
    out_rect->y = preview_y;
    out_rect->width = preview_width;
    out_rect->height = preview_height;
    return out_rect->width != 0u && out_rect->height != 0u;
}

static void draw_videos_window(const desktop_state_t* state, const window_rect_t* rect, const boot_info_t* boot_info, bool shaded) {
    u32 frame_height;
    u32 body_x;
    u32 body_y;
    u32 body_width;
    u32 body_height;
    u32 preview_x;
    u32 preview_y;
    u32 preview_width;
    u32 preview_height;
    char size_text[FILE_MANAGER_DETAIL_LENGTH];

    (void)boot_info;

    if (g_file_manager.audio_error_popup) {
        draw_development_placeholder_window(state, rect, shaded, WINDOW_VIDEOS, "Media Error", vga_color(220, 60, 40), "Unknown File Type", "Try again later");
        return;
    }
    if (g_file_manager.preview_name[0] == '\0') {
        draw_development_placeholder_window(state, rect, shaded, WINDOW_VIDEOS, "jabulmedia", vga_color(86, 118, 226), "Coming soon", "Very soon");
        return;
    }
    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_VIDEOS) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "File Preview", vga_color(86, 118, 226));
    if (shaded) {
        return;
    }

    body_x = rect->x + 18u;
    body_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 16u;
    body_width = safe_subtract(rect->width, 36u);
    body_height = safe_subtract(rect->height, WINDOW_TITLEBAR_HEIGHT + 34u);
    preview_x = body_x + 16u;
    preview_y = body_y + 82u;
    preview_width = safe_subtract(body_width, 32u);
    preview_height = safe_subtract(body_height, 134u);
    file_manager_format_file_size(size_text, g_file_manager.preview_size);

    draw_rounded_panel(body_x, body_y, body_width, body_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));
    draw_text_clipped(body_x + 18u, body_y + 18u, safe_subtract(body_width, 180u), g_file_manager.preview_name, ui_text_primary(state));
    draw_text_clipped(body_x + 18u, body_y + 42u, safe_subtract(body_width, 180u), size_text, ui_text_secondary(state));
    draw_secondary_button(state, body_x + body_width - 126u, body_y + 14u, 110u, "Back");
    draw_rounded_panel(preview_x, preview_y, preview_width, preview_height, 18u,
                       ui_surface_color(state, 1u), ui_stroke_color(state, 1u));

    if (g_file_manager.preview_data != NULL && g_file_manager.preview_size != 0u) {
        image_t preview_image;

        if (image_load_any(g_file_manager.preview_data, g_file_manager.preview_size, &preview_image)) {
            image_blit_fit_rounded(&preview_image, preview_x + 8u, preview_y + 8u, safe_subtract(preview_width, 16u), safe_subtract(preview_height, 16u), 14u);
        } else {
            draw_text_wrapped(preview_x + 18u,
                              preview_y + 18u,
                              safe_subtract(preview_width, 36u),
                              safe_subtract(preview_height, 36u),
                              "This file opened in preview, but this image format is not decoded yet. BMP and TGA previews work today.",
                              ui_text_muted(state));
        }
    } else {
        draw_text_wrapped(preview_x + 18u,
                          preview_y + 18u,
                          safe_subtract(preview_width, 36u),
                          safe_subtract(preview_height, 36u),
                          "Nothing is loaded in the preview window yet.",
                          ui_text_muted(state));
    }
}

static bool handle_videos_click(desktop_state_t* state, const boot_info_t* boot_info, u32 mouse_x, u32 mouse_y) {
    (void)boot_info;

    if (state == NULL || g_file_manager.preview_name[0] == '\0' || !window_visible(state, WINDOW_VIDEOS) || window_shaded(state, WINDOW_VIDEOS)) {
        return false;
    }

    const window_rect_t* rect = get_window_rect_const(state, WINDOW_VIDEOS);
    if (rect == NULL) {
        return false;
    }

    if (point_in_rect(mouse_x, mouse_y, rect->x + rect->width - 144u, rect->y + WINDOW_TITLEBAR_HEIGHT + 30u, 110u, 34u)) {
        file_manager_close_preview(state);
        return true;
    }

    return false;
}

static void draw_browser_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;
    u32 content_y;
    u32 content_height;
    u32 accent = vga_color(246, 132, 88);

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_BROWSER) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Jachron Browser", accent);
    if (shaded) {
        return;
    }

    // Toolbar area (matches hitboxes in handle_browser_click)
    draw_rounded_panel(rect->x + 12u, rect->y + 48u, rect->width - 24u, 44u, 12u, ui_surface_color(state, 1u), ui_stroke_color(state, 0u));
    
    // Address Bar
    draw_rounded_panel(rect->x + 20u, rect->y + 54u, rect->width - 210u, 32u, 8u, ui_surface_color(state, 0u), ui_stroke_color(state, 1u));
    draw_text_clipped(rect->x + 30u, rect->y + 62u, rect->width - 230u, g_browser.address[0] != '\0' ? g_browser.address : "Enter URL or search...", ui_text_primary(state));

    // Buttons
    draw_oobe_button(state, rect->x + rect->width - 182u, rect->y + 54u, 72u, "Go", false);
    draw_secondary_button(state, rect->x + rect->width - 100u, rect->y + 54u, 82u, "Save");

    content_y = rect->y + 104u;
    content_height = safe_subtract(rect->height, (content_y - rect->y) + 40u);
    draw_rounded_panel(rect->x + 18u, content_y, rect->width - 36u, content_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));

    // Render Content
    const char* content = (state->browser_page == BROWSER_PAGE_DOWNLOADS) ? g_browser.download_text : g_browser.page_text;

    if (content[0] != '\0') {
        draw_text_wrapped(rect->x + 34u, content_y + 18u, rect->width - 68u, content_height - 36u, content, ui_text_primary(state));
    }

    // Status Bar
    u32 status_y = rect->y + rect->height - 34u;
    draw_text_clipped(rect->x + 24u, status_y + 8u, rect->width - 48u, g_browser.status, ui_text_muted(state));
}

static void draw_games_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    u32 frame_height;
    u32 body_y;
    u32 side_text_width;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_GAMES) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Games", vga_color(198, 156, 255));
    if (shaded) {
        return;
    }

    draw_settings_tab(state, rect->x + 20u, rect->y + 50u, "Arcade", state->games_screen == GAMES_SCREEN_HOME);
    draw_settings_tab(state, rect->x + 154u, rect->y + 50u, "Snake", state->games_screen == GAMES_SCREEN_SNAKE);
    draw_settings_tab(state, rect->x + 288u, rect->y + 50u, "TicTacToe", state->games_screen == GAMES_SCREEN_TICTACTOE);
    body_y = rect->y + 92u;
    side_text_width = safe_subtract(rect->width, 312u);
    draw_rounded_panel(rect->x + 18u, body_y, rect->width - 36u, rect->height - 110u, WINDOW_CONTENT_RADIUS,
                       vga_color(250, 248, 252), vga_color(224, 214, 240));

    if (state->games_screen == GAMES_SCREEN_HOME) {
        draw_text_clipped(rect->x + 34u, body_y + 18u, rect->width - 68u, "Arcade", vga_color(84, 62, 108));
        draw_rounded_rect(rect->x + 34u, body_y + 48u, 170u, 92u, 18u, vga_color(234, 252, 242));
        draw_rect_outline(rect->x + 34u, body_y + 48u, 170u, 92u, 1u, vga_color(184, 224, 206));
        draw_text_clipped(rect->x + 54u, body_y + 74u, 130u, "Snake", vga_color(50, 82, 64));
        draw_text_wrapped(rect->x + 54u, body_y + 94u, 130u, 28u, "Open standalone window", vga_color(106, 150, 122));
        draw_rounded_rect(rect->x + 224u, body_y + 48u, 170u, 92u, 18u, vga_color(244, 236, 255));
        draw_rect_outline(rect->x + 224u, body_y + 48u, 170u, 92u, 1u, vga_color(218, 198, 244));
        draw_text_clipped(rect->x + 244u, body_y + 74u, 130u, "Tic-Tac-Toe", vga_color(84, 62, 108));
        draw_text_wrapped(rect->x + 244u, body_y + 94u, 130u, 28u, "Click a square to play", vga_color(148, 122, 176));
    } else if (state->games_screen == GAMES_SCREEN_SNAKE) {
        char score[16];
        u32 grid_x = rect->x + 34u;
        u32 grid_y = body_y + 24u;

        append_uint(score, state->snake_score);
        draw_text_clipped(rect->x + 34u, body_y + 6u, 68u, "Snake", vga_color(50, 82, 64));
        draw_text_clipped(rect->x + 112u, body_y + 6u, 54u, score, vga_color(84, 150, 106));
        draw_oobe_button(state, rect->x + rect->width - 146u, body_y + 4u, 112u, "Reset", false);

        for (u32 y = 0; y < SNAKE_GRID_HEIGHT; ++y) {
            for (u32 x = 0; x < SNAKE_GRID_WIDTH; ++x) {
                u32 cell_x = grid_x + x * SNAKE_CELL_SIZE;
                u32 cell_y = grid_y + y * SNAKE_CELL_SIZE;
                u32 fill = ((x + y) & 1u) == 0u ? vga_color(238, 252, 242) : vga_color(226, 246, 232);

                draw_rect(cell_x, cell_y, SNAKE_CELL_SIZE - 1u, SNAKE_CELL_SIZE - 1u, fill);
                if (state->snake_food_x == x && state->snake_food_y == y) {
                    draw_rounded_rect(cell_x + 4u, cell_y + 4u, 9u, 9u, 4u, vga_color(244, 110, 110));
                }
                for (u32 index = 0; index < state->snake_length; ++index) {
                    if (state->snake_x[index] == x && state->snake_y[index] == y) {
                        draw_rounded_rect(cell_x + 2u, cell_y + 2u, 13u, 13u, 4u,
                                          index == 0u ? vga_color(50, 164, 90) : vga_color(92, 204, 132));
                    }
                }
            }
        }
        draw_text_wrapped(rect->x + 278u, body_y + 42u, side_text_width, 24u, "Use arrow keys", vga_color(106, 150, 122));
        draw_text_wrapped(rect->x + 278u, body_y + 66u, side_text_width, 24u, "to move", vga_color(106, 150, 122));
        draw_text_wrapped(rect->x + 278u, body_y + 96u, side_text_width, 40u,
                          state->snake_game_over ? "Game over. Reset to try again." : "Get the highest score!",
                          state->snake_game_over ? vga_color(190, 74, 74) : vga_color(84, 120, 96));
    } else {
        const char* status = state->ttt_winner == 1u ? "Player X wins" :
                             state->ttt_winner == 2u ? "Player O wins" :
                             state->ttt_draw ? "Draw game" :
                             state->ttt_turn == 1u ? "Player X turn" : "Player O turn";
        u32 board_x = rect->x + 42u;
        u32 board_y = body_y + 24u;

        draw_text_clipped(rect->x + 34u, body_y + 6u, 132u, "Tic-Tac-Toe", vga_color(84, 62, 108));
        draw_text_clipped(rect->x + 182u, body_y + 6u, rect->width - 364u, status, vga_color(148, 122, 176));
        draw_oobe_button(state, rect->x + rect->width - 146u, body_y + 4u, 112u, "Reset", false);

        for (u32 index = 0; index < 9u; ++index) {
            u32 col = index % 3u;
            u32 row = index / 3u;
            u32 cell_x = board_x + col * 66u;
            u32 cell_y = board_y + row * 66u;

            draw_rounded_rect(cell_x, cell_y, 56u, 56u, 12u, vga_color(244, 236, 255));
            draw_rect_outline(cell_x, cell_y, 56u, 56u, 1u, vga_color(214, 196, 238));
            if (state->ttt_cells[index] == 1u) {
                draw_rect(cell_x + 14u, cell_y + 14u, 26u, 4u, vga_color(84, 62, 108));
                draw_rect(cell_x + 14u, cell_y + 38u, 26u, 4u, vga_color(84, 62, 108));
                draw_rect(cell_x + 14u, cell_y + 18u, 4u, 20u, vga_color(84, 62, 108));
                draw_rect(cell_x + 36u, cell_y + 18u, 4u, 20u, vga_color(84, 62, 108));
            } else if (state->ttt_cells[index] == 2u) {
                draw_rounded_rect(cell_x + 14u, cell_y + 14u, 26u, 26u, 13u, vga_color(104, 224, 172));
                draw_rounded_rect(cell_x + 20u, cell_y + 20u, 14u, 14u, 7u, vga_color(244, 236, 255));
            }
        }
    }
}

static void draw_snake_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    char score[16];
    u32 frame_height;
    u32 body_y;
    u32 grid_x;
    u32 grid_y;
    u32 side_text_width;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_SNAKE) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Snake", vga_color(72, 188, 112));
    if (shaded) {
        return;
    }

    body_y = rect->y + 54u;
    side_text_width = safe_subtract(rect->width, 312u);
    draw_rounded_panel(rect->x + 18u, body_y, rect->width - 36u, rect->height - 72u, WINDOW_CONTENT_RADIUS,
                       vga_color(250, 248, 252), vga_color(224, 214, 240));

    append_uint(score, state->snake_score);
    draw_text_clipped(rect->x + 34u, body_y + 12u, 42u, "Score", vga_color(84, 120, 96));
    draw_text_clipped(rect->x + 86u, body_y + 12u, 60u, score, vga_color(50, 164, 90));
    draw_oobe_button(state, rect->x + rect->width - 146u, body_y + 8u, 112u, "Reset", false);

    grid_x = rect->x + 34u;
    grid_y = body_y + 42u;
    for (u32 y = 0; y < SNAKE_GRID_HEIGHT; ++y) {
        for (u32 x = 0; x < SNAKE_GRID_WIDTH; ++x) {
            u32 cell_x = grid_x + x * SNAKE_CELL_SIZE;
            u32 cell_y = grid_y + y * SNAKE_CELL_SIZE;
            u32 fill = ((x + y) & 1u) == 0u ? vga_color(238, 252, 242) : vga_color(226, 246, 232);

            draw_rect(cell_x, cell_y, SNAKE_CELL_SIZE - 1u, SNAKE_CELL_SIZE - 1u, fill);
            if (state->snake_food_x == x && state->snake_food_y == y) {
                draw_rounded_rect(cell_x + 4u, cell_y + 4u, 9u, 9u, 4u, vga_color(244, 110, 110));
            }
            for (u32 index = 0; index < state->snake_length; ++index) {
                if (state->snake_x[index] == x && state->snake_y[index] == y) {
                    draw_rounded_rect(cell_x + 2u, cell_y + 2u, 13u, 13u, 4u,
                                      index == 0u ? vga_color(50, 164, 90) : vga_color(92, 204, 132));
                }
            }
        }
    }

    draw_text_wrapped(rect->x + 278u, body_y + 54u, side_text_width, 24u, "Use arrow keys", vga_color(106, 150, 122));
    draw_text_wrapped(rect->x + 278u, body_y + 78u, side_text_width, 24u, "To move.", vga_color(106, 150, 122));
    draw_text_wrapped(rect->x + 278u, body_y + 104u, side_text_width, 24u, "Enter to reset", vga_color(84, 120, 96));
    draw_text_wrapped(rect->x + 278u, body_y + 132u, side_text_width, 40u,
                      state->snake_game_over ? "Game over. Reset to try again." : "Get the highest score!",
                      state->snake_game_over ? vga_color(190, 74, 74) : vga_color(84, 120, 96));
}

static void draw_jabver_window(const desktop_state_t* state, const window_rect_t* rect, bool shaded) {
    char footer_text[256];
    u32 frame_height;
    u32 panel_x;
    u32 panel_y;
    u32 panel_width;
    u32 panel_height;
    u32 logo_x;
    u32 logo_y;
    u32 text_width;
    const char* username;

    if (rect == NULL || state == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_JABVER) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "jabver", vga_color(84, 118, 226));
    if (shaded) {
        return;
    }

    panel_x = rect->x + 16u;
    panel_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 16u;
    panel_width = rect->width - 32u;
    panel_height = rect->height - WINDOW_TITLEBAR_HEIGHT - 32u;
    logo_x = panel_x + 18u;
    logo_y = panel_y + 18u;
    text_width = panel_width > 36u ? panel_width - 36u : 0u;
    username = state->account_username[0] != '\0' ? state->account_username : "User";

    footer_text[0] = '\0';
    append_text(footer_text, "Thank you for choosing JabulOS Aero, However you must accept our licenses and agreements for the JabulOS Community, And you must admire Froxy for his work, Yours ");
    append_text(footer_text + strlen(footer_text), username);

    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, WINDOW_CONTENT_RADIUS,
                       vga_color(250, 251, 255), vga_color(212, 220, 238));
    
    if (state->splash_logo_image != NULL) {
        image_blit_fit_rounded_transparent(state->splash_logo_image, logo_x, logo_y, 80u, 72u, 0u, 0x000000);
    } else if (state->boot_splash_image != NULL) {
        image_blit_fit_rounded_transparent(state->boot_splash_image, logo_x, logo_y, 80u, 72u, 0u, 0x000000);
    } else {
        draw_rounded_rect(logo_x + 4u, logo_y + 4u, 72u, 64u, 16u, vga_color(90, 132, 246));
        draw_text(logo_x + 36u, logo_y + 28u, "J", vga_color(255, 255, 255));
    }

    draw_text_boldish(panel_x + 112u, panel_y + 20u, panel_width - 130u, "JabulOS Aero", vga_color(42, 54, 92));
    draw_text_clipped(panel_x + 112u, panel_y + 44u, panel_width - 130u, "Insider Release", vga_color(82, 96, 152));
    draw_text_clipped(panel_x + 18u, panel_y + 92u, text_width, " ", vga_color(68, 76, 108));
    draw_text_boldish(panel_x + 18u, panel_y + 118u, text_width, "Build 30000", vga_color(78, 84, 188));
    draw_text_wrapped(panel_x + 18u,
                      panel_y + 188u,
                      text_width,
                      120u,
                      footer_text,
                      vga_color(92, 98, 126));
    draw_text_clipped(panel_x + 18u, panel_y + panel_height - 28u, text_width,
                      "Copyright (c) 2026 JabulOS Aero", vga_color(112, 118, 146));
}

static void draw_task_manager_window(const boot_info_t* boot_info,
                                     bool wallpaper_loaded,
                                     bool ata_ready,
                                     const desktop_state_t* state,
                                     const window_rect_t* rect,
                                     bool shaded) {
    char header_line_one[25];
    char header_line_two[25];
    char value_text[40];
    char info_text[48];
    char secondary_text[48];
    char tertiary_text[48];
    char quaternary_text[48];
    char total_text[24];
    char used_text[24];
    char free_text[24];
    char runtime_text[24];
    u32 frame_height;
    u32 nav_x;
    u32 nav_y;
    u32 nav_width;
    u32 nav_height;
    u32 content_x;
    u32 content_y;
    u32 content_width;
    u32 card_x;
    u32 card_y;
    u32 card_width;
    u32 cpu_percent;
    u32 ram_percent;
    u32 storage_percent;
    u32 storage_benchmark;
    u64 total_memory;
    u64 free_memory;
    u64 used_memory;

    (void)boot_info;
    (void)wallpaper_loaded;
    (void)ata_ready;

    if (state == NULL || rect == NULL) {
        return;
    }

    frame_height = shaded ? shaded_window_height(WINDOW_TASKS) : rect->height;
    draw_window_frame(state, rect->x, rect->y, rect->width, frame_height, "Task Manager", vga_color(142, 112, 214));
    if (shaded) {
        return;
    }

    nav_width = 180u;
    nav_x = rect->x + 18u;
    nav_y = rect->y + 56u;
    nav_height = rect->height > 74u ? rect->height - 74u : 0u;
    draw_rounded_panel(nav_x, nav_y, nav_width, nav_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 1u), ui_stroke_color(state, 0u));
    draw_text_clipped(nav_x + 14u, nav_y + 16u, nav_width - 28u, "Performance", ui_text_secondary(state));
    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 48u,
                           nav_width - 20u,
                           "Cpu Usage",
                           state->task_manager_page == TASK_MANAGER_PAGE_CPU);
    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 88u,
                           nav_width - 20u,
                           "Ram",
                           state->task_manager_page == TASK_MANAGER_PAGE_RAM);
    draw_settings_nav_item(state,
                           nav_x + 10u,
                           nav_y + 128u,
                           nav_width - 20u,
                           "Storage",
                           state->task_manager_page == TASK_MANAGER_PAGE_STORAGE);

    content_x = nav_x + nav_width + 16u;
    content_y = nav_y;
    content_width = rect->width > (content_x - rect->x + 18u) ? rect->width - (content_x - rect->x) - 18u : 0u;
    draw_rounded_panel(content_x, content_y, content_width, nav_height, WINDOW_CONTENT_RADIUS,
                       ui_surface_color(state, 0u), ui_stroke_color(state, 0u));

    card_x = content_x + 16u;
    card_y = content_y + 56u;
    card_width = safe_subtract(content_width, 32u);
    total_memory = pmm_total_memory();
    free_memory = pmm_free_memory();
    used_memory = total_memory > free_memory ? total_memory - free_memory : 0u;
    cpu_percent = task_manager_cpu_usage_percent(state);
    ram_percent = total_memory != 0u ? (u32)((used_memory * 100u) / total_memory) : 0u;
    storage_benchmark = task_manager_storage_write_benchmark_kib(state);
    storage_percent = task_manager_storage_usage_percent(state);

    if (state->task_manager_page == TASK_MANAGER_PAGE_RAM) {
        draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "Ram", ui_text_secondary(state));
        format_megabytes(total_text, total_memory);
        draw_text_right_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), total_text, ui_text_accent(state));

        draw_rounded_panel(card_x, card_y, card_width, 92u, 22u, ui_surface_color(state, 1u), ui_stroke_color(state, 1u));
        draw_progress_bar(card_x + 18u, card_y + 34u, safe_subtract(card_width, 36u), 24u, ram_percent, vga_color(82, 174, 255));

        format_megabytes(used_text, used_memory);
        format_megabytes(free_text, free_memory);
        draw_settings_option_row(state, card_x, card_y + 108u, card_width, "Used memory", used_text);
        draw_settings_option_row(state, card_x, card_y + 148u, card_width, "Free memory", free_text);
        draw_settings_option_row(state, card_x, card_y + 188u, card_width, "Installed size", total_text);
        format_percent_text(info_text, ram_percent);
        append_text(info_text + strlen(info_text), " pressure");
        draw_settings_option_row(state, card_x, card_y + 228u, card_width, "Benchmark", info_text);
        return;
    }

    if (state->task_manager_page == TASK_MANAGER_PAGE_STORAGE) {
        draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "Storage", ui_text_secondary(state));
        draw_text_right_clipped(content_x + 16u,
                                content_y + 18u,
                                safe_subtract(content_width, 32u),
                                state->storage_info.present && state->storage_info.model[0] != '\0' ? state->storage_info.model : "No ATA drive",
                                ui_text_accent(state));

        draw_rounded_panel(card_x, card_y, card_width, 92u, 22u, ui_surface_color(state, 1u), ui_stroke_color(state, 1u));
        draw_progress_bar(card_x + 18u, card_y + 34u, safe_subtract(card_width, 36u), 24u, storage_percent, vga_color(94, 214, 172));

        format_storage_size(total_text, state->storage_info.total_sectors);
        format_transfer_rate_text(value_text, storage_benchmark);
        draw_settings_option_row(state, card_x, card_y + 108u, card_width, "Drive model",
                                 state->storage_info.present && state->storage_info.model[0] != '\0' ? state->storage_info.model : "Unavailable");
        draw_settings_option_row(state, card_x, card_y + 148u, card_width, "Capacity", state->storage_info.present ? total_text : "0 MB");
        draw_settings_option_row(state, card_x, card_y + 188u, card_width, "Write benchmark", value_text);
        draw_settings_option_row(state, card_x, card_y + 228u, card_width, "Controller", state->storage_info.present ? "ATA primary master" : "Not connected");
        return;
    }

    draw_text_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), "Cpu Usage", ui_text_secondary(state));
    split_task_manager_header_text(state->cpu_brand[0] != '\0' ? state->cpu_brand : "Unknown CPU",
                                   header_line_one, header_line_two, 24u);
    draw_text_right_clipped(content_x + 16u, content_y + 18u, safe_subtract(content_width, 32u), header_line_one, ui_text_accent(state));
    if (header_line_two[0] != '\0') {
        draw_text_right_clipped(content_x + 16u, content_y + 36u, safe_subtract(content_width, 32u), header_line_two, ui_text_muted(state));
    }

    draw_rounded_panel(card_x, card_y, card_width, 92u, 22u, ui_surface_color(state, 1u), ui_stroke_color(state, 1u));
    draw_progress_bar(card_x + 18u, card_y + 34u, safe_subtract(card_width, 36u), 24u, cpu_percent, vga_color(142, 112, 214));

    format_uptime_text(runtime_text, (u64)seconds_since(state->desktop_started_second, rtc_read_seconds_of_day()) * 1000ull);
    value_text[0] = '\0';
    append_uint(value_text, state->cpu_benchmark_score);
    append_text(value_text + strlen(value_text), " ops/s benchmark");
    info_text[0] = '\0';
    append_uint(info_text, state->cpu_clock_mhz);
    append_text(info_text + strlen(info_text), " MHz");
    secondary_text[0] = '\0';
    append_uint(secondary_text, state->cpu_family);
    append_text(secondary_text + strlen(secondary_text), "/");
    append_uint(secondary_text + strlen(secondary_text), state->cpu_model);
    tertiary_text[0] = '\0';
    append_text(tertiary_text, state->cpu_vendor[0] != '\0' ? state->cpu_vendor : "generic");
    append_text(tertiary_text + strlen(tertiary_text), " / ");
    append_uint(tertiary_text + strlen(tertiary_text), state->cpu_stepping);
    quaternary_text[0] = '\0';
    append_text(quaternary_text, state->cpu_ready ? "Live benchmark active" : "Benchmark unavailable");

    draw_settings_option_row(state, card_x, card_y + 108u, card_width, "Runtime", runtime_text);
    draw_settings_option_row(state, card_x, card_y + 148u, card_width, "Benchmark", value_text);
    draw_settings_option_row(state, card_x, card_y + 188u, card_width, "Measured clock", info_text);
    draw_settings_option_row(state, card_x, card_y + 228u, card_width, "Family / model", secondary_text);
    draw_settings_option_row(state, card_x, card_y + 268u, card_width, "Vendor / stepping", tertiary_text);
    draw_settings_option_row(state, card_x, card_y + 308u, card_width, "Status", quaternary_text);
}

static task_manager_hit_t hit_test_task_manager_option(const desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 nav_x;
    u32 nav_y;
    u32 nav_width;

    if (state == NULL || !window_visible(state, WINDOW_TASKS) || window_shaded(state, WINDOW_TASKS)) {
        return TASK_MANAGER_HIT_NONE;
    }

    rect = get_window_rect_const(state, WINDOW_TASKS);
    if (rect == NULL) {
        return TASK_MANAGER_HIT_NONE;
    }

    nav_x = rect->x + 18u;
    nav_y = rect->y + 56u;
    nav_width = 180u;

    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 48u, nav_width - 20u, 34u)) {
        return TASK_MANAGER_HIT_NAV_CPU;
    }
    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 88u, nav_width - 20u, 34u)) {
        return TASK_MANAGER_HIT_NAV_RAM;
    }
    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 128u, nav_width - 20u, 34u)) {
        return TASK_MANAGER_HIT_NAV_STORAGE;
    }

    return TASK_MANAGER_HIT_NONE;
}

static bool handle_task_manager_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    task_manager_hit_t hit = hit_test_task_manager_option(state, mouse_x, mouse_y);

    if (state == NULL || hit == TASK_MANAGER_HIT_NONE) {
        return false;
    }

    if (hit == TASK_MANAGER_HIT_NAV_CPU) {
        state->task_manager_page = TASK_MANAGER_PAGE_CPU;
    } else if (hit == TASK_MANAGER_HIT_NAV_RAM) {
        state->task_manager_page = TASK_MANAGER_PAGE_RAM;
    } else if (hit == TASK_MANAGER_HIT_NAV_STORAGE) {
        state->task_manager_page = TASK_MANAGER_PAGE_STORAGE;
    }

    return true;
}

static bool setup_active(const desktop_state_t* state) {
    return state != NULL && state->setup_phase != SETUP_PHASE_NONE;
}

static void fill_password_mask(const char* value, char* out_buffer, u32 capacity) {
    u32 index = 0u;

    if (out_buffer == NULL || capacity == 0u) {
        return;
    }

    out_buffer[0] = '\0';
    if (value == NULL) {
        return;
    }

    while (value[index] != '\0' && index + 1u < capacity) {
        out_buffer[index] = '*';
        ++index;
    }
    out_buffer[index] = '\0';
}

static void draw_text_centered_bold(u32 y, const char* text, u32 color) {
    draw_text_centered(y, text, color);
    draw_text_centered(y, text, color);
    draw_text_centered(y, text, color);
    draw_text_centered(y + 1u, text, color);
}

static void draw_setup_field(u32 x,
                             u32 y,
                             u32 width,
                             const char* label,
                             const char* value,
                             const char* placeholder,
                             bool focused,
                             bool password_field,
                             bool light_style) {
    char display_value[32];
    const char* text = value;
    u32 stroke = light_style ? (focused ? vga_color(102, 150, 255) : vga_color(198, 208, 228))
                             : (focused ? vga_color(102, 150, 255) : vga_color(76, 82, 98));
    u32 fill = light_style ? (focused ? vga_color(244, 248, 255) : vga_color(252, 253, 255))
                           : (focused ? vga_color(26, 30, 46) : vga_color(14, 18, 28));
    u32 text_color = light_style ? vga_color(50, 58, 82) : vga_color(244, 248, 255);
    u32 placeholder_color = light_style ? vga_color(132, 142, 164) : vga_color(118, 126, 146);
    u32 label_color = light_style ? vga_color(84, 98, 132) : vga_color(164, 190, 236);

    draw_text_clipped(x, y, width, label, label_color);
    draw_rounded_panel(x, y + 18u, width, 42u, 14u, fill, stroke);
    if (password_field) {
        fill_password_mask(value, display_value, sizeof(display_value));
        text = display_value;
    }

    if (text != NULL && text[0] != '\0') {
        draw_text_clipped(x + 14u, y + 33u, safe_subtract(width, 28u), text, text_color);
    } else {
        draw_text_clipped(x + 14u, y + 33u, safe_subtract(width, 28u), placeholder, placeholder_color);
    }
}

static void get_setup_user_field_rect(u32 field, u32* out_x, u32* out_y, u32* out_width, u32* out_height) {
    u32 panel_x = (display_driver_width() > 640u) ? (display_driver_width() - 640u) / 2u : 0u;
    u32 panel_y = (display_driver_height() > 388u) ? (display_driver_height() - 388u) / 2u : 0u;

    if (out_x != NULL) {
        *out_x = panel_x + 40u;
    }
    if (out_y != NULL) {
        *out_y = panel_y + (field == AUTH_FIELD_USERNAME ? 144u : 232u);
    }
    if (out_width != NULL) {
        *out_width = 560u;
    }
    if (out_height != NULL) {
        *out_height = 60u;
    }
}

static void get_lockscreen_password_rect(u32* out_x, u32* out_y, u32* out_width, u32* out_height) {
    u32 width = 360u;
    u32 x = (display_driver_width() > width) ? (display_driver_width() - width) / 2u : 0u;
    u32 y = (display_driver_height() > 320u) ? (display_driver_height() / 2u + 22u) : 84u;

    if (out_x != NULL) {
        *out_x = x;
    }
    if (out_y != NULL) {
        *out_y = y;
    }
    if (out_width != NULL) {
        *out_width = width;
    }
    if (out_height != NULL) {
        *out_height = 42u;
    }
}

static void format_lockscreen_time_string(char* out_buffer, u32 capacity) {
    u32 seconds_of_day;
    u32 hour24;
    u32 minute;
    u32 hour12;
    u32 index = 0u;

    if (out_buffer == NULL || capacity < 6u) {
        return;
    }

    seconds_of_day = rtc_read_seconds_of_day();
    hour24 = (seconds_of_day / 3600u) % 24u;
    minute = (seconds_of_day / 60u) % 60u;
    hour12 = hour24 % 12u;
    if (hour12 == 0u) {
        hour12 = 12u;
    }

    if (hour12 >= 10u) {
        out_buffer[index++] = (char)('0' + (hour12 / 10u));
    }
    out_buffer[index++] = (char)('0' + (hour12 % 10u));
    out_buffer[index++] = ':';
    out_buffer[index++] = (char)('0' + (minute / 10u));
    out_buffer[index++] = (char)('0' + (minute % 10u));
    out_buffer[index] = '\0';
}

static void draw_setup_text_install_screen(const desktop_state_t* state, u32 current_second) {
    static const char* logo[] = {
        "      _       _           _  ___  ____  ",
        "     | | __ _| |__  _   _| |/ _ \\/ ___| ",
        "  _  | |/ _` | '_ \\| | | | | | | \\___ \\ ",
        " | |_| | (_| | |_) | |_| | | |_| |___) |",
        "  \\___/ \\__,_|_.__/ \\__,_|_|\\___/|____/ "
    };
    u32 elapsed = seconds_since(state->setup_phase_started_second, current_second);
    u32 line_y = 150u;
    u32 text_color = vga_color(184, 214, 184);
    bool copying_done = elapsed >= SETUP_INSTALL_COPY_SECONDS;
    bool install_done = elapsed >= SETUP_INSTALL_SECONDS && state->oobe_install_complete;

    vga_clear(vga_color(0, 0, 0));
    for (u32 index = 0; index < sizeof(logo) / sizeof(logo[0]); ++index) {
        draw_text(28u, 24u + index * 18u, logo[index], vga_color(140, 220, 154));
    }

    draw_text(28u, 118u, "JabulOS Setup Service", vga_color(238, 242, 238));
    draw_text(28u, line_y, copying_done ? "[ OK ] Copying ISO Files" : "[...] Copying ISO Files", text_color);
    if (copying_done) {
        draw_text(28u, line_y + 24u, install_done ? "[ OK ] Installing OS" :
                  (state->oobe_install_failed ? "[ERR] Installing OS" : "[...] Installing OS"), text_color);
    }
    if (state->oobe_install_failed) {
        draw_text(28u, line_y + 58u, "Disk write failed. Reboot and launch setup again.", vga_color(232, 120, 120));
    } else if (install_done) {
        draw_text(28u, line_y + 58u, "Installation complete. Rebooting into OOBE...", vga_color(196, 226, 196));
    } else if (copying_done) {
        draw_text(28u, line_y + 58u, "Please wait while JabulOS is written to disk.", vga_color(158, 178, 158));
    } else {
        draw_text(28u, line_y + 58u, "Preparing files from the install media.", vga_color(158, 178, 158));
    }
}

static void draw_setup_installer_screen(const desktop_state_t* state, const boot_info_t* boot_info, u32 current_second) {
    (void)current_second;
    u32 panel_x;
    u32 panel_y;
    u32 target_count;
    u32 content_y;
    u32 panel_width = 700u;
    u32 panel_height = 404u;

    target_count = install_target_count(state);

    panel_x = (display_driver_width() > panel_width) ? (display_driver_width() - panel_width) / 2u : 0u;
    panel_y = (display_driver_height() > panel_height) ? (display_driver_height() - panel_height) / 2u : 0u;
    vga_fill_rounded_rect_alpha(panel_x + 6u, panel_y + 10u, panel_width, panel_height, 28u, 28, 32, 50, 32u);
    vga_fill_rounded_rect_alpha(panel_x + 2u, panel_y + 4u, panel_width, panel_height, 28u, 28, 32, 50, 16u);
    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, 28u, vga_color(252, 253, 255), vga_color(208, 214, 232));
    vga_fill_rounded_rect_alpha(panel_x + 1u, panel_y + 1u, panel_width - 2u, 64u, 27u, 255, 255, 255, 44u);
    draw_rect(panel_x + 24u, panel_y + 94u, panel_width - 48u, 1u, vga_color(220, 226, 242));

    draw_text_clipped(panel_x + 34u, panel_y + 40u, 632u, "Install JabulOS", vga_color(36, 44, 74));
    draw_text_clipped(panel_x + 34u, panel_y + 66u, 632u, "Live setup from ISO", vga_color(84, 118, 194));
    content_y = draw_text_wrapped(panel_x + 34u,
                                  panel_y + 98u,
                                  632u,
                                  42u,
                                  "Select the disk target you would like to install to. Available targets are listed below with their sizes.",
                                  vga_color(92, 102, 126));
    if (state->oobe_install_failed) {
        draw_text_wrapped(panel_x + 34u, content_y + 8u, 632u, 24u, "The selected target is too small or the disk write failed.", vga_color(186, 76, 76));
    } else {
        draw_text_wrapped(panel_x + 34u,
                          content_y + 8u,
                          632u,
                          24u,
                          state->oobe_storage_ready ? "ATA disk detected. Choose a target and install." : "No ATA disk detected. Install is unavailable.",
                          state->oobe_storage_ready ? vga_color(86, 146, 106) : vga_color(186, 76, 76));
    }

    if (state->oobe_storage_ready && target_count > 0u) {
        for (u32 index = 0; index < target_count; ++index) {
            char label[32];
            char details[48];
            char size_buffer[24];
            u32 row_y = panel_y + 174u + index * 42u;
            u32 target_lba = 0u;
            u32 target_sectors = 0u;
            u32 partition_number = 0u;
            u32 row_fill;
            u32 row_stroke;
            u32 label_color;
            u32 detail_color;
            u32 size_color;
            bool whole_disk = false;
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

            if (selected) {
                row_fill = vga_color(220, 228, 255);
                row_stroke = vga_color(124, 148, 214);
                label_color = vga_color(28, 38, 76);
                detail_color = vga_color(66, 86, 138);
                size_color = vga_color(56, 96, 192);
            } else if (fits_target) {
                row_fill = vga_color(240, 242, 248);
                row_stroke = vga_color(176, 182, 198);
                label_color = vga_color(22, 24, 34);
                detail_color = vga_color(88, 96, 116);
                size_color = vga_color(76, 112, 194);
            } else {
                row_fill = vga_color(252, 238, 238);
                row_stroke = vga_color(210, 148, 148);
                label_color = vga_color(74, 28, 28);
                detail_color = vga_color(144, 78, 78);
                size_color = vga_color(180, 92, 92);
            }

            draw_rounded_panel(panel_x + 34u, row_y, 632u, 34u, 12u, row_fill, row_stroke);
            vga_fill_rounded_rect_alpha(panel_x + 36u, row_y + 2u, 628u, 9u, 9u, 255, 255, 255, selected ? 44u : 28u);
            draw_text_clipped(panel_x + 48u, row_y + 12u, 132u, label, label_color);
            draw_text_clipped(panel_x + 196u, row_y + 12u, 366u, details, detail_color);
            draw_text_right_clipped(panel_x + 34u + 632u - 116u, row_y + 12u, 100u, size_buffer, size_color);
        }
    }

    draw_text_wrapped(panel_x + 34u,
                      panel_y + 344u,
                      632u,
                      24u,
                      state->oobe_storage_ready ? "" : "Connect a disk to continue.",
                      vga_color(104, 112, 132));
    draw_oobe_button(state, panel_x + 700u - 34u - 148u - 10u - 168u, panel_y + 356u, 168u,
                     "Skip installer", true);
    draw_oobe_button(state, panel_x + 700u - 34u - 148u, panel_y + 356u, 148u,
                     state->oobe_storage_ready ? "Install" : "No disk", true);
}

static void draw_setup_welcome_screen(void) {
    u32 panel_x;
    u32 panel_y;
    u32 panel_width = 640u;
    u32 panel_height = 360u;

    panel_x = (display_driver_width() > panel_width) ? (display_driver_width() - panel_width) / 2u : 0u;
    panel_y = (display_driver_height() > panel_height) ? (display_driver_height() - panel_height) / 2u : 0u;
    vga_fill_rounded_rect_alpha(panel_x + 6u, panel_y + 10u, panel_width, panel_height, 28u, 28, 32, 50, 30u);
    vga_fill_rounded_rect_alpha(panel_x + 2u, panel_y + 4u, panel_width, panel_height, 28u, 28, 32, 50, 16u);
    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, 28u, vga_color(252, 253, 255), vga_color(208, 214, 232));
    vga_fill_rounded_rect_alpha(panel_x + 1u, panel_y + 1u, panel_width - 2u, 64u, 27u, 255, 255, 255, 44u);
    draw_rect(panel_x + 24u, panel_y + 94u, panel_width - 48u, 1u, vga_color(220, 226, 242));

    draw_text(panel_x + 40u, panel_y + 40u, "Welcome to JabulOS Installer", vga_color(36, 44, 74));
    draw_text(panel_x + 40u, panel_y + 68u, "Let's get JabulOS installed on your machine.", vga_color(84, 118, 194));

    draw_oobe_button(NULL, panel_x + 640u - 40u - 120u, panel_y + 360u - 40u - OOBE_BUTTON_HEIGHT, 120u, "Next", true);
}

static void draw_setup_update_screen(const desktop_state_t* state) {
    u32 width = display_driver_width();
    u32 height = display_driver_height();
    vga_clear(vga_color(0, 0, 0));
    
    char message[128];
    strcpy(message, "Updating/Downgrading to ");
    append_uint(message + strlen(message), state->update_version);
    strcat(message, ", Please wait");
    
    u32 text_w = (u32)strlen(message) * 8;
    draw_text((width - text_w) / 2, height / 2 - 20, message, vga_color(255, 255, 255));
    
    u32 bar_width = 400;
    u32 bar_height = 12;
    u32 bar_x = (width - bar_width) / 2;
    u32 bar_y = height / 2 + 20;
    
    draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1, vga_color(100, 100, 100));
    
    u64 elapsed = timer_ticks_ms() - state->setup_phase_started_ms;
    u32 progress = (u32)(elapsed * (bar_width - 4) / 50000);
    if (progress > bar_width - 4) progress = bar_width - 4;
    
    draw_rect(bar_x + 2, bar_y + 2, progress, bar_height - 4, vga_color(255, 255, 255));
}

static bool install_staged_update(desktop_state_t* state) {
    (void)state;
    return true;
}

static void draw_setup_progress_message(const desktop_state_t* state, const char* text, u32 phase_seconds) {
    u32 text_y = (display_driver_height() > 40u) ? (display_driver_height() / 2u - 10u) : 0u;
    u32 phase_duration_ms = phase_seconds * 1000u;
    u32 fade_duration_ms = phase_duration_ms / 3u;
    u32 elapsed_ms = 0u;
    u32 fade_in = 255u;
    u32 fade_out = 255u;
    u32 intensity;
    const u32 min_intensity = 104u;

    if (fade_duration_ms > 900u) {
        fade_duration_ms = 900u;
    }
    if (fade_duration_ms == 0u) {
        fade_duration_ms = phase_duration_ms > 1u ? phase_duration_ms / 2u : 1u;
    }
    if (state != NULL) {
        u64 now_ms = timer_ticks_ms();
        if (now_ms > state->setup_phase_started_ms) {
            elapsed_ms = (u32)(now_ms - state->setup_phase_started_ms);
        }
    }
    if (elapsed_ms > phase_duration_ms) {
        elapsed_ms = phase_duration_ms;
    }
    if (elapsed_ms < fade_duration_ms) {
        fade_in = (elapsed_ms * 255u) / fade_duration_ms;
    }
    if (phase_duration_ms > elapsed_ms) {
        u32 remaining_ms = phase_duration_ms - elapsed_ms;
        if (remaining_ms < fade_duration_ms) {
            fade_out = (remaining_ms * 255u) / fade_duration_ms;
        }
    } else {
        fade_out = 0u;
    }
    intensity = fade_in < fade_out ? fade_in : fade_out;
    intensity = min_intensity + ((255u - min_intensity) * intensity) / 255u;

    if (state != NULL && state->setup_phase == SETUP_PHASE_OOBE_PREPARE) {
        u32 panel_width = display_driver_width() > 760u ? 760u : safe_subtract(display_driver_width(), 40u);
        u32 panel_height = 104u;
        u32 panel_x = display_driver_width() > panel_width ? (display_driver_width() - panel_width) / 2u : 0u;
        u32 panel_y = display_driver_height() > panel_height ? (display_driver_height() - panel_height) / 2u : 0u;
        u32 text_color = vga_color(intensity, intensity, intensity);

        vga_fill_rounded_rect_alpha(panel_x + 8u, panel_y + 12u, panel_width, panel_height, 26u, 0, 0, 0, 28u);
        draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, 26u, vga_color(10, 12, 20), vga_color(56, 62, 92));
        vga_fill_rounded_rect_alpha(panel_x + 2u, panel_y + 2u, panel_width - 4u, 18u, 16u, 255, 255, 255, 14u);
        draw_text_wrapped(panel_x + 28u, panel_y + 28u, safe_subtract(panel_width, 56u), 48u, text, text_color);
        draw_text_wrapped(panel_x + 29u, panel_y + 28u, safe_subtract(panel_width, 57u), 48u, text, text_color);
        return;
    }

    draw_text_centered_bold(text_y, text, (state != NULL) ? vga_color(36, 44, 74) : vga_color(intensity, intensity, intensity));
}

static void draw_setup_user_oobe_screen(const desktop_state_t* state) {
    u32 panel_x;
    u32 panel_y;
    u32 username_y;
    u32 password_y;

    panel_x = (display_driver_width() > 640u) ? (display_driver_width() - 640u) / 2u : 0u;
    panel_y = (display_driver_height() > 388u) ? (display_driver_height() - 388u) / 2u : 0u;
    vga_fill_rounded_rect_alpha(panel_x + 6u, panel_y + 10u, 640u, 388u, 28u, 120, 136, 178, 26u);
    vga_fill_rounded_rect_alpha(panel_x + 2u, panel_y + 4u, 640u, 388u, 28u, 120, 136, 178, 12u);
    draw_rounded_panel(panel_x, panel_y, 640u, 388u, 28u, vga_color(252, 253, 255), vga_color(208, 214, 232));
    vga_fill_rounded_rect_alpha(panel_x + 1u, panel_y + 1u, 638u, 64u, 27u, 255, 255, 255, 44u);
    draw_rect(panel_x + 24u, panel_y + 94u, 592u, 1u, vga_color(220, 226, 242));
    draw_text_clipped(panel_x + 40u, panel_y + 52u, 560u, "Welcome To JabulOS Aero", vga_color(36, 44, 74));
    username_y = panel_y + 120u;
    password_y = panel_y + 208u;
    draw_setup_field(panel_x + 40u,
                     username_y,
                     560u,
                     "Enter User",
                     state->account_input_username,
                     "Type your user name",
                     state->auth_field_focus == AUTH_FIELD_USERNAME,
                     false,
                     true);
    draw_setup_field(panel_x + 40u,
                     password_y,
                     560u,
                     "Enter Password",
                     state->account_input_password,
                     "Type any password",
                     state->auth_field_focus == AUTH_FIELD_PASSWORD,
                     true,
                     true);
    if (state->oobe_install_failed) {
        draw_text_clipped(panel_x + 40u, panel_y + 318u, 560u, "Enter both a user name and a password.", vga_color(196, 86, 86));
    }
    draw_oobe_button(state, panel_x + 640u - 40u - 176u, panel_y + 338u, 176u, "Continue", true);
}

static void draw_setup_checking_hardware_screen(const desktop_state_t* state) {
    draw_setup_progress_message(state, "Checking System Hardware", 3u);
}

static void draw_setup_skip_installer_screen(const desktop_state_t* state) {
    u32 sw = display_driver_width();
    u32 sh = display_driver_height();
    vga_clear(vga_color(0, 0, 0));

    // Draw "Loading Resources" text
    draw_text_centered(sh / 2u - 20u, "Loading Resources", vga_color(255, 255, 255));

    // Draw loading bar
    u32 bar_width = sw / 4u;
    u32 bar_height = 8u;
    u32 bar_x = (sw - bar_width) / 2u;
    u32 bar_y = sh / 2u + 20u;

    u32 duration = 5000u; // 5 seconds
    u64 elapsed = timer_ticks_ms() - state->setup_phase_started_ms;
    u32 progress_width = (u32)((elapsed * bar_width) / duration);
    if (progress_width > bar_width) progress_width = bar_width;

    draw_rect(bar_x, bar_y, bar_width, bar_height, vga_color(40, 40, 40));
    draw_rect(bar_x, bar_y, progress_width, bar_height, vga_color(255, 255, 255));
}

static void draw_setup_hardware_error_screen(void) {
    u32 panel_x;
    u32 panel_y;
    u32 panel_width = 640u;
    u32 panel_height = 240u;

    panel_x = (display_driver_width() > panel_width) ? (display_driver_width() - panel_width) / 2u : 0u;
    panel_y = (display_driver_height() > panel_height) ? (display_driver_height() - panel_height) / 2u : 0u;
    vga_fill_rounded_rect_alpha(panel_x + 6u, panel_y + 10u, panel_width, panel_height, 28u, 120, 136, 178, 26u);
    draw_rounded_panel(panel_x, panel_y, panel_width, panel_height, 28u, vga_color(252, 253, 255), vga_color(208, 214, 232));
    vga_fill_rounded_rect_alpha(panel_x + 1u, panel_y + 1u, panel_width - 2u, 64u, 27u, 255, 255, 255, 44u);
    draw_rect(panel_x + 24u, panel_y + 94u, panel_width - 48u, 1u, vga_color(220, 226, 242));

    draw_text_clipped(panel_x + 40u, panel_y + 40u, panel_width - 80u, "Compatibility Error", vga_color(186, 76, 76));
    draw_text_wrapped(panel_x + 40u, panel_y + 120u, panel_width - 80u, 48u, "Your system does not meet the official requirements to install JabulOS Aero.", vga_color(92, 102, 126));
    draw_text_wrapped(panel_x + 40u, panel_y + 168u, panel_width - 80u, 24u, "Requirement: 512MB RAM, 700MB IDE Storage.", vga_color(124, 132, 154));
}

static void draw_text_centered_extra_bold(u32 y, const char* text, u32 color) {
    u32 width = (u32)strlen(text) * vga_text_advance_x();
    u32 x = (display_driver_width() > width) ? (display_driver_width() - width) / 2u : 0u;
    
    // Draw 4 times with small offsets to simulate heavy bold weight
    draw_text(x, y, text, color);
    draw_text(x + 1, y, text, color);
    draw_text(x, y + 1, text, color);
    draw_text(x + 1, y + 1, text, color);
}

static void draw_lockscreen(const desktop_state_t* state) {
    char clock_buffer[12];
    char date_buffer[64];
    u32 screen_w = display_driver_width();
    u32 screen_h = display_driver_height();
    u64 now = timer_ticks_ms();
    u32 anim_duration = 200u;
    u32 anim_progress_q8 = 0u;

    if (state->lockscreen_background != NULL) {
        display_driver_draw_wallpaper(state->lockscreen_background);
    } else {
        vga_clear(vga_color(0, 0, 0));
    }

    if (state->lockscreen_prompt_visible) {
        u64 elapsed = now - state->lockscreen_prompt_started_ms;
        if (elapsed < anim_duration) {
            anim_progress_q8 = (u32)((elapsed * 256u) / anim_duration);
        } else {
            anim_progress_q8 = 256u;
        }
    }

    // Background darkening during prompt
    if (anim_progress_q8 > 0u) {
        u8 alpha = (u8)((anim_progress_q8 * 140u) / 256u);
        vga_fill_rect_alpha(0, 0, screen_w, screen_h, 0, 0, 0, alpha);
    }

    format_lockscreen_time_string(clock_buffer, sizeof(clock_buffer));
    rtc_read_date_string(date_buffer);
    
    // Clock animation: slides up when prompt is visible
    u32 clock_center_y = screen_h / 2u - 100u;
    u32 clock_top_y = 120u;
    u32 clock_y = (clock_center_y * (256u - anim_progress_q8) + clock_top_y * anim_progress_q8) / 256u;
    
    // Large centered clock (rendered with extra weight to match image)
    draw_text_centered_extra_bold(clock_y, clock_buffer, vga_color(255, 255, 255));
    // Centered date below clock
    draw_text_centered(clock_y + 48u, date_buffer, vga_color(255, 255, 255));

    // Bottom right icons (simulated with text for now as we don't have icons)
    if (!state->lockscreen_prompt_visible) {
        u32 icon_y = screen_h - 40u;
        draw_text(screen_w - 120u, icon_y, "N", vga_color(255, 255, 255)); // Network
        draw_text(screen_w - 80u, icon_y, "B", vga_color(255, 255, 255));  // Battery
        draw_text(screen_w - 40u, icon_y, "P", vga_color(255, 255, 255));  // Power
    }

    if (anim_progress_q8 > 0u) {
        u32 ui_y_offset = (30u * (256u - anim_progress_q8)) / 256u;
        
        // Profile picture (MacOS style but only shown in prompt phase)
        if (state->lockscreen_profile != NULL) {
            u32 profile_size = 120u;
            u32 profile_x = (screen_w - profile_size) / 2u;
            u32 profile_y = screen_h / 2u - 60u + ui_y_offset;
            image_blit_fit_rounded(state->lockscreen_profile, profile_x, profile_y, profile_size, profile_size, profile_size / 2u);
            
            const char* username = state->account_username[0] != '\0' ? state->account_username : "User";
            draw_text_centered_bold(profile_y + profile_size + 20u, username, vga_color(255, 255, 255));
        }

        // Password field
        u32 field_x, field_y, field_width, field_height;
        get_lockscreen_password_rect(&field_x, &field_y, &field_width, &field_height);
        field_y += 100u + ui_y_offset;

        draw_setup_field(field_x,
                         field_y,
                         field_width,
                         "", // Empty label for cleaner look
                         state->login_password_input,
                         "Enter Password",
                         true,
                         true,
                         false);

        draw_oobe_button(state, field_x + (field_width - 160u) / 2u, field_y + 82u, 160u, "Unlock", true);
        
        if (state->login_error) {
            draw_text_centered(field_y + 128u, "Wrong password, Try again", vga_color(255, 80, 80));
        }
    }
}

static void render_setup_screen(const desktop_state_t* state,
                                const image_t* wallpaper,
                                const boot_info_t* boot_info,
                                u32 current_second) {
    if (state->setup_phase != SETUP_PHASE_INSTALLING && state->setup_phase != SETUP_PHASE_LOCKSCREEN) {
        draw_desktop_background(state, wallpaper);
        vga_fill_rect_alpha(0, 0, display_driver_width(), display_driver_height(), 255, 255, 255, 160);
    }

    if (state->setup_phase == SETUP_PHASE_WELCOME) {
        draw_setup_welcome_screen();
    } else if (state->setup_phase == SETUP_PHASE_INSTALLER) {
        draw_setup_installer_screen(state, boot_info, current_second);
    } else if (state->setup_phase == SETUP_PHASE_INSTALLING) {
        draw_setup_text_install_screen(state, current_second);
    } else if (state->setup_phase == SETUP_PHASE_USER_OOBE) {
        draw_setup_user_oobe_screen(state);
    } else if (state->setup_phase == SETUP_PHASE_OOBE_HI) {
        draw_setup_progress_message(state, "Hi", SETUP_OOBE_HI_SECONDS);
    } else if (state->setup_phase == SETUP_PHASE_OOBE_SETTING_UP) {
        draw_setup_progress_message(state, "Setting up JabulOS For you", SETUP_OOBE_SETTING_UP_SECONDS);
    } else if (state->setup_phase == SETUP_PHASE_OOBE_PREPARE) {
        draw_setup_progress_message(state, "Get prepared to load JabulOS on your PC for first use", SETUP_OOBE_PREPARE_SECONDS);
    } else if (state->setup_phase == SETUP_PHASE_OOBE_ALMOST_THERE) {
        draw_setup_progress_message(state, "Almost there", SETUP_OOBE_ALMOST_THERE_SECONDS);
    } else if (state->setup_phase == SETUP_PHASE_CHECKING_HARDWARE) {
        draw_setup_checking_hardware_screen(state);
    } else if (state->setup_phase == SETUP_PHASE_SKIPPING_INSTALLER) {
        draw_setup_skip_installer_screen(state);
    } else if (state->setup_phase == SETUP_PHASE_UPDATE_PROGRESS) {
        draw_setup_update_screen(state);
    } else if (state->setup_phase == SETUP_PHASE_HARDWARE_ERROR) {
        draw_setup_hardware_error_screen();
    } else if (state->setup_phase == SETUP_PHASE_LOCKSCREEN) {
        draw_lockscreen(state);
    } else {
        vga_clear(vga_color(0, 0, 0));
    }
}

static void draw_oobe_button(const desktop_state_t* state, u32 x, u32 y, u32 width, const char* label, bool primary) {
    bool dark = desktop_dark_mode(state);
    u32 fill = primary ? (dark ? vga_color(78, 122, 224) : vga_color(82, 132, 238))
                       : (dark ? ui_surface_color(state, 3u) : vga_color(116, 156, 242));
    u32 stroke = primary ? (dark ? vga_color(142, 178, 255) : vga_color(202, 224, 255))
                         : (dark ? ui_stroke_color(state, 2u) : vga_color(194, 214, 252));
    u32 text = vga_color(248, 250, 255);

    draw_rounded_rect(x, y, width, OOBE_BUTTON_HEIGHT, 14u, fill);
    vga_fill_rounded_rect_alpha(x + 2u, y + 2u, width - 4u, 10u, 10u, 255, 255, 255, primary ? (dark ? 18u : 58u) : (dark ? 10u : 42u));
    draw_rounded_rect_outline(x, y, width, OOBE_BUTTON_HEIGHT, 14u, 1u, stroke);
    draw_text_centered_clipped(x + 8u, y + 13u, safe_subtract(width, 16u), label, text);
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
    u32 body_width;
    u32 status_width;

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
        title = "Welcome";
        primary_label = "Install now";
        secondary_label = "Open desktop";
    }

    format_install_target_name(state, state->install_target_index, target_label, target_size);
    draw_window_frame(state, rect->x, rect->y, rect->width, rect->height, title, accent);
    body_width = safe_subtract(rect->width, 48u);
    status_width = safe_subtract(rect->width, 72u);

    if (state->oobe_stage == OOBE_STAGE_INTRO) {
        u32 panel_x = rect->x + 28u;
        u32 panel_y = rect->y + 54u;
        u32 chip_color = vga_color(132, 122, 244);

        draw_text_clipped(rect->x + 150u, rect->y + 58u, safe_subtract(rect->width, 174u), "Welcome back, Froxy.", vga_color(32, 36, 60));
        draw_rounded_rect(panel_x, panel_y + 12u, 108u, 172u, 28u, vga_color(236, 238, 252));
        vga_fill_rect_alpha(panel_x + 2u, panel_y + 14u, 104u, 46u, 255, 255, 255, 34);
        draw_rounded_rect(panel_x + 14u, panel_y + 24u, 72u, 14u, 7u, chip_color);
        draw_rounded_rect(panel_x + 14u, panel_y + 50u, 72u, 14u, 7u, chip_color);
        draw_rounded_rect(panel_x + 14u, panel_y + 76u, 72u, 14u, 7u, chip_color);
        draw_rounded_rect(panel_x + 14u, panel_y + 112u, 46u, 46u, 18u, vga_color(148, 152, 238));
        draw_rounded_rect(panel_x + 68u, panel_y + 112u, 122u, 46u, 20u, vga_color(116, 124, 244));
        draw_rounded_rect(panel_x + 14u, panel_y + 166u, 54u, 34u, 14u, vga_color(210, 214, 248));
        draw_rounded_rect(panel_x + 78u, panel_y + 166u, 28u, 34u, 14u, chip_color);
        draw_rounded_rect(panel_x + 120u, panel_y + 166u, 94u, 34u, 14u, vga_color(188, 192, 236));

        draw_text_clipped(rect->x + 44u,
                          rect->y + 250u,
                          112u,
                          state->oobe_storage_ready ? "Storage ready" : "Storage offline",
                          state->oobe_storage_ready ? vga_color(76, 128, 100) : vga_color(156, 76, 76));
        draw_text_clipped(rect->x + 172u,
                          rect->y + 250u,
                          safe_subtract(rect->width, 196u),
                          state->oobe_disk_has_install ? "Desktop image found" : "Live session",
                          vga_color(96, 100, 146));
        draw_oobe_button(state, rect->x + 24u, rect->y + rect->height - 50u, OOBE_BUTTON_WIDTH, secondary_label, false);
        draw_oobe_button(state, rect->x + rect->width - OOBE_BUTTON_WIDTH - 24u, rect->y + rect->height - 50u,
                         OOBE_BUTTON_WIDTH, primary_label, true);
        return;
    }

    draw_text_clipped(rect->x + 24u, rect->y + 56u, body_width, PREVIEW_LABEL_TEXT, vga_color(232, 238, 248));
    draw_text_clipped(rect->x + 24u, rect->y + 78u, body_width, "Modern translucent shell active", vga_color(214, 222, 236));
    draw_text_clipped(rect->x + 24u, rect->y + 98u, body_width, "Kernel already runs in 64-bit mode", vga_color(188, 198, 214));

    draw_rounded_rect(rect->x + 24u, rect->y + 126u, rect->width - 48u, 54u, 14u, vga_color(18, 24, 36));
    vga_fill_rect_alpha(rect->x + 25u, rect->y + 127u, rect->width - 50u, 52u, 255, 255, 255, 10);
    draw_rect_outline(rect->x + 24u, rect->y + 126u, rect->width - 48u, 54u, 1u, vga_color(88, 102, 130));
    if (target_label[0] != '\0') {
        draw_text_right_clipped(rect->x + rect->width - 220u, rect->y + 110u, 128u, target_label, vga_color(132, 196, 255));
        draw_text_right_clipped(rect->x + rect->width - 84u, rect->y + 110u, 60u, target_size, vga_color(184, 194, 210));
    }

    if (state->oobe_stage == OOBE_STAGE_CONFIRM) {
        draw_text_wrapped(rect->x + 38u,
                          rect->y + 142u,
                          status_width,
                          34u,
                          target_label[0] != '\0' ? "Warning: this overwrites the selected target." : "Warning: this overwrites the selected disk target.",
                          vga_color(246, 210, 186));
        draw_text_wrapped(rect->x + 38u, rect->y + 166u, status_width, 24u, "Kernel and initrd are written directly.", vga_color(214, 222, 236));
    } else if (state->oobe_stage == OOBE_STAGE_COMPLETE) {
        draw_text_wrapped(rect->x + 38u,
                          rect->y + 142u,
                          status_width,
                          34u,
                          target_label[0] != '\0' ? "System image was written to the selected target." : "System image was written to disk.",
                          vga_color(206, 244, 220));
        draw_text_wrapped(rect->x + 38u, rect->y + 166u, status_width, 24u, "The next boot skips the one-time setup flow.", vga_color(214, 222, 236));
    } else if (state->oobe_stage == OOBE_STAGE_ERROR) {
        draw_text_wrapped(rect->x + 38u, rect->y + 142u, status_width, 24u, "The installer could not write the disk.", vga_color(252, 204, 204));
        draw_text_wrapped(rect->x + 38u, rect->y + 166u, status_width, 24u, "Retry in a VM or check your storage.", vga_color(214, 222, 236));
    } else if (state->oobe_stage == OOBE_STAGE_STORAGE_MISSING) {
        draw_text_wrapped(rect->x + 38u, rect->y + 142u, status_width, 24u, "No ATA primary master was detected.", vga_color(252, 204, 204));
        draw_text_wrapped(rect->x + 38u, rect->y + 166u, status_width, 24u, "The desktop works, but install is disabled.", vga_color(214, 222, 236));
    } else {
        draw_text_wrapped(rect->x + 38u,
                          rect->y + 142u,
                          status_width,
                          34u,
                          target_label[0] != '\0' ? "Ready to install the live image to the selected target." : "Ready to install the live image.",
                          vga_color(214, 222, 236));
        draw_text_wrapped(rect->x + 38u, rect->y + 166u, status_width, 24u, "The one-time OOBE only runs until an install exists.", vga_color(214, 222, 236));
    }

    draw_text_clipped(rect->x + 24u,
                      rect->y + 198u,
                      170u,
                      state->oobe_storage_ready ? "Storage ATA ready" : "Storage ATA missing",
                      vga_color(214, 222, 236));
    draw_text_clipped(rect->x + 220u,
                      rect->y + 198u,
                      safe_subtract(rect->width, 244u),
                      state->oobe_disk_has_install ? "Disk image present" : "Disk image not found",
                      vga_color(214, 222, 236));
    draw_progress_bar(rect->x + 24u, rect->y + 220u, rect->width - 48u, 16u,
                      state->oobe_disk_has_install ? 100u : (state->oobe_storage_ready ? 52u : 8u),
                      state->oobe_disk_has_install ? vga_color(86, 198, 140) : vga_color(88, 144, 240));

    draw_oobe_button(state, rect->x + 24u, rect->y + rect->height - 50u, OOBE_BUTTON_WIDTH, secondary_label, false);
    draw_oobe_button(state, rect->x + rect->width - OOBE_BUTTON_WIDTH - 24u, rect->y + rect->height - 50u,
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
        state->oobe_install_complete = false;
        state->oobe_install_failed = !installed;
        if (installed) {
            if (!persist_installed_target_state(state,
                                                INSTALL_STATE_STAGE_NEEDS_OOBE,
                                                target_lba,
                                                target_sectors,
                                                "",
                                                "")) {
                state->oobe_stage = OOBE_STAGE_ERROR;
                return true;
            }
            state->live_media_boot = false;
            state->oobe_disk_has_install = true;
            state->oobe_install_complete = true;
        }
        state->oobe_stage = state->oobe_install_complete ? OOBE_STAGE_COMPLETE : OOBE_STAGE_ERROR;
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
    draw_mouse_cursor_to_surface(x, y, true);
}

static bool activate_launcher_result(desktop_state_t* state, u32 launcher_index) {
    if (state == NULL || launcher_index >= LAUNCHER_APP_COUNT) {
        return false;
    }

    open_window(state, g_launcher_window_ids[launcher_index]);
    close_window(state, WINDOW_SEARCH);
    state->show_welcome = false;
    state->launcher_power_menu_open = false;
    state->launcher_query[0] = '\0';
    return true;
}

static bool handle_launcher_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y, u32 current_second) {
    const window_rect_t* rect;
    u32 launcher_index;
    u32 power_x;
    u32 power_y;
    u32 power_width;
    u32 power_height;
    u32 menu_x;
    u32 menu_y;
    u32 menu_width;
    u32 menu_height;

    if (state == NULL || !window_visible(state, WINDOW_SEARCH) || window_shaded(state, WINDOW_SEARCH)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_SEARCH);
    if (rect == NULL) {
        return false;
    }

    if (point_in_rect(mouse_x, mouse_y, rect->x + 20u, rect->y + 18u, rect->width - 40u, 34u)) {
        state->launcher_power_menu_open = false;
        return true;
    }

    launcher_power_button_rect(rect, &power_x, &power_y, &power_width, &power_height);
    if (point_in_rect(mouse_x, mouse_y, power_x, power_y, power_width, power_height)) {
        state->launcher_power_menu_open = !state->launcher_power_menu_open;
        return true;
    }

    launcher_power_menu_rect(rect, &menu_x, &menu_y, &menu_width, &menu_height);
    if (state->launcher_power_menu_open &&
        point_in_rect(mouse_x, mouse_y, menu_x, menu_y, menu_width, menu_height)) {
        if (point_in_rect(mouse_x, mouse_y, menu_x + 10u, menu_y + 10u, menu_width - 20u, 28u)) {
            begin_power_action(state, POWER_ACTION_SHUTDOWN, current_second);
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, menu_x + 10u, menu_y + 46u, menu_width - 20u, 28u)) {
            begin_power_action(state, POWER_ACTION_RESTART, current_second);
            return true;
        }
        return true;
    }

    state->launcher_power_menu_open = false;
    if (launcher_result_at(state, rect, mouse_x, mouse_y, &launcher_index)) {
        return activate_launcher_result(state, launcher_index);
    }

    return false;
}

static settings_hit_t hit_test_settings_option(const desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 nav_x;
    u32 nav_y;
    u32 nav_width;
    u32 nav_height;
    u32 content_x;
    u32 content_y;
    u32 content_width;
    u32 content_height;
    u32 button_width;
    u32 button_x;
    u32 button_y;

    if (state == NULL || !window_visible(state, WINDOW_SETTINGS) || window_shaded(state, WINDOW_SETTINGS)) {
        return SETTINGS_HIT_NONE;
    }

    rect = get_window_rect_const(state, WINDOW_SETTINGS);
    if (rect == NULL) {
        return SETTINGS_HIT_NONE;
    }

    nav_width = 196u;
    nav_x = rect->x + 18u;
    nav_y = rect->y + 56u;
    nav_height = rect->height > 74u ? rect->height - 74u : 0u;

    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 14u, nav_width - 20u, 34u)) {
        return SETTINGS_HIT_NAV_SYSTEM_SPECS;
    }
    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 54u, nav_width - 20u, 34u)) {
        return SETTINGS_HIT_NAV_PERSONALIZE;
    }
    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 94u, nav_width - 20u, 34u)) {
        return SETTINGS_HIT_NAV_THEMES;
    }
    if (point_in_rect(mouse_x, mouse_y, nav_x + 10u, nav_y + 134u, nav_width - 20u, 34u)) {
        return SETTINGS_HIT_NAV_UPDATE;
    }

    if (state->settings_page == SETTINGS_PAGE_PERSONALIZE) {
        u32 row_width;
        u32 row_x;
        u32 row_y;
        u32 style_button_width;
        u32 style_button_gap;
        u32 tenui_button_x;
        u32 macxwin_button_x;
        u32 style_button_y;

        content_x = nav_x + nav_width + 16u;
        content_y = nav_y;
        content_width = rect->width > (content_x - rect->x + 18u) ? rect->width - (content_x - rect->x) - 18u : 0u;

        row_x = content_x + 16u;
        row_y = content_y + 50u;
        row_width = safe_subtract(content_width, 32u);
        style_button_width = 98u;
        style_button_gap = 10u;
        tenui_button_x = row_x + safe_subtract(row_width, style_button_width + 14u);
        macxwin_button_x = tenui_button_x - style_button_gap - style_button_width;
        style_button_y = row_y + 13u;
        if (point_in_rect(mouse_x, mouse_y, macxwin_button_x, style_button_y, style_button_width, 34u)) {
            return SETTINGS_HIT_TASKBAR_STYLE_MACXWIN;
        }
        if (point_in_rect(mouse_x, mouse_y, tenui_button_x, style_button_y, style_button_width, 34u)) {
            return SETTINGS_HIT_TASKBAR_STYLE_TENUI;
        }
    }

    if (state->settings_page == SETTINGS_PAGE_THEMES) {
        content_x = nav_x + nav_width + 16u;
        content_y = nav_y;
        content_width = rect->width > (content_x - rect->x + 18u) ? rect->width - (content_x - rect->x) - 18u : 0u;

        for (u32 index = 0u; index < WALLPAPER_THEME_COUNT; ++index) {
            window_rect_t card_rect;
            if (!settings_theme_card_rect(content_x, content_y, content_width, index, &card_rect)) {
                continue;
            }
            if (point_in_rect(mouse_x, mouse_y, card_rect.x, card_rect.y, card_rect.width, card_rect.height)) {
                return (settings_hit_t)(SETTINGS_HIT_THEME_ALANBLISS + index);
            }
        }
    }

    if (state->settings_page == SETTINGS_PAGE_UPDATE && state->update_available && !state->update_installing) {
        content_x = nav_x + nav_width + 16u;
        content_y = nav_y;
        content_width = rect->width > (content_x - rect->x + 18u) ? rect->width - (content_x - rect->x) - 18u : 0u;
        content_height = nav_height;
        button_width = 180u;
        button_x = content_x + safe_subtract(content_width, button_width + 16u);
        button_y = content_y + safe_subtract(content_height, 52u);
        if (point_in_rect(mouse_x, mouse_y, button_x, button_y, button_width, 34u)) {
            return SETTINGS_HIT_UPDATE_INSTALL;
        }
    }

    return SETTINGS_HIT_NONE;
}

static bool handle_settings_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    settings_hit_t hit = hit_test_settings_option(state, mouse_x, mouse_y);

    if (state == NULL || hit == SETTINGS_HIT_NONE) {
        return false;
    }

    if (hit == SETTINGS_HIT_NAV_SYSTEM_SPECS) {
        state->settings_page = SETTINGS_PAGE_SYSTEM_SPECS;
    } else if (hit == SETTINGS_HIT_NAV_UPDATE) {
        state->settings_page = SETTINGS_PAGE_UPDATE;
    } else if (hit == SETTINGS_HIT_NAV_PERSONALIZE) {
        state->settings_page = SETTINGS_PAGE_PERSONALIZE;
    } else if (hit == SETTINGS_HIT_NAV_THEMES) {
        state->settings_page = SETTINGS_PAGE_THEMES;
    } else if (hit == SETTINGS_HIT_UPDATE_INSTALL) {
        state->update_install_complete = false;
        state->update_install_failed = false;
        state->update_installing = true;
        if (!install_staged_update(state)) {
            state->update_install_failed = true;
        }
    } else if (hit == SETTINGS_HIT_TASKBAR_STYLE_MACXWIN) {
        state->taskbar_style = TASKBAR_STYLE_MACXWIN;
        position_search_panel(state);
    } else if (hit == SETTINGS_HIT_TASKBAR_STYLE_TENUI) {
        state->taskbar_style = TASKBAR_STYLE_TENUI;
        position_search_panel(state);
    } else if (hit >= SETTINGS_HIT_THEME_ALANBLISS && hit <= SETTINGS_HIT_THEME_MACTOSH) {
        state->wallpaper_theme = (u32)(hit - SETTINGS_HIT_THEME_ALANBLISS);
    }

    return true;
}

static bool handle_file_manager_click(desktop_state_t* state,
                                      const boot_info_t* boot_info,
                                      u32 mouse_x,
                                      u32 mouse_y) {
    const window_rect_t* rect;
    file_manager_layout_t layout;

    if (state == NULL || !window_visible(state, WINDOW_FILES) || window_shaded(state, WINDOW_FILES)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_FILES);
    if (rect == NULL || !file_manager_get_layout(rect, &layout)) {
        return false;
    }

    for (u32 index = 0u; index < 5u; ++index) {
        if (point_in_rect(mouse_x, mouse_y, layout.nav_x + 10u, layout.nav_y + 46u + index * 42u, layout.nav_width - 20u, 32u)) {
            file_manager_select_sidebar(state, boot_info, (file_manager_sidebar_t)index);
            return true;
        }
    }

    if (point_in_rect(mouse_x, mouse_y, layout.content_x + layout.content_width - 252u, layout.body_y + 14u, 70u, 34u)) {
        if (g_file_manager.scroll >= layout.rows_visible) {
            g_file_manager.scroll -= layout.rows_visible;
        } else {
            g_file_manager.scroll = 0u;
        }
        return true;
    }
    if (point_in_rect(mouse_x, mouse_y, layout.content_x + layout.content_width - 174u, layout.body_y + 14u, 70u, 34u)) {
        if (g_file_manager.scroll + layout.rows_visible < g_file_manager.entry_count) {
            g_file_manager.scroll += layout.rows_visible;
        }
        return true;
    }
    if (point_in_rect(mouse_x, mouse_y, layout.content_x + layout.content_width - 96u, layout.body_y + 14u, 80u, 34u)) {
        if (g_file_manager.path[0] != '\0') {
            file_manager_step_up_path();
            g_file_manager.scroll = 0u;
            g_file_manager.selected_entry = 0u;
            file_manager_refresh_entries(state, boot_info);
        } else if (g_file_manager.sidebar == FILE_MANAGER_SIDEBAR_THIS_PC && !g_file_manager.show_this_pc_root) {
            g_file_manager.show_this_pc_root = true;
            file_manager_copy_string(g_file_manager.source_label, sizeof(g_file_manager.source_label), "This PC");
            file_manager_set_status("Pick a partition or browse the live media.");
            file_manager_refresh_entries(state, boot_info);
        } else {
            file_manager_set_status("You are already at the top of this location.");
        }
        return true;
    }

    for (u32 row = 0u; row < layout.rows_visible; ++row) {
        u32 entry_index = g_file_manager.scroll + row;
        u32 row_y = layout.rows_y + row * layout.row_height;

        if (entry_index >= g_file_manager.entry_count) {
            break;
        }
        if (point_in_rect(mouse_x, mouse_y, layout.content_x + 14u, row_y, layout.content_width - 28u, 34u)) {
            g_file_manager.selected_entry = entry_index;
            return file_manager_activate_entry(state, boot_info, &g_file_manager.entries[entry_index]);
        }
    }

    return false;
}

static bool handle_browser_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;

    if (state == NULL || !window_visible(state, WINDOW_BROWSER) || window_shaded(state, WINDOW_BROWSER)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_BROWSER);
    if (rect == NULL) {
        return false;
    }

    if (point_in_rect(mouse_x, mouse_y, rect->x + rect->width - 182u, rect->y + 54u, 72u, 34u)) {
        state->browser_page = BROWSER_PAGE_WEB;
        return browser_navigate_input(g_browser.address) || g_browser.address[0] != '\0';
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + rect->width - 100u, rect->y + 54u, 62u, 34u)) {
        state->browser_page = BROWSER_PAGE_DOWNLOADS;
        return browser_download_url(g_browser.address) || g_browser.address[0] != '\0';
    }

    return false;
}

static bool handle_notepad_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 panel_x, panel_y, panel_width, panel_height;

    if (state == NULL || !window_visible(state, WINDOW_NOTEPAD) || window_shaded(state, WINDOW_NOTEPAD)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_NOTEPAD);
    if (rect == NULL) {
        return false;
    }

    panel_x = rect->x + 8u;
    panel_y = rect->y + WINDOW_TITLEBAR_HEIGHT + 8u;
    panel_width = rect->width - 16u;
    panel_height = rect->height - WINDOW_TITLEBAR_HEIGHT - 16u;

    if (point_in_rect(mouse_x, mouse_y, panel_x + panel_width - 80u, panel_y + panel_height - 34u, 70u, 26u)) {
        notepad_save_to_disk(state);
        return true;
    }

    return false;
}

static bool handle_games_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 body_y;

    if (state == NULL || !window_visible(state, WINDOW_GAMES) || window_shaded(state, WINDOW_GAMES)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_GAMES);
    if (rect == NULL) {
        return false;
    }

    if (point_in_rect(mouse_x, mouse_y, rect->x + 20u, rect->y + 50u, 126u, 28u)) {
        state->games_screen = GAMES_SCREEN_HOME;
        return true;
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + 154u, rect->y + 50u, 126u, 28u)) {
        state->games_screen = GAMES_SCREEN_SNAKE;
        return true;
    }
    if (point_in_rect(mouse_x, mouse_y, rect->x + 288u, rect->y + 50u, 126u, 28u)) {
        state->games_screen = GAMES_SCREEN_TICTACTOE;
        return true;
    }

    body_y = rect->y + 92u;
    if (state->games_screen == GAMES_SCREEN_HOME) {
        if (point_in_rect(mouse_x, mouse_y, rect->x + 34u, body_y + 48u, 170u, 92u)) {
            open_window(state, WINDOW_SNAKE);
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, rect->x + 224u, body_y + 48u, 170u, 92u)) {
            state->games_screen = GAMES_SCREEN_TICTACTOE;
            return true;
        }
    } else if (state->games_screen == GAMES_SCREEN_SNAKE) {
        if (point_in_rect(mouse_x, mouse_y, rect->x + rect->width - 146u, body_y + 4u, 112u, OOBE_BUTTON_HEIGHT)) {
            snake_reset_state(state);
            return true;
        }
    } else if (state->games_screen == GAMES_SCREEN_TICTACTOE) {
        if (point_in_rect(mouse_x, mouse_y, rect->x + rect->width - 146u, body_y + 4u, 112u, OOBE_BUTTON_HEIGHT)) {
            ttt_reset(state);
            return true;
        }
        if (state->ttt_winner == 0u && !state->ttt_draw) {
            for (u32 index = 0; index < 9u; ++index) {
                u32 col = index % 3u;
                u32 row = index / 3u;
                u32 cell_x = rect->x + 42u + col * 66u;
                u32 cell_y = body_y + 24u + row * 66u;

                if (point_in_rect(mouse_x, mouse_y, cell_x, cell_y, 56u, 56u) && state->ttt_cells[index] == 0u) {
                    state->ttt_cells[index] = state->ttt_turn;
                    state->ttt_turn = state->ttt_turn == 1u ? 2u : 1u;
                    ttt_update_state(state);
                    return true;
                }
            }
        }
    }

    return false;
}

static bool handle_snake_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    const window_rect_t* rect;
    u32 body_y;

    if (state == NULL || !window_visible(state, WINDOW_SNAKE) || window_shaded(state, WINDOW_SNAKE)) {
        return false;
    }

    rect = get_window_rect_const(state, WINDOW_SNAKE);
    if (rect == NULL) {
        return false;
    }

    body_y = rect->y + 54u;
    if (point_in_rect(mouse_x, mouse_y, rect->x + rect->width - 146u, body_y + 8u, 112u, OOBE_BUTTON_HEIGHT)) {
        snake_reset_state(state);
        return true;
    }

    return false;
}

static bool handle_setup_click(desktop_state_t* state,
                               const boot_info_t* boot_info,
                               u32 current_second,
                               u32 mouse_x,
                               u32 mouse_y) {
    u32 panel_x;
    u32 panel_y;

    if (!setup_active(state)) {
        return false;
    }

    if (state->setup_phase == SETUP_PHASE_WELCOME) {
        panel_x = (display_driver_width() > 640u) ? (display_driver_width() - 640u) / 2u : 0u;
        panel_y = (display_driver_height() > 360u) ? (display_driver_height() - 360u) / 2u : 0u;

        if (point_in_rect(mouse_x, mouse_y, panel_x + 640u - 40u - 120u, panel_y + 360u - 40u - OOBE_BUTTON_HEIGHT, 120u, OOBE_BUTTON_HEIGHT)) {
            set_setup_phase(state, SETUP_PHASE_CHECKING_HARDWARE, current_second);
            return true;
        }
    } else if (state->setup_phase == SETUP_PHASE_INSTALLER) {
        panel_x = (display_driver_width() > 700u) ? (display_driver_width() - 700u) / 2u : 0u;
        panel_y = (display_driver_height() > 404u) ? (display_driver_height() - 404u) / 2u : 0u;

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

        if (point_in_rect(mouse_x, mouse_y, panel_x + 700u - 34u - 148u - 10u - 168u, panel_y + 356u, 168u, OOBE_BUTTON_HEIGHT)) {
            set_setup_phase(state, SETUP_PHASE_SKIPPING_INSTALLER, current_second);
            return true;
        }

        if (point_in_rect(mouse_x, mouse_y, panel_x + 700u - 34u - 148u, panel_y + 356u, 148u, OOBE_BUTTON_HEIGHT)) {
            u32 target_lba;
            u32 target_sectors;

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

            state->oobe_install_failed = !persist_state_for_target(INSTALL_STATE_STAGE_PENDING_INSTALL,
                                                                   target_lba,
                                                                   target_sectors,
                                                                   "",
                                                                   "");
            if (state->oobe_install_failed) {
                return true;
            }

            state->install_state_stage = INSTALL_STATE_STAGE_PENDING_INSTALL;
            state->install_state_target_lba = target_lba;
            state->install_state_target_sectors = target_sectors;
            set_setup_phase(state, SETUP_PHASE_INSTALLING, current_second);
            return true;
        }
    } else if (state->setup_phase == SETUP_PHASE_USER_OOBE) {
        u32 field_x;
        u32 field_y;
        u32 field_width;
        u32 field_height;

        get_setup_user_field_rect(AUTH_FIELD_USERNAME, &field_x, &field_y, &field_width, &field_height);
        if (point_in_rect(mouse_x, mouse_y, field_x, field_y, field_width, field_height)) {
            state->auth_field_focus = AUTH_FIELD_USERNAME;
            return true;
        }
        get_setup_user_field_rect(AUTH_FIELD_PASSWORD, &field_x, &field_y, &field_width, &field_height);
        if (point_in_rect(mouse_x, mouse_y, field_x, field_y, field_width, field_height)) {
            state->auth_field_focus = AUTH_FIELD_PASSWORD;
            return true;
        }

        panel_x = (display_driver_width() > 640u) ? (display_driver_width() - 640u) / 2u : 0u;
        panel_y = (display_driver_height() > 388u) ? (display_driver_height() - 388u) / 2u : 0u;

        if (point_in_rect(mouse_x, mouse_y, panel_x + 640u - 40u - 176u, panel_y + 338u, 176u, OOBE_BUTTON_HEIGHT)) {
            return begin_oobe_account_creation(state, current_second);
        }
    } else if (state->setup_phase == SETUP_PHASE_LOCKSCREEN) {
        u32 field_x;
        u32 field_y;
        u32 field_width;
        u32 field_height;

        if (!state->lockscreen_prompt_visible) {
            state->lockscreen_prompt_visible = true;
            state->lockscreen_prompt_started_ms = timer_ticks_ms();
            state->login_error = false;
            state->auth_field_focus = AUTH_FIELD_PASSWORD;
            return true;
        }

        get_lockscreen_password_rect(&field_x, &field_y, &field_width, &field_height);
        
        // Account for animation shift
        u64 now = timer_ticks_ms();
        u32 anim_duration = 200u;
        u32 anim_progress_q8 = 256u;
        if (now - state->lockscreen_prompt_started_ms < anim_duration) {
            anim_progress_q8 = (u32)(((now - state->lockscreen_prompt_started_ms) * 256u) / anim_duration);
        }
        u32 ui_y_offset = (30u * (256u - anim_progress_q8)) / 256u;
        field_y += 100u + ui_y_offset;

        if (point_in_rect(mouse_x, mouse_y, field_x, field_y, field_width, field_height)) {
            state->auth_field_focus = AUTH_FIELD_PASSWORD;
            return true;
        }
        if (point_in_rect(mouse_x, mouse_y, field_x + (field_width - 160u) / 2u, field_y + 82u, 160u, OOBE_BUTTON_HEIGHT)) {
            return try_unlock_lockscreen(state);
        }
        return true;
    }

    return false;
}

static bool update_setup_state(desktop_state_t* state, const boot_info_t* boot_info, u32 current_second) {
    if (state == NULL) {
        return false;
    }

    if (state->setup_phase == SETUP_PHASE_LOCKSCREEN) {
        if (state->lockscreen_prompt_visible) {
            u64 now = timer_ticks_ms();
            u32 anim_duration = 200u;
            if (now - state->lockscreen_prompt_started_ms < anim_duration) {
                return true;
            }
        }
    }

    if (state->setup_phase == SETUP_PHASE_INSTALLING) {
        u32 elapsed = seconds_since(state->setup_phase_started_second, current_second);

        if (!state->oobe_install_write_started && elapsed >= SETUP_INSTALL_COPY_SECONDS) {
            u32 kernel_size;
            bool installed;

            if (boot_info == NULL ||
                state->install_state_target_sectors < install_required_sectors(boot_info)) {
                state->oobe_install_complete = false;
                state->oobe_install_failed = true;
                state->oobe_install_write_started = false;
                return true;
            }

            kernel_size = (u32)(boot_info->kernel_end - boot_info->kernel_start);
            installed = installer_install_to_target((const void*)boot_info->kernel_start,
                                                    kernel_size,
                                                    (const void*)boot_info->initrd_start,
                                                    (u32)boot_info->initrd_size,
                                                    state->install_state_target_lba,
                                                    state->install_state_target_sectors);
            state->oobe_install_complete = installed;
            state->oobe_install_failed = !installed;
            state->oobe_install_write_started = true;
            if (installed) {
                state->oobe_disk_has_install = true;
                state->live_media_boot = false;
                state->install_state_stage = INSTALL_STATE_STAGE_NEEDS_OOBE;
                if (!persist_state_from_desktop(state, INSTALL_STATE_STAGE_NEEDS_OOBE, "", "")) {
                    state->oobe_install_complete = false;
                    state->oobe_install_failed = true;
                }
            }
            return true;
        }

        if (elapsed >= SETUP_INSTALL_SECONDS) {
            if (state->oobe_install_complete && !state->oobe_install_failed) {
                reboot_system();
            }
            return state->oobe_install_complete || state->oobe_install_failed;
        }
    }

    if (state->setup_phase == SETUP_PHASE_OOBE_HI &&
        seconds_since(state->setup_phase_started_second, current_second) >= SETUP_OOBE_HI_SECONDS) {
        set_setup_phase(state, SETUP_PHASE_OOBE_SETTING_UP, current_second);
        return true;
    }

    if (state->setup_phase == SETUP_PHASE_OOBE_SETTING_UP &&
        seconds_since(state->setup_phase_started_second, current_second) >= SETUP_OOBE_SETTING_UP_SECONDS) {
        set_setup_phase(state, SETUP_PHASE_OOBE_PREPARE, current_second);
        return true;
    }

    if (state->setup_phase == SETUP_PHASE_OOBE_PREPARE &&
        seconds_since(state->setup_phase_started_second, current_second) >= SETUP_OOBE_PREPARE_SECONDS) {
        set_setup_phase(state, SETUP_PHASE_OOBE_ALMOST_THERE, current_second);
        return true;
    }

    if (state->setup_phase == SETUP_PHASE_OOBE_ALMOST_THERE &&
        seconds_since(state->setup_phase_started_second, current_second) >= SETUP_OOBE_ALMOST_THERE_SECONDS) {
        if (!persist_state_from_desktop(state,
                                        INSTALL_STATE_STAGE_READY,
                                        state->account_username,
                                        state->account_password)) {
            state->login_error = true;
        } else {
            set_setup_phase(state, SETUP_PHASE_NONE, current_second);
        }
        return true;
    }

    if (state->setup_phase == SETUP_PHASE_CHECKING_HARDWARE &&
        seconds_since(state->setup_phase_started_second, current_second) >= 3u) {
        set_setup_phase(state, SETUP_PHASE_INSTALLER, current_second);
        return true;
    }

    if (state->setup_phase == SETUP_PHASE_SKIPPING_INSTALLER &&
        timer_ticks_ms() - state->setup_phase_started_ms >= 5000u) {
        set_setup_phase(state, SETUP_PHASE_NONE, current_second);
        return true;
    }

    return false;
}

static bool handle_window_button_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    window_id_t window;
    const window_rect_t* rect;
    window_button_t button;

    if (state == NULL) {
        return false;
    }

    window = find_top_window_at_point(state, mouse_x, mouse_y);
    if (window == WINDOW_NONE) {
        return false;
    }

    rect = get_window_rect_const(state, window);
    button = hit_test_window_button(state, rect, mouse_x, mouse_y);
    if (button == WINDOW_BUTTON_NONE) {
        return false;
    }

    state->dragging_window = WINDOW_NONE;
    if (button == WINDOW_BUTTON_CLOSE) {
        close_window(state, window);
    } else if (button == WINDOW_BUTTON_FULLSCREEN) {
        toggle_window_fullscreen(state, window);
    }
    return true;
}

static bool desktop_shortcut_contains(u32 shortcut_index, u32 mouse_x, u32 mouse_y) {
    u32 x;
    u32 y;
    u32 width;
    u32 height;

    get_desktop_shortcut_rect(shortcut_index, &x, &y, &width, &height);
    return point_in_rect(mouse_x, mouse_y, x, y, width, height);
}

static bool handle_window_right_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    window_id_t window;

    if (state == NULL) {
        return false;
    }

    window = find_top_window_at_point(state, mouse_x, mouse_y);
    if (window == WINDOW_NONE) {
        state->context_menu_open = true;
        state->context_menu_x = mouse_x;
        state->context_menu_y = mouse_y;
        state->context_menu_target_entry = -1; // Default to empty space

        // Check if we clicked on a desktop shortcut (user files)
        u32 shortcut_index = 0;
        // Skip fixed shortcuts (don't allow renaming/deleting them for now)
        for (u32 index = 0; index < DESKTOP_SHORTCUT_COUNT; ++index) {
            if (desktop_shortcut_contains(shortcut_index++, mouse_x, mouse_y)) {
                // Fixed shortcut - maybe show limited menu?
                return true;
            }
        }

        // Check user files
        for (u32 i = 0; i < USER_FILES_MAX; ++i) {
            if (g_user_files.entries[i].active) {
                if (desktop_shortcut_contains(shortcut_index++, mouse_x, mouse_y)) {
                    state->context_menu_open = true;
                    state->context_menu_x = mouse_x;
                    state->context_menu_y = mouse_y;
                    state->context_menu_target_entry = (s32)i + 1; // +1 to match File Manager's indexing logic
                    return true;
                }
            }
        }
        return true;
    }

    if (window == WINDOW_FILES) {
        window_rect_t* rect = get_window_rect(state, WINDOW_FILES);
        file_manager_layout_t layout;
        if (file_manager_get_layout(rect, &layout)) {
            // Check if right click is inside the content area
            if (point_in_rect(mouse_x, mouse_y, layout.content_x, layout.rows_y, layout.content_width, layout.body_height - (layout.rows_y - layout.body_y))) {
                state->context_menu_open = true;
                state->context_menu_x = mouse_x;
                state->context_menu_y = mouse_y;
                state->context_menu_target_entry = -1; // Default to empty space

                // Check if we clicked on an entry
                for (u32 row = 0u; row < layout.rows_visible; ++row) {
                    u32 entry_index = g_file_manager.scroll + row;
                    if (entry_index >= g_file_manager.entry_count) break;
                    
                    u32 row_y = layout.rows_y + row * layout.row_height;
                    if (point_in_rect(mouse_x, mouse_y, layout.content_x + 14u, row_y, layout.content_width - 28u, 34u)) {
                        state->context_menu_target_entry = (s32)entry_index;
                        break;
                    }
                }
                return true;
            }
        }
    }

    state->context_menu_open = false;
    return false;
}

static bool handle_desktop_shortcut_click(desktop_state_t* state, u32 mouse_x, u32 mouse_y) {
    if (state == NULL) {
        return false;
    }

    u32 shortcut_index = 0;

    // Fixed shortcuts
    for (u32 index = 0; index < DESKTOP_SHORTCUT_COUNT; ++index) {
        if (desktop_shortcut_contains(shortcut_index++, mouse_x, mouse_y)) {
            state->show_welcome = false;
            open_window(state, desktop_shortcut_window(index));
            return true;
        }
    }

    // User files shortcuts
    for (u32 i = 0; i < USER_FILES_MAX; ++i) {
        if (g_user_files.entries[i].active) {
            if (desktop_shortcut_contains(shortcut_index++, mouse_x, mouse_y)) {
                state->show_welcome = false;
                state->notepad_user_file_index = (s32)i;
                notepad_load_from_disk(state);
                open_window(state, WINDOW_NOTEPAD);
                return true;
            }
        }
    }

    return false;
}

static bool handle_window_left_click(desktop_state_t* state,
                                     const boot_info_t* boot_info,
                                     u32 mouse_x,
                                     u32 mouse_y) {
    window_id_t target;
    const window_rect_t* rect;

    if (state == NULL) {
        return false;
    }

    target = find_top_window_at_point(state, mouse_x, mouse_y);
    if (target == WINDOW_NONE) {
        return false;
    }

    bring_window_to_front(state, target);
    rect = get_window_rect_const(state, target);
    if (rect == NULL) {
        return false;
    }

    if (handle_window_button_click(state, mouse_x, mouse_y)) {
        return true;
    }

    if (target == WINDOW_SEARCH) {
        if (handle_launcher_click(state, mouse_x, mouse_y, rtc_read_seconds_of_day())) {
            return true;
        }
    } else if (target == WINDOW_SETTINGS) {
        if (handle_settings_click(state, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_NOTEPAD) {
        if (handle_notepad_click(state, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_FILES) {
        if (handle_file_manager_click(state, boot_info, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_TASKS) {
        if (handle_task_manager_click(state, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_VIDEOS) {
        if (handle_videos_click(state, boot_info, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_BROWSER) {
        if (handle_browser_click(state, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_GAMES) {
        if (handle_games_click(state, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_SNAKE) {
        if (handle_snake_click(state, mouse_x, mouse_y)) {
            return true;
        }
    } else if (target == WINDOW_WELCOME) {
        if (handle_oobe_click(state, boot_info, mouse_x, mouse_y)) {
            return true;
        }
    }

    if (point_in_window_title(state, mouse_x, mouse_y, rect, target)) {
        state->dragging_window = target;
        state->drag_offset_x = (s32)mouse_x - (s32)rect->x;
        state->drag_offset_y = (s32)mouse_y - (s32)rect->y;
    }

    return true;
}

static bool update_dragged_window(desktop_state_t* state, const mouse_state_t* mouse) {
    if (state == NULL || mouse == NULL || state->dragging_window == WINDOW_NONE || !mouse->left_down) {
        return false;
    }

    return move_dragged_window_to_mouse(state, mouse);
}

static void draw_desktop_scene(const image_t* wallpaper,
                               const boot_info_t* boot_info,
                               bool wallpaper_loaded,
                               bool ata_ready,
                               const desktop_state_t* state,
                               const mouse_state_t* mouse) {
    if (state != NULL && state->power_action != POWER_ACTION_NONE) {
        render_power_action_screen(state->power_action);
        return;
    }

    if (setup_active(state)) {
        render_setup_screen(state, wallpaper, boot_info, rtc_read_seconds_of_day());
        return;
    }

    draw_desktop_shell(wallpaper, state, mouse);
    draw_desktop_windows(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, NULL);
}

static void render_desktop(const image_t* wallpaper,
                           const boot_info_t* boot_info,
                           bool wallpaper_loaded,
                           bool ata_ready,
                           const desktop_state_t* state,
                           const mouse_state_t* mouse,
                           u32 current_clock_second) {
    if (state != NULL && state->power_action != POWER_ACTION_NONE) {
        render_power_action_screen(state->power_action);
        return;
    }

    if (!setup_active(state) && prepare_desktop_shell_cache(wallpaper, state, mouse, current_clock_second)) {
        draw_desktop_windows(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, NULL);
        draw_context_menu(state, mouse);
    } else {
        draw_desktop_scene(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, mouse);
    }

    display_driver_present();
    if (mouse != NULL) {
        draw_mouse_cursor_front(mouse->x, mouse->y);
    }
}

static void merge_dirty_rect(window_rect_t* target, const window_rect_t* rect) {
    u32 target_end_x;
    u32 target_end_y;
    u32 rect_end_x;
    u32 rect_end_y;
    u32 merged_end_x;
    u32 merged_end_y;

    if (target == NULL || rect == NULL || rect->width == 0u || rect->height == 0u) {
        return;
    }

    if (target->width == 0u || target->height == 0u) {
        *target = *rect;
        return;
    }

    target_end_x = target->x + target->width;
    target_end_y = target->y + target->height;
    rect_end_x = rect->x + rect->width;
    rect_end_y = rect->y + rect->height;

    if (rect->x < target->x) {
        target->x = rect->x;
    }
    if (rect->y < target->y) {
        target->y = rect->y;
    }

    merged_end_x = rect_end_x > target_end_x ? rect_end_x : target_end_x;
    merged_end_y = rect_end_y > target_end_y ? rect_end_y : target_end_y;
    target->width = merged_end_x - target->x;
    target->height = merged_end_y - target->y;
}

static bool dirty_rects_overlap_or_touch(const window_rect_t* left, const window_rect_t* right, u32 padding) {
    u32 left_end_x;
    u32 left_end_y;
    u32 right_end_x;
    u32 right_end_y;

    if (left == NULL || right == NULL || left->width == 0u || left->height == 0u || right->width == 0u || right->height == 0u) {
        return false;
    }

    left_end_x = left->x + left->width;
    left_end_y = left->y + left->height;
    right_end_x = right->x + right->width;
    right_end_y = right->y + right->height;

    return !(left_end_x + padding < right->x ||
             right_end_x + padding < left->x ||
             left_end_y + padding < right->y ||
             right_end_y + padding < left->y);
}

static bool window_rects_intersect(const window_rect_t* left, const window_rect_t* right) {
    u32 left_end_x;
    u32 left_end_y;
    u32 right_end_x;
    u32 right_end_y;

    if (left == NULL || right == NULL || left->width == 0u || left->height == 0u || right->width == 0u || right->height == 0u) {
        return false;
    }

    left_end_x = left->x + left->width;
    left_end_y = left->y + left->height;
    right_end_x = right->x + right->width;
    right_end_y = right->y + right->height;

    return !(left_end_x <= right->x ||
             right_end_x <= left->x ||
             left_end_y <= right->y ||
             right_end_y <= left->y);
}

static bool dirty_rect_list_intersects_rect(const dirty_rect_list_t* list, const window_rect_t* rect) {
    if (rect == NULL || rect->width == 0u || rect->height == 0u) {
        return false;
    }

    if (list == NULL || list->count == 0u) {
        return true;
    }

    for (u32 index = 0; index < list->count; ++index) {
        if (window_rects_intersect(&list->rects[index], rect)) {
            return true;
        }
    }

    return false;
}

static void dirty_rect_list_add(dirty_rect_list_t* list, const window_rect_t* rect) {
    if (list == NULL || rect == NULL || rect->width == 0u || rect->height == 0u) {
        return;
    }

    for (u32 index = 0; index < list->count; ++index) {
        if (dirty_rects_overlap_or_touch(&list->rects[index], rect, 18u)) {
            merge_dirty_rect(&list->rects[index], rect);
            return;
        }
    }

    if (list->count < DIRTY_RECT_MAX) {
        list->rects[list->count] = *rect;
        ++list->count;
        return;
    }

    merge_dirty_rect(&list->rects[0], rect);
}

static bool window_rect_matches(const window_rect_t* left, const window_rect_t* right) {
    return left != NULL && right != NULL &&
           left->x == right->x &&
           left->y == right->y &&
           left->width == right->width &&
           left->height == right->height;
}

static bool get_window_present_rect(const desktop_state_t* state, window_id_t window, window_rect_t* out_rect) {
    const window_rect_t* rect;

    if (state == NULL || out_rect == NULL) {
        return false;
    }

    rect = get_window_rect_const(state, window);
    if (rect == NULL) {
        return false;
    }

    *out_rect = *rect;
    if (window_shaded(state, window)) {
        out_rect->height = shaded_window_height(window);
    }
    return true;
}

static bool expand_rect_with_padding(const window_rect_t* rect, u32 padding, window_rect_t* out_rect) {
    u32 start_x;
    u32 start_y;
    u32 end_x;
    u32 end_y;

    if (rect == NULL || out_rect == NULL || rect->width == 0u || rect->height == 0u) {
        return false;
    }

    start_x = rect->x > padding ? rect->x - padding : 0u;
    start_y = rect->y > padding ? rect->y - padding : 0u;
    end_x = rect->x + rect->width + padding;
    end_y = rect->y + rect->height + padding;

    if (end_x > display_driver_width() || end_x < rect->x) {
        end_x = display_driver_width();
    }
    if (end_y > display_driver_height() || end_y < rect->y) {
        end_y = display_driver_height();
    }
    if (end_x <= start_x || end_y <= start_y) {
        return false;
    }

    out_rect->x = start_x;
    out_rect->y = start_y;
    out_rect->width = end_x - start_x;
    out_rect->height = end_y - start_y;
    return true;
}

static bool get_window_composite_rect(const desktop_state_t* state, window_id_t window, window_rect_t* out_rect) {
    window_rect_t present_rect;

    if (!get_window_present_rect(state, window, &present_rect)) {
        return false;
    }

    return expand_rect_with_padding(&present_rect, WINDOW_COMPOSITE_PADDING, out_rect);
}

static bool videos_playback_needs_redraw(const desktop_state_t* state, window_rect_t* out_rect) {
    u32 next_frame_index;
    u32 frame_count = 0u;

    if (state == NULL || !state->videos_playing || !window_visible(state, WINDOW_VIDEOS)) {
        return false;
    }

    if (state->videos_active_frame_count != 0u) {
        frame_count = state->videos_active_frame_count;
    } else if (state->videos_item_count != 0u && state->videos_selected_item < state->videos_item_count) {
        frame_count = state->videos_item_frame_count[state->videos_selected_item];
    }

    if (frame_count <= 1u) {
        return false;
    }

    next_frame_index = videos_current_frame_index(state);
    if (next_frame_index == state->videos_last_frame_index) {
        return false;
    }

    if (out_rect != NULL) {
        return videos_get_preview_present_rect(state, out_rect);
    }
    return true;
}

static void snapshot_window_state(const desktop_state_t* state,
                                  bool visible[WINDOW_STACK_SIZE + 1u],
                                  window_rect_t rects[WINDOW_STACK_SIZE + 1u],
                                  window_id_t order[WINDOW_STACK_SIZE],
                                  u32* dock_mask) {
    if (visible == NULL || rects == NULL || order == NULL) {
        return;
    }

    memset(visible, 0, sizeof(bool) * (WINDOW_STACK_SIZE + 1u));
    memset(rects, 0, sizeof(window_rect_t) * (WINDOW_STACK_SIZE + 1u));

    if (state == NULL) {
        memset(order, 0, sizeof(window_id_t) * WINDOW_STACK_SIZE);
        if (dock_mask != NULL) {
            *dock_mask = 0u;
        }
        return;
    }

    memcpy(order, state->window_order, sizeof(window_id_t) * WINDOW_STACK_SIZE);
    if (dock_mask != NULL) {
        *dock_mask = desktop_shell_dock_mask(state);
    }

    for (u32 window = WINDOW_SEARCH; window <= WINDOW_LEGAL; ++window) {
        visible[window] = window_visible(state, (window_id_t)window);
        if (visible[window]) {
            (void)get_window_composite_rect(state, (window_id_t)window, &rects[window]);
        }
    }
}

static bool compute_dirty_rects_from_snapshot(const desktop_state_t* state,
                                              const bool visible_before[WINDOW_STACK_SIZE + 1u],
                                              const window_rect_t rects_before[WINDOW_STACK_SIZE + 1u],
                                              const window_id_t order_before[WINDOW_STACK_SIZE],
                                              u32 dock_mask_before,
                                              dirty_rect_list_t* out_rects) {
    bool order_changed = false;

    if (state == NULL || visible_before == NULL || rects_before == NULL || order_before == NULL || out_rects == NULL) {
        return false;
    }

    for (u32 index = 0; index < WINDOW_STACK_SIZE; ++index) {
        if (order_before[index] != state->window_order[index]) {
            order_changed = true;
            break;
        }
    }

    if (order_changed) {
        for (u32 window = WINDOW_SEARCH; window <= WINDOW_LEGAL; ++window) {
            window_rect_t current_rect = {0u, 0u, 0u, 0u};
            bool visible_now = window_visible(state, (window_id_t)window);
            bool order_slot_changed = false;

            for (u32 index = 0; index < WINDOW_STACK_SIZE; ++index) {
                if (order_before[index] == (window_id_t)window ||
                    state->window_order[index] == (window_id_t)window) {
                    if (order_before[index] != state->window_order[index]) {
                        order_slot_changed = true;
                        break;
                    }
                }
            }

            if (visible_before[window] != visible_now || order_slot_changed) {
                if (visible_before[window]) {
                    dirty_rect_list_add(out_rects, &rects_before[window]);
                }
                if (visible_now && get_window_composite_rect(state, (window_id_t)window, &current_rect)) {
                    dirty_rect_list_add(out_rects, &current_rect);
                }
            }
        }
    } else {
        for (u32 window = WINDOW_SEARCH; window <= WINDOW_LEGAL; ++window) {
            window_rect_t current_rect = {0u, 0u, 0u, 0u};
            bool visible_now = window_visible(state, (window_id_t)window);
            bool rect_changed = false;

            if (visible_now) {
                rect_changed = get_window_composite_rect(state, (window_id_t)window, &current_rect) &&
                               !window_rect_matches(&rects_before[window], &current_rect);
            }

            if (visible_before[window] != visible_now || rect_changed) {
                if (visible_before[window]) {
                    dirty_rect_list_add(out_rects, &rects_before[window]);
                }
                if (visible_now) {
                    dirty_rect_list_add(out_rects, &current_rect);
                }
            }
        }
    }

    if (dock_mask_before != desktop_shell_dock_mask(state)) {
        dock_layout_t layout;
        window_rect_t dock_rect;

        get_dock_layout(state, &layout);
        dock_rect.x = layout.dock_x;
        dock_rect.y = layout.dock_y;
        dock_rect.width = layout.dock_width;
        dock_rect.height = layout.dock_height;
        dirty_rect_list_add(out_rects, &dock_rect);
    }

    return out_rects->count != 0u;
}

static bool desktop_shell_cache_is_current(const desktop_state_t* state, u32 current_clock_second) {
    u32 framebuffer_bytes;

    if (state == NULL || g_desktop_shell_cache == NULL) {
        return false;
    }

    framebuffer_bytes = display_driver_framebuffer_bytes();
    if (framebuffer_bytes == 0u || g_desktop_shell_cache_size != framebuffer_bytes) {
        return false;
    }

    return g_desktop_shell_clock_second == current_clock_second &&
           g_desktop_shell_theme == state->desktop_theme &&
           g_desktop_shell_wallpaper_theme == state->wallpaper_theme &&
           g_desktop_shell_dock_mask == desktop_shell_dock_mask(state);
}

static void restore_desktop_shell_cache_regions(const dirty_rect_list_t* dirty_rects) {
    if (dirty_rects == NULL || dirty_rects->count == 0u || g_desktop_shell_cache == NULL || g_desktop_shell_cache_size == 0u) {
        return;
    }

    for (u32 index = 0; index < dirty_rects->count; ++index) {
        const window_rect_t* rect = &dirty_rects->rects[index];
        display_driver_restore_drawbuffer_surface_region(g_desktop_shell_cache,
                                                         g_desktop_shell_cache_size,
                                                         rect->x,
                                                         rect->y,
                                                         rect->width,
                                                         rect->height);
    }
}

static void render_dirty_window_region(const image_t* wallpaper,
                                       const boot_info_t* boot_info,
                                       bool wallpaper_loaded,
                                       bool ata_ready,
                                       const desktop_state_t* state,
                                       const mouse_state_t* mouse,
                                       const window_rect_t* dirty_rect) {
    dirty_rect_list_t region_list = {};

    if (dirty_rect == NULL || dirty_rect->width == 0u || dirty_rect->height == 0u || mouse == NULL) {
        return;
    }

    region_list.rects[0] = *dirty_rect;
    region_list.count = 1u;

    restore_desktop_shell_cache_regions(&region_list);
    display_driver_set_clip_rect(dirty_rect->x, dirty_rect->y, dirty_rect->width, dirty_rect->height);
    draw_desktop_windows(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, &region_list);
    draw_context_menu(state, mouse);
    display_driver_clear_clip_rect();
}

static void render_desktop_partial(const image_t* wallpaper,
                                   const boot_info_t* boot_info,
                                   bool wallpaper_loaded,
                                   bool ata_ready,
                                   const desktop_state_t* state,
                                   const mouse_state_t* mouse,
                                   const dirty_rect_list_t* dirty_rects,
                                   u32 current_clock_second) {
    u32 rendered_region_count = 0u;

    if (dirty_rects == NULL || dirty_rects->count == 0u) {
        render_desktop(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, mouse, current_clock_second);
        return;
    }

    if (setup_active(state)) {
        render_desktop(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, mouse, current_clock_second);
        return;
    }
    if (!desktop_shell_cache_is_current(state, current_clock_second)) {
        if (g_desktop_shell_theme != state->desktop_theme ||
            !prepare_desktop_shell_cache(wallpaper, state, mouse, current_clock_second)) {
            render_desktop(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, mouse, current_clock_second);
            return;
        }
    }

    for (u32 index = 0; index < dirty_rects->count; ++index) {
        window_rect_t composite_rect;

        if (!expand_rect_with_padding(&dirty_rects->rects[index], 0u, &composite_rect)) {
            continue;
        }
        render_dirty_window_region(wallpaper,
                                   boot_info,
                                   wallpaper_loaded,
                                   ata_ready,
                                   state,
                                   mouse,
                                   &composite_rect);
        display_driver_present_region(composite_rect.x,
                                      composite_rect.y,
                                      composite_rect.width,
                                      composite_rect.height);
        ++rendered_region_count;
    }

    if (rendered_region_count == 0u) {
        render_desktop(wallpaper, boot_info, wallpaper_loaded, ata_ready, state, mouse, current_clock_second);
        return;
    }

    if (mouse != NULL) {
        draw_mouse_cursor_front(mouse->x, mouse->y);
    }
}

static void refresh_cursor_only(u32 previous_x,
                                u32 previous_y,
                                const mouse_state_t* mouse) {
    u32 current_x;
    u32 current_y;
    window_rect_t previous_rect = {0u, 0u, 0u, 0u};
    window_rect_t current_rect = {0u, 0u, 0u, 0u};

    if (mouse == NULL) {
        return;
    }

    current_x = mouse->x;
    current_y = mouse->y;
    if (!get_cursor_draw_rect(current_x, current_y, &current_rect)) {
        return;
    }

    if (get_cursor_draw_rect(previous_x, previous_y, &previous_rect)) {
        display_driver_restore_frontbuffer_from_drawbuffer_region(previous_rect.x,
                                                                  previous_rect.y,
                                                                  previous_rect.width,
                                                                  previous_rect.height);
    }
    if (current_x != previous_x || current_y != previous_y) {
        display_driver_restore_frontbuffer_from_drawbuffer_region(current_rect.x,
                                                                  current_rect.y,
                                                                  current_rect.width,
                                                                  current_rect.height);
    }
    draw_mouse_cursor_front(current_x, current_y);
}

static void draw_desktop_shell(const image_t* wallpaper,
                               const desktop_state_t* state,
                               const mouse_state_t* mouse) {
    draw_desktop_background(state, wallpaper);
    draw_top_bar(state);
    draw_desktop_shortcuts(state);
    draw_dock(state, mouse);
    draw_preview_label();
}

static void draw_desktop_windows(const image_t* wallpaper,
                                 const boot_info_t* boot_info,
                                 bool wallpaper_loaded,
                                 bool ata_ready,
                                 const desktop_state_t* state,
                                 const dirty_rect_list_t* dirty_rects) {
    (void)wallpaper;

    for (u32 index = 0; index < WINDOW_STACK_SIZE; ++index) {
        window_id_t window = state->window_order[index];
        const window_rect_t* rect = get_window_rect_const(state, window);
        window_rect_t composite_rect = {0u, 0u, 0u, 0u};

        if (window == state->dragging_window || !window_visible(state, window)) {
            continue;
        }
        if (!get_window_composite_rect(state, window, &composite_rect) ||
            !dirty_rect_list_intersects_rect(dirty_rects, &composite_rect)) {
            continue;
        }

        if (window == WINDOW_SEARCH) {
            draw_launcher_panel(state, rect);
        } else if (window == WINDOW_RUN) {
            draw_run_dialog_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_SETTINGS) {
            draw_settings_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_NOTEPAD) {
            draw_notepad_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_FILES) {
            draw_file_manager_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_TASKS) {
            draw_task_manager_window(boot_info, wallpaper_loaded, ata_ready, state, rect, window_shaded(state, window));
        } else if (window == WINDOW_VIDEOS) {
            draw_videos_window(state, rect, boot_info, window_shaded(state, window));
        } else if (window == WINDOW_BROWSER) {
            draw_browser_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_GAMES) {
            draw_games_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_SNAKE) {
            draw_snake_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_JABVER) {
            draw_jabver_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_LEGAL) {
            draw_legal_window(state, rect, window_shaded(state, window));
        } else if (window == WINDOW_WELCOME) {
            draw_oobe_window(state, rect);
        }
    }

    if (state->dragging_window != WINDOW_NONE && window_visible(state, state->dragging_window)) {
        const window_rect_t* rect = get_window_rect_const(state, state->dragging_window);
        window_rect_t composite_rect = {0u, 0u, 0u, 0u};

        if (!get_window_composite_rect(state, state->dragging_window, &composite_rect) ||
            !dirty_rect_list_intersects_rect(dirty_rects, &composite_rect)) {
            goto update_prompt;
        }

        draw_dragged_window_preview(state, state->dragging_window, rect);
    }

update_prompt:
    draw_update_prompt_overlay(state, dirty_rects);
}

static bool prepare_desktop_shell_cache(const image_t* wallpaper,
                                        const desktop_state_t* state,
                                        const mouse_state_t* mouse,
                                        u32 current_clock_second) {
    u32 framebuffer_bytes;
    u32 dock_mask;

    if (state == NULL || mouse == NULL) {
        return false;
    }

    framebuffer_bytes = display_driver_framebuffer_bytes();
    if (framebuffer_bytes == 0u) {
        return false;
    }

    if (g_desktop_shell_cache == NULL || g_desktop_shell_cache_size != framebuffer_bytes) {
        g_desktop_shell_cache = allocate_surface_buffer(framebuffer_bytes);
        if (g_desktop_shell_cache == NULL) {
            g_desktop_shell_cache_size = 0u;
            return false;
        }
        g_desktop_shell_cache_size = framebuffer_bytes;
        g_desktop_shell_clock_second = 0xFFFFFFFFu;
        g_desktop_shell_theme = 0xFFFFFFFFu;
        g_desktop_shell_wallpaper_theme = 0xFFFFFFFFu;
        g_desktop_shell_dock_mask = 0xFFFFFFFFu;
    }

    dock_mask = desktop_shell_dock_mask(state);
    if (g_desktop_shell_clock_second != current_clock_second ||
        g_desktop_shell_theme != state->desktop_theme ||
        g_desktop_shell_wallpaper_theme != state->wallpaper_theme ||
        g_desktop_shell_dock_mask != dock_mask) {
        draw_desktop_shell(wallpaper, state, mouse);
        display_driver_copy_drawbuffer(g_desktop_shell_cache, framebuffer_bytes);
        g_desktop_shell_clock_second = current_clock_second;
        g_desktop_shell_theme = state->desktop_theme;
        g_desktop_shell_wallpaper_theme = state->wallpaper_theme;
        g_desktop_shell_dock_mask = dock_mask;
    } else {
        display_driver_restore_drawbuffer(g_desktop_shell_cache, framebuffer_bytes);
    }

    return true;
}

/*
static void refresh_desktop_chrome(const desktop_state_t* state, const mouse_state_t* mouse) {
    dock_layout_t layout;

    draw_top_bar(state);
    draw_dock(state, mouse);
    draw_preview_label();

    display_driver_present_region(0, 0, display_driver_width(), TOP_BAR_HEIGHT);
    get_dock_layout(state, &layout);
    display_driver_present_region(layout.dock_x, layout.dock_y, layout.dock_width, layout.dock_height);
    draw_mouse_cursor_front(mouse->x, mouse->y);
}
*/

/*
static void render_boot_splash_screen(const image_t* splash_image) {
    u32 screen_height = vga_height();
    u32 prompt_y;

    prompt_y = screen_height > 72u ? screen_height - 44u : screen_height / 2u;

    if (splash_image != NULL) {
        image_blit_stretch_bilinear(splash_image);
        vga_fill_rect_alpha(0, prompt_y > 12u ? prompt_y - 12u : 0u,
                            vga_width(),
                            28u,
                            0, 0, 0, 72);
    } else {
        vga_clear(vga_color(0, 0, 0));
    }
    draw_text_centered(prompt_y, "Press any key to start", vga_color(255, 255, 255));
    vga_present();
}

static void render_boot_bsod_screen(void) {
    u32 screen_height = vga_height();
    u32 title_y = screen_height > 180u ? (screen_height / 2u) - 32u : screen_height / 2u;
    u32 detail_y = screen_height > 80u ? screen_height - 72u : title_y + 24u;
    u32 prompt_y = screen_height > 52u ? screen_height - 44u : detail_y + 20u;
    u32 white = vga_color(255, 255, 255);

    vga_clear(vga_color(0, 0, 170));
    draw_text_centered(title_y, "Not our fault!", white);
    draw_text_centered(detail_y,
                       "The system has detected you messing with it! It has decided to BSOD",
                       white);
    draw_text_centered(prompt_y, "Press any key to reboot", white);
    vga_present();
}
*/

void gui_boot_log_reset(void) {
    memset(g_boot_log_lines, 0, sizeof(g_boot_log_lines));
    g_boot_log_count = 0;
}

void gui_boot_log_append(const char* text) {
    if (g_boot_log_count < BOOT_LOG_MAX_LINES) {
        copy_boot_log_line(g_boot_log_lines[g_boot_log_count], text);
        ++g_boot_log_count;
    } else {
        memmove(g_boot_log_lines, g_boot_log_lines[1], sizeof(g_boot_log_lines[0]) * (BOOT_LOG_MAX_LINES - 1u));
        copy_boot_log_line(g_boot_log_lines[BOOT_LOG_MAX_LINES - 1u], text);
    }
}

void gui_show_boot_sequence(const boot_info_t* boot_info) {
    if (boot_info != NULL && boot_info->initrd_start != 0 && boot_info->initrd_size != 0) {
        vga_clear(vga_color(0, 0, 0));

        initrd_file_t splash_file;
        if (initrd_find_file((const void*)boot_info->initrd_start, (u32)boot_info->initrd_size, "JabulSplash.bmp", &splash_file)) {
            image_t splash_img;
            if (image_load_any(splash_file.data, splash_file.size, &splash_img)) {
                u32 sw = vga_width();
                u32 sh = vga_height();
                u32 icon_size = 300; // A bit small as requested
                u32 icon_x = (sw - icon_size) / 2;
                u32 icon_y = (sh - icon_size) / 2 - 50;

                u32 bar_width = sw / 4;
                u32 bar_height = 8;
                u32 bar_x = (sw - bar_width) / 2;
                u32 bar_y = icon_y + icon_size + 60;

                u64 start_time = timer_ticks_ms();
                u64 duration = 5000;

                while (timer_ticks_ms() - start_time < duration) {
                    if (boot_poll_keyboard_input(false) == BOOT_INPUT_ANY_KEY) {
                        run_boot_chaos_mode();
                    }

                    vga_clear(vga_color(0, 0, 0));
                    image_blit_fit_rounded(&splash_img, icon_x, icon_y, icon_size, icon_size, 0);

                    // Draw loading bar background
                    draw_rect(bar_x, bar_y, bar_width, bar_height, vga_color(40, 40, 40));

                    // Draw progress
                    u64 elapsed = timer_ticks_ms() - start_time;
                    u32 progress_width = (u32)((elapsed * bar_width) / duration);
                    if (progress_width > bar_width) progress_width = bar_width;

                    draw_rect(bar_x, bar_y, progress_width, bar_height, vga_color(255, 255, 255));

                    vga_present();

                    // Small delay to maintain ~60fps and allow keyboard polling
                    u64 frame_wait = timer_ticks_ms();
                    while (timer_ticks_ms() - frame_wait < 16) {
                        if (boot_poll_keyboard_input(false) == BOOT_INPUT_ANY_KEY) {
                            run_boot_chaos_mode();
                        }
                    }
                }

                // Splash Logo Growth Animation
                u64 grow_start = timer_ticks_ms();
                u64 grow_duration = 600; // slightly longer for stuff
                while (timer_ticks_ms() - grow_start < grow_duration) {
                    u64 elapsed = timer_ticks_ms() - grow_start;
                    // Grow from icon_size to icon_size * 1.5
                    u32 current_grow_size = icon_size + (u32)((elapsed * (icon_size / 2)) / grow_duration);
                    u32 current_x = (sw - current_grow_size) / 2;
                    u32 current_y = (sh - current_grow_size) / 2 - 50;

                    vga_clear(vga_color(0, 0, 0));
                    image_blit_fit_rounded(&splash_img, current_x, current_y, current_grow_size, current_grow_size, 0);
                    vga_present();

                    // Increased FPS by reducing wait to 4ms (~250fps target, drawing will bottleneck)
                    u64 frame_wait = timer_ticks_ms();
                    while (timer_ticks_ms() - frame_wait < 4) {}
                }

                // Flash and Fade Out Animation
                u64 fade_duration = 400; // 0.4 seconds fade
                u64 fade_start = timer_ticks_ms();
                
                while (timer_ticks_ms() - fade_start < fade_duration) {
                    u64 elapsed = timer_ticks_ms() - fade_start;
                    u8 alpha = (u8)(255 - (elapsed * 255) / fade_duration);
                    
                    // We start with a full white screen and fade to black
                    vga_clear(vga_color(alpha, alpha, alpha));
                    vga_present();

                    u64 frame_wait = timer_ticks_ms();
                    while (timer_ticks_ms() - frame_wait < 8) {}
                }
                
                vga_clear(vga_color(0, 0, 0));
                vga_present();
            }
        }
    }
}

void gui_run_desktop(const image_t* wallpaper,
                     const image_t* boot_splash_image,
                     const image_t* splash_logo_image,
                     const image_t* lock_bg,
                     const image_t* lock_profile,
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
    desktop_initialize(&state, boot_info, boot_splash_image, splash_logo_image, ata_ready,
                       boot_info != NULL && boot_info->initrd_start != 0, last_clock_second);
    state.lockscreen_background = lock_bg;
    state.lockscreen_profile = lock_profile;
    initialize_theme_image_cache(boot_info, wallpaper);
    initialize_app_icon_cache(boot_info);
    mouse_driver_initialize(&mouse);
    previous_cursor_x = mouse.x;
    previous_cursor_y = mouse.y;

    for (;;) {
        bool input_changed;
        bool clock_changed;
        bool mouse_moved;
        bool mouse_pressed;
        bool mouse_released;
        bool right_pressed;
        bool right_released;
        bool previous_down = mouse.left_down;
        bool previous_right_down = mouse.right_down;
        bool window_visible_before[WINDOW_STACK_SIZE + 1u];
        window_rect_t window_rects_before[WINDOW_STACK_SIZE + 1u];
        window_id_t window_order_before[WINDOW_STACK_SIZE];
        bool full_present_required = false;
        bool partial_present_valid = false;
        dock_app_t clicked_app;
        u32 current_clock_second;
        u32 dock_mask_before = 0u;
        dirty_rect_list_t partial_present_rects = {};
        window_rect_t previous_drag_rect = {0u, 0u, 0u, 0u};
        bool had_drag_rect = false;

        snapshot_window_state(&state, window_visible_before, window_rects_before, window_order_before, &dock_mask_before);
        input_changed = poll_input(&state, &mouse);
        net_service();
        net_transport_service();
        if (state.crash_hotkey_triggered) {
            run_desktop_bsod_sequence();
        }
        current_clock_second = rtc_read_seconds_of_day();
        clock_changed = current_clock_second != last_clock_second;
        if (clock_changed) {
            scene_dirty = true;
        }
        update_power_action_state(&state, current_clock_second);
        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state)) {
        }
        if (window_visible(&state, WINDOW_TASKS) &&
            !window_minimized(&state, WINDOW_TASKS) &&
            elapsed_seconds_since(state.cpu_last_benchmark_second, current_clock_second) >= 15u) {
            cpu_run_benchmark(&state);
            current_clock_second = rtc_read_seconds_of_day();
            clock_changed = current_clock_second != last_clock_second;
            scene_dirty = true;
        }
        mouse_moved = mouse.x != previous_cursor_x || mouse.y != previous_cursor_y;
        mouse_pressed = mouse.left_down && !previous_down;
        mouse_released = !mouse.left_down && previous_down;
        right_pressed = mouse.right_down && !previous_right_down;
        right_released = !mouse.right_down && previous_right_down;

        if (state.power_action != POWER_ACTION_NONE) {
            scene_dirty = true;
            full_present_required = true;
        }

        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state)) {
            window_rect_t media_rect = {0u, 0u, 0u, 0u};

            startup_sound_tick(&state);
            videos_audio_tick(&state);

            if (videos_playback_needs_redraw(&state, &media_rect)) {
                scene_dirty = true;
                dirty_rect_list_add(&partial_present_rects, &media_rect);
                partial_present_valid = true;
            }
        } else if (state.power_action == POWER_ACTION_NONE) {
            startup_sound_tick(&state);
        }

        if (state.power_action == POWER_ACTION_NONE && update_setup_state(&state, boot_info, current_clock_second)) {
            scene_dirty = true;
            full_present_required = true;
        }
        if (state.power_action == POWER_ACTION_NONE && setup_active(&state) && clock_changed) {
            scene_dirty = true;
            full_present_required = true;
        }
        if (state.power_action == POWER_ACTION_NONE && mouse_pressed) {
            bool context_menu_was_open = state.context_menu_open;
            if (handle_context_menu_click(&state, boot_info, mouse.x, mouse.y)) {
                scene_dirty = true;
                full_present_required = true;
            } else if (context_menu_was_open) {
                // Clicked outside the context menu, it closed, need to redraw
                scene_dirty = true;
                full_present_required = true;
            } else if (handle_update_prompt_click(&state, mouse.x, mouse.y)) {
                scene_dirty = true;
                full_present_required = true;
            } else if (setup_active(&state)) {
                if (handle_setup_click(&state, boot_info, current_clock_second, mouse.x, mouse.y)) {
                    scene_dirty = true;
                    full_present_required = true;
                }
            } else if (dock_hit_test(&state, mouse.x, mouse.y, &clicked_app)) {
                if (toggle_dock_app(&state, clicked_app)) {
                    scene_dirty = true;
                }
            } else if (handle_window_left_click(&state, boot_info, mouse.x, mouse.y)) {
                scene_dirty = true;
            } else if (handle_desktop_shortcut_click(&state, mouse.x, mouse.y)) {
                scene_dirty = true;
            }
        }

        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state) && right_pressed) {
            if (handle_window_right_click(&state, mouse.x, mouse.y)) {
                scene_dirty = true;
                full_present_required = true;
            }
        }

        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state) && state.dragging_window != WINDOW_NONE) {
            const window_rect_t* drag_rect = get_window_rect_const(&state, state.dragging_window);
            if (drag_rect != NULL) {
                previous_drag_rect = *drag_rect;
                had_drag_rect = true;
            }
        }

        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state) && update_dragged_window(&state, &mouse)) {
            scene_dirty = true;
            if (had_drag_rect) {
                window_rect_t previous_drag_composite = {0u, 0u, 0u, 0u};
                if (expand_rect_with_padding(&previous_drag_rect, WINDOW_COMPOSITE_PADDING, &previous_drag_composite)) {
                    dirty_rect_list_add(&partial_present_rects, &previous_drag_composite);
                }
            }
            if (state.dragging_window != WINDOW_NONE) {
                const window_rect_t* drag_rect = get_window_rect_const(&state, state.dragging_window);
                if (drag_rect != NULL) {
                    window_rect_t drag_composite = {0u, 0u, 0u, 0u};
                    if (expand_rect_with_padding(drag_rect, WINDOW_COMPOSITE_PADDING, &drag_composite)) {
                        dirty_rect_list_add(&partial_present_rects, &drag_composite);
                    }
                }
            }
            partial_present_valid = partial_present_rects.count != 0u;
        }

        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state) && mouse_released && state.dragging_window != WINDOW_NONE) {
            if (had_drag_rect) {
                window_rect_t previous_drag_composite = {0u, 0u, 0u, 0u};
                if (expand_rect_with_padding(&previous_drag_rect, WINDOW_COMPOSITE_PADDING, &previous_drag_composite)) {
                    dirty_rect_list_add(&partial_present_rects, &previous_drag_composite);
                }
            }
            if (move_dragged_window_to_mouse(&state, &mouse)) {
                partial_present_valid = true;
            }
            const window_rect_t* drag_rect = get_window_rect_const(&state, state.dragging_window);
            if (drag_rect != NULL) {
                window_rect_t drag_composite = {0u, 0u, 0u, 0u};
                if (expand_rect_with_padding(drag_rect, WINDOW_COMPOSITE_PADDING, &drag_composite)) {
                    dirty_rect_list_add(&partial_present_rects, &drag_composite);
                }
                partial_present_valid = true;
            }
            state.dragging_window = WINDOW_NONE;
            scene_dirty = true;
        }

        if (input_changed && !mouse_moved && !mouse_pressed && !mouse_released && !right_pressed && !right_released) {
            scene_dirty = true;
            if (setup_active(&state)) {
                full_present_required = true;
            } else {
                window_rect_t active_window_rect = {0u, 0u, 0u, 0u};
                if (get_top_visible_window_composite_rect(&state, &active_window_rect)) {
                    dirty_rect_list_add(&partial_present_rects, &active_window_rect);
                    partial_present_valid = true;
                }
            }
        }

        if (state.power_action == POWER_ACTION_NONE && scene_dirty && !full_present_required) {
            partial_present_valid = compute_dirty_rects_from_snapshot(&state,
                                                                      window_visible_before,
                                                                      window_rects_before,
                                                                      window_order_before,
                                                                      dock_mask_before,
                                                                      &partial_present_rects) || partial_present_valid;

            if (!setup_active(&state) && mouse_moved) {
                window_rect_t previous_cursor_rect = {0u, 0u, 0u, 0u};
                window_rect_t current_cursor_rect = {0u, 0u, 0u, 0u};
                bool added_cursor_rect = false;

                if (get_cursor_draw_rect(previous_cursor_x, previous_cursor_y, &previous_cursor_rect)) {
                    dirty_rect_list_add(&partial_present_rects, &previous_cursor_rect);
                    added_cursor_rect = true;
                }
                if (get_cursor_draw_rect(mouse.x, mouse.y, &current_cursor_rect)) {
                    dirty_rect_list_add(&partial_present_rects, &current_cursor_rect);
                    added_cursor_rect = true;
                }
                if (added_cursor_rect) {
                    partial_present_valid = true;
                }
            }

            if (g_desktop_shell_theme != state.desktop_theme ||
                g_desktop_shell_wallpaper_theme != state.wallpaper_theme) {
                full_present_required = true;
            } else {
                if (g_desktop_shell_clock_second != current_clock_second) {
                    window_rect_t top_bar_rect = {0u, 0u, display_driver_width(), TOP_BAR_HEIGHT};
                    dirty_rect_list_add(&partial_present_rects, &top_bar_rect);
                    partial_present_valid = true;
                }
                if (g_desktop_shell_dock_mask != desktop_shell_dock_mask(&state)) {
                    dock_layout_t layout;
                    window_rect_t dock_rect;

                    get_dock_layout(&state, &layout);
                    dock_rect.x = layout.dock_x;
                    dock_rect.y = layout.dock_y;
                    dock_rect.width = layout.dock_width;
                    dock_rect.height = layout.dock_height;
                    dirty_rect_list_add(&partial_present_rects, &dock_rect);
                    partial_present_valid = true;
                }
                if (!partial_present_valid) {
                    window_rect_t active_window_rect = {0u, 0u, 0u, 0u};
                    if (get_top_visible_window_composite_rect(&state, &active_window_rect)) {
                        dirty_rect_list_add(&partial_present_rects, &active_window_rect);
                        partial_present_valid = true;
                    }
                }
            }
        }

        if (scene_dirty) {
            if (!setup_active(&state) && partial_present_valid && !full_present_required) {
                render_desktop_partial(wallpaper,
                                       boot_info,
                                       wallpaper_loaded,
                                       ata_ready,
                                       &state,
                                       &mouse,
                                       &partial_present_rects,
                                       current_clock_second);
            } else {
                render_desktop(wallpaper, boot_info, wallpaper_loaded, ata_ready, &state, &mouse, current_clock_second);
            }
            if (state.videos_playing) {
                state.videos_last_frame_index = videos_current_frame_index(&state);
            }
            if (state.startup_audio_armed && !state.startup_audio_played) {
                (void)startup_sound_start(&state);
            }
            scene_dirty = false;
            last_clock_second = current_clock_second;
            previous_cursor_x = mouse.x;
            previous_cursor_y = mouse.y;
            continue;
        }

        if (state.power_action == POWER_ACTION_NONE && mouse_moved) {
            refresh_cursor_only(previous_cursor_x, previous_cursor_y, &mouse);
            previous_cursor_x = mouse.x;
            previous_cursor_y = mouse.y;
            if (!setup_active(&state) && clock_changed) {
                last_clock_second = current_clock_second;
            } else if (clock_changed) {
                last_clock_second = current_clock_second;
            }
            continue;
        }

        if (state.power_action == POWER_ACTION_NONE && !setup_active(&state) && clock_changed) {
            last_clock_second = current_clock_second;
            previous_cursor_x = mouse.x;
            previous_cursor_y = mouse.y;
        }

    }
}

