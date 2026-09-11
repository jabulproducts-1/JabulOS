#include "jabulos.h"

#define GAME_KEY_QUEUE_CAPACITY 64u
#define GAME_FILE_HANDLE_COUNT 8u

typedef struct {
    bool used;
    const u8* data;
    u32 size;
    u32 position;
} game_file_handle_t;

typedef struct {
    const void* initrd_archive;
    u32 initrd_size;
    bool active;
    u32 width;
    u32 height;
    u32 pitch_bytes;
    u32* framebuffer;
    u32 framebuffer_bytes;
    u64 started_ms;
    u32 frame_count;
    const char* title;
    game_key_event_t key_events[GAME_KEY_QUEUE_CAPACITY];
    u32 key_read_index;
    u32 key_write_index;
    game_file_handle_t file_handles[GAME_FILE_HANDLE_COUNT];
} game_runtime_state_t;

static game_runtime_state_t g_game_runtime = {};

static void* allocate_contiguous_pages(u32 size) {
    u32 frames = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    u64 first_frame = 0;

    if (size == 0u) {
        return NULL;
    }

    for (u32 index = 0; index < frames; ++index) {
        u64 frame = pmm_alloc_frame();

        if (frame == 0u) {
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
    return (void*)(u64)first_frame;
}

static const char* normalize_runtime_path(const char* path) {
    if (path == NULL) {
        return NULL;
    }

    while (*path == '/' || *path == '\\') {
        ++path;
    }

    return *path == '\0' ? NULL : path;
}

void game_runtime_initialize(const boot_info_t* boot_info) {
    memset(&g_game_runtime, 0, sizeof(g_game_runtime));
    if (boot_info == NULL) {
        return;
    }

    g_game_runtime.initrd_archive = (const void*)boot_info->initrd_start;
    g_game_runtime.initrd_size = (u32)boot_info->initrd_size;
}

bool game_runtime_start_fullscreen(u32 width, u32 height, const char* title) {
    u32 required_bytes;

    if (width == 0u || height == 0u) {
        return false;
    }

    required_bytes = width * height * sizeof(u32);
    if (g_game_runtime.framebuffer == NULL || g_game_runtime.framebuffer_bytes < required_bytes) {
        void* new_buffer;

        if (g_game_runtime.framebuffer != NULL) {
            game_runtime_free(g_game_runtime.framebuffer, g_game_runtime.framebuffer_bytes);
            g_game_runtime.framebuffer = NULL;
            g_game_runtime.framebuffer_bytes = 0u;
        }

        new_buffer = allocate_contiguous_pages(required_bytes);
        if (new_buffer == NULL) {
            return false;
        }

        g_game_runtime.framebuffer = (u32*)new_buffer;
        g_game_runtime.framebuffer_bytes = required_bytes;
    }

    memset(g_game_runtime.framebuffer, 0, required_bytes);
    g_game_runtime.width = width;
    g_game_runtime.height = height;
    g_game_runtime.pitch_bytes = width * sizeof(u32);
    g_game_runtime.started_ms = timer_ticks_ms();
    g_game_runtime.frame_count = 0u;
    g_game_runtime.title = title != NULL ? title : "Game Runtime";
    g_game_runtime.active = true;
    game_runtime_clear_key_events();
    return true;
}

void game_runtime_stop(void) {
    g_game_runtime.active = false;
    game_runtime_clear_key_events();
}

bool game_runtime_is_active(void) {
    return g_game_runtime.active;
}

u32* game_runtime_framebuffer(void) {
    return g_game_runtime.framebuffer;
}

u32 game_runtime_framebuffer_width(void) {
    return g_game_runtime.width;
}

u32 game_runtime_framebuffer_height(void) {
    return g_game_runtime.height;
}

u32 game_runtime_framebuffer_pitch_bytes(void) {
    return g_game_runtime.pitch_bytes;
}

void game_runtime_present(void) {
    if (!g_game_runtime.active || g_game_runtime.framebuffer == NULL) {
        return;
    }

    display_driver_blit_rgbx32_fit(g_game_runtime.framebuffer,
                                   g_game_runtime.width,
                                   g_game_runtime.height,
                                   vga_color(0, 0, 0));
    ++g_game_runtime.frame_count;
}

void game_runtime_get_info(game_runtime_info_t* out_info) {
    u64 elapsed_ms;

    if (out_info == NULL) {
        return;
    }

    memset(out_info, 0, sizeof(*out_info));
    out_info->active = g_game_runtime.active;
    out_info->width = g_game_runtime.width;
    out_info->height = g_game_runtime.height;
    out_info->pitch_bytes = g_game_runtime.pitch_bytes;
    out_info->frame_count = g_game_runtime.frame_count;
    out_info->started_ms = g_game_runtime.started_ms;
    out_info->title = g_game_runtime.title;

    if (g_game_runtime.started_ms == 0u || g_game_runtime.frame_count == 0u) {
        return;
    }

    elapsed_ms = timer_ticks_ms() - g_game_runtime.started_ms;
    if (elapsed_ms != 0u) {
        out_info->average_fps = (u32)(((u64)g_game_runtime.frame_count * 1000ull) / elapsed_ms);
    }
}

void game_runtime_clear_key_events(void) {
    g_game_runtime.key_read_index = 0u;
    g_game_runtime.key_write_index = 0u;
    memset(g_game_runtime.key_events, 0, sizeof(g_game_runtime.key_events));
}

void game_runtime_push_key_event(u16 scancode, bool pressed, bool extended, char ascii) {
    u32 next_index;

    next_index = (g_game_runtime.key_write_index + 1u) % GAME_KEY_QUEUE_CAPACITY;
    if (next_index == g_game_runtime.key_read_index) {
        g_game_runtime.key_read_index = (g_game_runtime.key_read_index + 1u) % GAME_KEY_QUEUE_CAPACITY;
    }

    g_game_runtime.key_events[g_game_runtime.key_write_index].scancode = scancode;
    g_game_runtime.key_events[g_game_runtime.key_write_index].ascii = ascii;
    g_game_runtime.key_events[g_game_runtime.key_write_index].pressed = pressed;
    g_game_runtime.key_events[g_game_runtime.key_write_index].extended = extended;
    g_game_runtime.key_write_index = next_index;
}

bool game_runtime_poll_key_event(game_key_event_t* out_event) {
    if (out_event == NULL || g_game_runtime.key_read_index == g_game_runtime.key_write_index) {
        return false;
    }

    *out_event = g_game_runtime.key_events[g_game_runtime.key_read_index];
    g_game_runtime.key_read_index = (g_game_runtime.key_read_index + 1u) % GAME_KEY_QUEUE_CAPACITY;
    return true;
}

void* game_runtime_alloc(u32 size) {
    return allocate_contiguous_pages(size);
}

void game_runtime_free(void* base, u32 size) {
    u32 frames;

    if (base == NULL || size == 0u) {
        return;
    }

    frames = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    for (u32 index = 0; index < frames; ++index) {
        pmm_free_frame((u64)(uintptr_t)base + (u64)index * PAGE_SIZE);
    }
}

bool game_runtime_file_exists(const char* path) {
    initrd_file_t file;
    const char* normalized_path = normalize_runtime_path(path);

    if (normalized_path == NULL ||
        g_game_runtime.initrd_archive == NULL ||
        g_game_runtime.initrd_size == 0u) {
        return false;
    }

    return initrd_find_file(g_game_runtime.initrd_archive, g_game_runtime.initrd_size, normalized_path, &file);
}

int game_runtime_file_open(const char* path) {
    initrd_file_t file;
    const char* normalized_path = normalize_runtime_path(path);

    if (normalized_path == NULL ||
        g_game_runtime.initrd_archive == NULL ||
        g_game_runtime.initrd_size == 0u ||
        !initrd_find_file(g_game_runtime.initrd_archive, g_game_runtime.initrd_size, normalized_path, &file)) {
        return -1;
    }

    for (u32 index = 0; index < GAME_FILE_HANDLE_COUNT; ++index) {
        if (!g_game_runtime.file_handles[index].used) {
            g_game_runtime.file_handles[index].used = true;
            g_game_runtime.file_handles[index].data = (const u8*)file.data;
            g_game_runtime.file_handles[index].size = file.size;
            g_game_runtime.file_handles[index].position = 0u;
            return (int)index;
        }
    }

    return -1;
}

u32 game_runtime_file_size(int handle) {
    if (handle < 0 || (u32)handle >= GAME_FILE_HANDLE_COUNT || !g_game_runtime.file_handles[handle].used) {
        return 0u;
    }

    return g_game_runtime.file_handles[handle].size;
}

u32 game_runtime_file_tell(int handle) {
    if (handle < 0 || (u32)handle >= GAME_FILE_HANDLE_COUNT || !g_game_runtime.file_handles[handle].used) {
        return 0u;
    }

    return g_game_runtime.file_handles[handle].position;
}

bool game_runtime_file_seek(int handle, s32 offset, game_file_seek_origin_t origin) {
    s64 next_position;
    game_file_handle_t* file;

    if (handle < 0 || (u32)handle >= GAME_FILE_HANDLE_COUNT || !g_game_runtime.file_handles[handle].used) {
        return false;
    }

    file = &g_game_runtime.file_handles[handle];
    switch (origin) {
        case GAME_FILE_SEEK_SET:
            next_position = offset;
            break;
        case GAME_FILE_SEEK_CURRENT:
            next_position = (s64)file->position + offset;
            break;
        case GAME_FILE_SEEK_END:
            next_position = (s64)file->size + offset;
            break;
        default:
            return false;
    }

    if (next_position < 0 || (u64)next_position > file->size) {
        return false;
    }

    file->position = (u32)next_position;
    return true;
}

u32 game_runtime_file_read(int handle, void* destination, u32 size) {
    u32 remaining;
    game_file_handle_t* file;

    if (destination == NULL ||
        handle < 0 ||
        (u32)handle >= GAME_FILE_HANDLE_COUNT ||
        !g_game_runtime.file_handles[handle].used) {
        return 0u;
    }

    file = &g_game_runtime.file_handles[handle];
    if (file->position >= file->size) {
        return 0u;
    }

    remaining = file->size - file->position;
    if (size > remaining) {
        size = remaining;
    }

    memcpy(destination, file->data + file->position, size);
    file->position += size;
    return size;
}

void game_runtime_file_close(int handle) {
    if (handle < 0 || (u32)handle >= GAME_FILE_HANDLE_COUNT) {
        return;
    }

    memset(&g_game_runtime.file_handles[handle], 0, sizeof(g_game_runtime.file_handles[handle]));
}
