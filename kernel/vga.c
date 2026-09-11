#include "jabulos.h"
#include "ui_font.h"

static framebuffer_info_t g_framebuffer;
static volatile u8* g_frontbuffer;
static u8* g_drawbuffer;
static u8* g_presentbuffer;
static u32 g_framebuffer_bytes;
static bool g_drawbuffer_uses_fallback = false;
static bool g_presentbuffer_uses_fallback = false;
static bool g_vsync_supported = true;
static bool g_clip_enabled = false;
static u32 g_clip_x = 0;
static u32 g_clip_y = 0;
static u32 g_clip_width = 0;
static u32 g_clip_height = 0;

// double buffering
#define VGA_SOFTWARE_BUFFER_MAX_BYTES (40u * 1024u * 1024u)
static u8 g_drawbuffer_fallback[VGA_SOFTWARE_BUFFER_MAX_BYTES];
static u8 g_presentbuffer_fallback[VGA_SOFTWARE_BUFFER_MAX_BYTES];

enum {
    FONT_SUBPIXEL_SAMPLES = 4,
    FONT_SHADOW_OFFSET_X = 0,
    FONT_SHADOW_OFFSET_Y = 1,
    FONT_SHADOW_ALPHA = 28
};

static u32 scale_channel(u8 value, u8 size) {
    if (size == 0) {
        return 0;
    }
    if (size >= 8) {
        return ((u32)value) << (size - 8);
    }
    return (u32)((value * ((1u << size) - 1u)) / 255u);
}

static u8 expand_channel(u32 value, u8 position, u8 size) {
    if (size == 0) {
        return 0;
    }

    u32 mask = ((1u << size) - 1u);
    u32 channel = (value >> position) & mask;
    return (u8)((channel * 255u) / mask);
}

static void unpack_color(u32 color, u8* red, u8* green, u8* blue) {
    *red = expand_channel(color, g_framebuffer.red_position, g_framebuffer.red_mask_size);
    *green = expand_channel(color, g_framebuffer.green_position, g_framebuffer.green_mask_size);
    *blue = expand_channel(color, g_framebuffer.blue_position, g_framebuffer.blue_mask_size);
}

static bool wait_for_retrace_state(bool in_retrace) {
    const u32 retrace_mask = 0x08u;
    const u32 timeout = 250000u;

    for (u32 spin = 0; spin < timeout; ++spin) {
        if (((inb(0x3DA) & retrace_mask) != 0u) == in_retrace) {
            return true;
        }
        io_wait();
    }

    return false;
}

static void vga_wait_for_vsync_if_needed(u32 width, u32 height) {
    if (!g_vsync_supported || width == 0u || height == 0u) {
        return;
    }

    u64 present_area = (u64)width * (u64)height;
    u64 screen_area = (u64)g_framebuffer.width * (u64)g_framebuffer.height;
    
    // threshold for vsycb
    if (present_area * 2u < screen_area && height < 256u) {
        return;
    }

 
    if ((inb(0x3DA) & 0x08u) != 0u) {
        return;
    }

    // Wait for the start of the next vertical retrace
    if (!wait_for_retrace_state(true)) {
        g_vsync_supported = false;
    }
}

static void copy_buffer_region(volatile u8* destination,
                               u8* mirror_destination,
                               const u8* source,
                               u32 x,
                               u32 y,
                               u32 width,
                               u32 height) {
    u32 bytes_per_pixel;
    u32 copy_width;
    u32 copy_height;
    u32 row_bytes;

    if (destination == NULL || source == NULL) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height || width == 0u || height == 0u) {
        return;
    }

    copy_width = (x + width > g_framebuffer.width) ? (g_framebuffer.width - x) : width;
    copy_height = (y + height > g_framebuffer.height) ? (g_framebuffer.height - y) : height;
    bytes_per_pixel = g_framebuffer.bpp / 8;
    row_bytes = copy_width * bytes_per_pixel;

    if (copy_width == g_framebuffer.width && x == 0u) {
        u32 offset = y * g_framebuffer.pitch;
        u32 block_bytes = copy_height * g_framebuffer.pitch;
        memcpy((void*)(destination + offset), source + offset, block_bytes);
        if (mirror_destination != NULL) {
            memcpy(mirror_destination + offset, source + offset, block_bytes);
        }
        return;
    }

    for (u32 row = 0; row < copy_height; ++row) {
        u32 offset = (y + row) * g_framebuffer.pitch + x * bytes_per_pixel;
        memcpy((void*)(destination + offset), source + offset, row_bytes);
        if (mirror_destination != NULL) {
            memcpy(mirror_destination + offset, source + offset, row_bytes);
        }
    }
}

static u8* allocate_backbuffer(u32 size) {
    u32 frames = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    u64 first_frame = 0;

    if (size == 0) {
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

        if (index == 0) {
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

static u8* acquire_software_buffer(u32 size, u8* fallback, bool* out_used_fallback) {
    u8* buffer = allocate_backbuffer(size);

    if (out_used_fallback != NULL) {
        *out_used_fallback = false;
    }

    if (buffer != NULL) {
        return buffer;
    }

    if (fallback != NULL && size <= VGA_SOFTWARE_BUFFER_MAX_BYTES) {
        memset(fallback, 0, size);
        if (out_used_fallback != NULL) {
            *out_used_fallback = true;
        }
        return fallback;
    }

    return NULL;
}

static bool point_in_clip_rect(u32 x, u32 y) {
    u32 clip_end_x;
    u32 clip_end_y;

    if (!g_clip_enabled) {
        return true;
    }

    clip_end_x = g_clip_x + g_clip_width;
    clip_end_y = g_clip_y + g_clip_height;
    return x >= g_clip_x && x < clip_end_x && y >= g_clip_y && y < clip_end_y;
}

static void put_pixel_to_buffer(volatile u8* buffer, u32 x, u32 y, u32 color) {
    u32 bytes_per_pixel;
    u32 offset;

    if (buffer == NULL || x >= g_framebuffer.width || y >= g_framebuffer.height) {
        return;
    }

    bytes_per_pixel = g_framebuffer.bpp / 8;
    offset = y * g_framebuffer.pitch + x * bytes_per_pixel;

    if (g_framebuffer.bpp == 32) {
        *(u32*)(buffer + offset) = color;
    } else if (g_framebuffer.bpp == 24) {
        buffer[offset] = (u8)(color & 0xFF);
        buffer[offset + 1] = (u8)((color >> 8) & 0xFF);
        buffer[offset + 2] = (u8)((color >> 16) & 0xFF);
    }
}

static void blend_pixel(u32 x, u32 y, u32 color, u8 alpha) {
    if (alpha == 0 || x >= g_framebuffer.width || y >= g_framebuffer.height) {
        return;
    }

    if (alpha == 255) {
        put_pixel(x, y, color);
        return;
    }

    u32 bytes_per_pixel = g_framebuffer.bpp / 8;
    u32 offset = y * g_framebuffer.pitch + x * bytes_per_pixel;
    u8* pixel = g_drawbuffer + offset;

    if (g_framebuffer.bpp == 32) {
        u32 existing = *(u32*)pixel;
        
        u32 rb1 = existing & 0xFF00FFu;
        u32 g1 = existing & 0x00FF00u;
        u32 rb2 = color & 0xFF00FFu;
        u32 g2 = color & 0x00FF00u;

        u32 rb = rb1 + (((rb2 - rb1) * alpha) >> 8);
        u32 g = g1 + (((g2 - g1) * alpha) >> 8);

        *(u32*)pixel = (rb & 0xFF00FFu) | (g & 0x00FF00u);
    } else {
        // fallback for 24bpp
        u8 base_r, base_g, base_b;
        u8 src_r, src_g, src_b;
        unpack_color(*(u32*)pixel, &base_r, &base_g, &base_b);
        unpack_color(color, &src_r, &src_g, &src_b);

        u32 inv_alpha = 255u - alpha;
        u8 out_r = (u8)(((u32)src_r * alpha + (u32)base_r * inv_alpha) / 255u);
        u8 out_g = (u8)(((u32)src_g * alpha + (u32)base_g * inv_alpha) / 255u);
        u8 out_b = (u8)(((u32)src_b * alpha + (u32)base_b * inv_alpha) / 255u);
        
        pixel[0] = out_b;
        pixel[1] = out_g;
        pixel[2] = out_r;
    }
}

static bool glyph_is_empty(const u8 glyph[UI_FONT_BITMAP_HEIGHT][UI_FONT_BITMAP_WIDTH]) {
    for (u32 row = 0; row < UI_FONT_BITMAP_HEIGHT; ++row) {
        for (u32 col = 0; col < UI_FONT_BITMAP_WIDTH; ++col) {
            if (glyph[row][col] != 0u) {
                return false;
            }
        }
    }
    return true;
}

static const u8 (*resolve_glyph(char character))[UI_FONT_BITMAP_WIDTH] {
    const u8 (*glyph)[UI_FONT_BITMAP_WIDTH] = ui_font_alpha[(u8)character];

    if (glyph_is_empty(glyph) && character != ' ') {
        return ui_font_alpha[(u8)'?'];
    }

    return glyph;
}

static u8 glyph_pixel_coverage(const u8 glyph[UI_FONT_BITMAP_HEIGHT][UI_FONT_BITMAP_WIDTH], s32 x, s32 y) {
    if (x < 0 || y < 0 || x >= (s32)UI_FONT_BITMAP_WIDTH || y >= (s32)UI_FONT_BITMAP_HEIGHT) {
        return 0;
    }

    return glyph[(u32)y][(u32)x];
}

static void draw_glyph_aa(u32 x, u32 y, const u8 glyph[UI_FONT_BITMAP_HEIGHT][UI_FONT_BITMAP_WIDTH], u32 color) {
    const u32 shadow_color = vga_color(18, 22, 32);

    for (u32 row = 0; row < UI_FONT_RENDER_HEIGHT; ++row) {
        for (u32 col = 0; col < UI_FONT_RENDER_WIDTH; ++col) {
            u8 coverage = glyph_pixel_coverage(glyph, (s32)col, (s32)row);
            if (coverage > 0u) {
                blend_pixel(x + col + FONT_SHADOW_OFFSET_X,
                            y + row + FONT_SHADOW_OFFSET_Y,
                            shadow_color,
                            (u8)(((u32)coverage * FONT_SHADOW_ALPHA) / 255u));
                blend_pixel(x + col, y + row, color, coverage);
            }
        }
    }
}

void vga_initialize(const framebuffer_info_t* framebuffer) {
    g_framebuffer = *framebuffer;
    g_frontbuffer = (volatile u8*)(u64)framebuffer->address;
    g_framebuffer_bytes = framebuffer->pitch * framebuffer->height;
    g_drawbuffer = acquire_software_buffer(g_framebuffer_bytes, g_drawbuffer_fallback, &g_drawbuffer_uses_fallback);
    g_presentbuffer = acquire_software_buffer(g_framebuffer_bytes, g_presentbuffer_fallback, &g_presentbuffer_uses_fallback);
    if (g_drawbuffer == NULL) {
        g_drawbuffer = (u8*)(u64)g_frontbuffer;
    }
    if (g_drawbuffer != (u8*)(u64)g_frontbuffer) {
        memcpy((void*)g_frontbuffer, g_drawbuffer, g_framebuffer_bytes);
    }
    if (g_presentbuffer != NULL) {
        memcpy(g_presentbuffer, (const void*)g_frontbuffer, g_framebuffer_bytes);
    }
}

u32 vga_color(u8 red, u8 green, u8 blue) {
    return (scale_channel(red, g_framebuffer.red_mask_size) << g_framebuffer.red_position) |
           (scale_channel(green, g_framebuffer.green_mask_size) << g_framebuffer.green_position) |
           (scale_channel(blue, g_framebuffer.blue_mask_size) << g_framebuffer.blue_position);
}

void put_pixel(u32 x, u32 y, u32 color) {
    if (!point_in_clip_rect(x, y)) {
        return;
    }
    put_pixel_to_buffer((volatile u8*)g_drawbuffer, x, y, color);
}

void draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color) {
    if (width == 0 || height == 0) return;
    
    // Check if we can use a fast row-based fill
    if (!g_clip_enabled || (x >= g_clip_x && y >= g_clip_y && 
        x + width <= g_clip_x + g_clip_width && y + height <= g_clip_y + g_clip_height)) {
        
        u32 bytes_per_pixel = g_framebuffer.bpp / 8;
        u8* dest = g_drawbuffer + y * g_framebuffer.pitch + x * bytes_per_pixel;

        if (g_framebuffer.bpp == 32) {
            for (u32 py = 0; py < height; ++py) {
                u32* row = (u32*)dest;
                for (u32 px = 0; px < width; ++px) {
                    row[px] = color;
                }
                dest += g_framebuffer.pitch;
            }
            return;
        }
    }

    // Fallback to slow put_pixel if clipped or other formats
    for (u32 py = 0; py < height; ++py) {
        for (u32 px = 0; px < width; ++px) {
            put_pixel(x + px, y + py, color);
        }
    }
}

void draw_rect_outline(u32 x, u32 y, u32 width, u32 height, u32 thickness, u32 color) {
    draw_rect(x, y, width, thickness, color);
    draw_rect(x, y + height - thickness, width, thickness, color);
    draw_rect(x, y, thickness, height, color);
    draw_rect(x + width - thickness, y, thickness, height, color);
}

static bool rounded_rect_contains(u32 px, u32 py, u32 width, u32 height, u32 radius) {
    u32 effective_radius = radius;

    if (effective_radius == 0u) {
        return true;
    }

    if (effective_radius * 2u > width) {
        effective_radius = width / 2u;
    }
    if (effective_radius * 2u > height) {
        effective_radius = height / 2u;
    }

    if (px < effective_radius && py < effective_radius) {
        s32 dx = (s32)effective_radius - 1 - (s32)px;
        s32 dy = (s32)effective_radius - 1 - (s32)py;
        return (u32)(dx * dx + dy * dy) <= effective_radius * effective_radius;
    } else if (px >= width - effective_radius && py < effective_radius) {
        s32 dx = (s32)px - (s32)(width - effective_radius);
        s32 dy = (s32)effective_radius - 1 - (s32)py;
        return (u32)(dx * dx + dy * dy) <= effective_radius * effective_radius;
    } else if (px < effective_radius && py >= height - effective_radius) {
        s32 dx = (s32)effective_radius - 1 - (s32)px;
        s32 dy = (s32)py - (s32)(height - effective_radius);
        return (u32)(dx * dx + dy * dy) <= effective_radius * effective_radius;
    } else if (px >= width - effective_radius && py >= height - effective_radius) {
        s32 dx = (s32)px - (s32)(width - effective_radius);
        s32 dy = (s32)py - (s32)(height - effective_radius);
        return (u32)(dx * dx + dy * dy) <= effective_radius * effective_radius;
    }

    return true;
}

void draw_rounded_rect(u32 x, u32 y, u32 width, u32 height, u32 radius, u32 color) {
    if (radius == 0) {
        draw_rect(x, y, width, height, color);
        return;
    }

    if (radius * 2 > width) {
        radius = width / 2;
    }
    if (radius * 2 > height) {
        radius = height / 2;
    }

    u32 radius_sq = radius * radius;
    for (u32 py = 0; py < height; ++py) {
        for (u32 px = 0; px < width; ++px) {
            bool inside = true;

            if (px < radius && py < radius) {
                s32 dx = (s32)radius - 1 - (s32)px;
                s32 dy = (s32)radius - 1 - (s32)py;
                inside = (u32)(dx * dx + dy * dy) <= radius_sq;
            } else if (px >= width - radius && py < radius) {
                s32 dx = (s32)px - (s32)(width - radius);
                s32 dy = (s32)radius - 1 - (s32)py;
                inside = (u32)(dx * dx + dy * dy) <= radius_sq;
            } else if (px < radius && py >= height - radius) {
                s32 dx = (s32)radius - 1 - (s32)px;
                s32 dy = (s32)py - (s32)(height - radius);
                inside = (u32)(dx * dx + dy * dy) <= radius_sq;
            } else if (px >= width - radius && py >= height - radius) {
                s32 dx = (s32)px - (s32)(width - radius);
                s32 dy = (s32)py - (s32)(height - radius);
                inside = (u32)(dx * dx + dy * dy) <= radius_sq;
            }

            if (inside) {
                put_pixel(x + px, y + py, color);
            }
        }
    }
}

void draw_rounded_rect_outline(u32 x, u32 y, u32 width, u32 height, u32 radius, u32 thickness, u32 color) {
    if (width == 0u || height == 0u || thickness == 0u) {
        return;
    }

    if (thickness * 2u >= width || thickness * 2u >= height) {
        draw_rounded_rect(x, y, width, height, radius, color);
        return;
    }

    u32 inner_width = width - thickness * 2u;
    u32 inner_height = height - thickness * 2u;
    u32 inner_radius = radius > thickness ? radius - thickness : 0u;

    for (u32 py = 0; py < height; ++py) {
        for (u32 px = 0; px < width; ++px) {
            bool in_outer = rounded_rect_contains(px, py, width, height, radius);
            bool in_inner = false;

            if (in_outer &&
                px >= thickness && py >= thickness &&
                px < width - thickness && py < height - thickness) {
                in_inner = rounded_rect_contains(px - thickness, py - thickness, inner_width, inner_height, inner_radius);
            }

            if (in_outer && !in_inner) {
                put_pixel(x + px, y + py, color);
            }
        }
    }
}

void vga_fill_rect_alpha(u32 x, u32 y, u32 width, u32 height, u8 red, u8 green, u8 blue, u8 alpha) {
    for (u32 py = 0; py < height; ++py) {
        for (u32 px = 0; px < width; ++px) {
            blend_pixel(x + px, y + py, vga_color(red, green, blue), alpha);
        }
    }
}

void vga_fill_rounded_rect_alpha(u32 x, u32 y, u32 width, u32 height, u32 radius, u8 red, u8 green, u8 blue, u8 alpha) {
    if (width == 0u || height == 0u || alpha == 0u) {
        return;
    }

    for (u32 py = 0; py < height; ++py) {
        for (u32 px = 0; px < width; ++px) {
            if (rounded_rect_contains(px, py, width, height, radius)) {
                blend_pixel(x + px, y + py, vga_color(red, green, blue), alpha);
            }
        }
    }
}

void draw_char(u32 x, u32 y, char character, u32 color) {
    draw_glyph_aa(x, y, resolve_glyph(character), color);
}

void draw_text(u32 x, u32 y, const char* text, u32 color) {
    while (*text != '\0') {
        draw_char(x, y, *text, color);
        x += UI_FONT_ADVANCE_X;
        ++text;
    }
}

u32 vga_text_advance_x(void) {
    return UI_FONT_ADVANCE_X;
}

u32 vga_text_height(void) {
    return UI_FONT_RENDER_HEIGHT;
}

void vga_clear(u32 color) {
    draw_rect(0, 0, g_framebuffer.width, g_framebuffer.height, color);
}

void vga_present(void) {
    if (g_frontbuffer == NULL || g_drawbuffer == NULL) {
        return;
    }
    if (g_drawbuffer == (u8*)(u64)g_frontbuffer) {
        return;
    }

    vga_wait_for_vsync_if_needed(g_framebuffer.width, g_framebuffer.height);
    copy_buffer_region(g_frontbuffer,
                       g_presentbuffer,
                       g_drawbuffer,
                       0u,
                       0u,
                       g_framebuffer.width,
                       g_framebuffer.height);
}

void vga_present_region(u32 x, u32 y, u32 width, u32 height) {
    if (g_frontbuffer == NULL || g_drawbuffer == NULL) {
        return;
    }
    if (g_drawbuffer == (u8*)(u64)g_frontbuffer) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height || width == 0 || height == 0) {
        return;
    }

    // Region presents are used heavily by the GUI compositor. Waiting for vblank when the region
    // is large enough reduces visible tearing/flicker during interactive actions (e.g. dragging).
    vga_wait_for_vsync_if_needed(width, height);

    copy_buffer_region(g_frontbuffer,
                       g_presentbuffer,
                       g_drawbuffer,
                       x,
                       y,
                       width,
                       height);
}

void vga_put_pixel_front(u32 x, u32 y, u32 color) {
    put_pixel_to_buffer(g_frontbuffer, x, y, color);
    if (g_presentbuffer != NULL) {
        put_pixel_to_buffer(g_presentbuffer, x, y, color);
    }
}

void vga_restore_frontbuffer_from_drawbuffer_region(u32 x, u32 y, u32 width, u32 height) {
    if (g_frontbuffer == NULL || g_drawbuffer == NULL || g_drawbuffer == (u8*)(u64)g_frontbuffer) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height || width == 0u || height == 0u) {
        return;
    }

    vga_wait_for_vsync_if_needed(width, height);

    copy_buffer_region(g_frontbuffer,
                       g_presentbuffer,
                       g_drawbuffer,
                       x,
                       y,
                       width,
                       height);
}

void vga_set_clip_rect(u32 x, u32 y, u32 width, u32 height) {
    u32 end_x;
    u32 end_y;

    if (width == 0u || height == 0u || x >= g_framebuffer.width || y >= g_framebuffer.height) {
        g_clip_enabled = true;
        g_clip_x = 0u;
        g_clip_y = 0u;
        g_clip_width = 0u;
        g_clip_height = 0u;
        return;
    }

    end_x = x + width;
    end_y = y + height;
    if (end_x > g_framebuffer.width || end_x < x) {
        end_x = g_framebuffer.width;
    }
    if (end_y > g_framebuffer.height || end_y < y) {
        end_y = g_framebuffer.height;
    }

    g_clip_enabled = true;
    g_clip_x = x;
    g_clip_y = y;
    g_clip_width = end_x - x;
    g_clip_height = end_y - y;
}

void vga_clear_clip_rect(void) {
    g_clip_enabled = false;
    g_clip_x = 0u;
    g_clip_y = 0u;
    g_clip_width = 0u;
    g_clip_height = 0u;
}

u32 vga_width(void) {
    return g_framebuffer.width;
}

u32 vga_height(void) {
    return g_framebuffer.height;
}

u32 vga_framebuffer_bytes(void) {
    return g_framebuffer_bytes;
}

void vga_copy_drawbuffer(void* destination, u32 size) {
    if (destination == NULL || g_drawbuffer == NULL) {
        return;
    }

    if (size > g_framebuffer_bytes) {
        size = g_framebuffer_bytes;
    }

    memcpy(destination, g_drawbuffer, size);
}

void vga_copy_drawbuffer_region(void* destination, u32 size, u32 x, u32 y, u32 width, u32 height) {
    u32 bytes_per_pixel;
    u32 copy_width;
    u32 copy_height;
    u32 required_size;

    if (destination == NULL || g_drawbuffer == NULL) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height || width == 0 || height == 0) {
        return;
    }

    copy_width = (x + width > g_framebuffer.width) ? (g_framebuffer.width - x) : width;
    copy_height = (y + height > g_framebuffer.height) ? (g_framebuffer.height - y) : height;
    bytes_per_pixel = g_framebuffer.bpp / 8;
    required_size = copy_width * copy_height * bytes_per_pixel;
    if (size < required_size) {
        return;
    }

    for (u32 row = 0; row < copy_height; ++row) {
        u32 src_offset = (y + row) * g_framebuffer.pitch + x * bytes_per_pixel;
        u32 dst_offset = row * copy_width * bytes_per_pixel;
        memcpy((u8*)destination + dst_offset, g_drawbuffer + src_offset, copy_width * bytes_per_pixel);
    }
}

void vga_restore_drawbuffer(const void* source, u32 size) {
    if (source == NULL || g_drawbuffer == NULL) {
        return;
    }

    if (size > g_framebuffer_bytes) {
        size = g_framebuffer_bytes;
    }

    memcpy(g_drawbuffer, source, size);
}

void vga_restore_drawbuffer_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height) {
    u32 bytes_per_pixel;
    u32 copy_width;
    u32 copy_height;
    u32 required_size;

    if (source == NULL || g_drawbuffer == NULL) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height || width == 0 || height == 0) {
        return;
    }

    copy_width = (x + width > g_framebuffer.width) ? (g_framebuffer.width - x) : width;
    copy_height = (y + height > g_framebuffer.height) ? (g_framebuffer.height - y) : height;
    bytes_per_pixel = g_framebuffer.bpp / 8;
    required_size = copy_width * copy_height * bytes_per_pixel;
    if (size < required_size) {
        return;
    }

    for (u32 row = 0; row < copy_height; ++row) {
        u32 dst_offset = (y + row) * g_framebuffer.pitch + x * bytes_per_pixel;
        u32 src_offset = row * copy_width * bytes_per_pixel;
        memcpy(g_drawbuffer + dst_offset, (const u8*)source + src_offset, copy_width * bytes_per_pixel);
    }
}

void vga_restore_drawbuffer_surface_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height) {
    u32 bytes_per_pixel;
    u32 copy_width;
    u32 copy_height;

    if (source == NULL || g_drawbuffer == NULL) {
        return;
    }

    if (size < g_framebuffer_bytes) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height || width == 0 || height == 0) {
        return;
    }

    copy_width = (x + width > g_framebuffer.width) ? (g_framebuffer.width - x) : width;
    copy_height = (y + height > g_framebuffer.height) ? (g_framebuffer.height - y) : height;
    bytes_per_pixel = g_framebuffer.bpp / 8;

    for (u32 row = 0; row < copy_height; ++row) {
        u32 offset = (y + row) * g_framebuffer.pitch + x * bytes_per_pixel;
        memcpy(g_drawbuffer + offset, (const u8*)source + offset, copy_width * bytes_per_pixel);
    }
}

void vga_blit_rgbx32_fit(const u32* pixels, u32 source_width, u32 source_height, u32 clear_color) {
    u32 screen_width;
    u32 screen_height;
    u32 draw_width;
    u32 draw_height;
    u32 draw_x;
    u32 draw_y;

    if (pixels == NULL || source_width == 0u || source_height == 0u) {
        return;
    }

    screen_width = vga_width();
    screen_height = vga_height();
    if (screen_width == 0u || screen_height == 0u) {
        return;
    }

    draw_width = screen_width;
    draw_height = (screen_width * source_height) / source_width;
    if (draw_height > screen_height) {
        draw_height = screen_height;
        draw_width = (screen_height * source_width) / source_height;
    }

    if (draw_width == 0u) {
        draw_width = 1u;
    }
    if (draw_height == 0u) {
        draw_height = 1u;
    }

    draw_x = (screen_width - draw_width) / 2u;
    draw_y = (screen_height - draw_height) / 2u;

    vga_clear(clear_color);

    for (u32 dst_y = 0; dst_y < draw_height; ++dst_y) {
        u32 source_y = (dst_y * source_height) / draw_height;

        for (u32 dst_x = 0; dst_x < draw_width; ++dst_x) {
            u32 source_x = (dst_x * source_width) / draw_width;
            u32 pixel = pixels[source_y * source_width + source_x];
            u8 red = (u8)(pixel & 0xFFu);
            u8 green = (u8)((pixel >> 8) & 0xFFu);
            u8 blue = (u8)((pixel >> 16) & 0xFFu);
            put_pixel(draw_x + dst_x, draw_y + dst_y, vga_color(red, green, blue));
        }
    }
}
