#include "jabulos.h"

static u16 read_le16(const u8* data) {
    return (u16)data[0] | ((u16)data[1] << 8);
}

static u32 read_le32(const u8* data) {
    return (u32)data[0] |
           ((u32)data[1] << 8) |
           ((u32)data[2] << 16) |
           ((u32)data[3] << 24);
}

static bool load_bmp(const u8* data, u32 size, image_t* out_image) {
    if (size < 54 || data[0] != 'B' || data[1] != 'M') {
        return false;
    }

    u32 pixel_offset = read_le32(data + 10);
    u32 dib_size = read_le32(data + 14);
    s32 width = (s32)read_le32(data + 18);
    s32 height = (s32)read_le32(data + 22);
    u16 planes = read_le16(data + 26);
    u16 bpp = read_le16(data + 28);
    u32 compression = read_le32(data + 30);

    if (dib_size < 40 || planes != 1 || compression != 0 || (bpp != 24 && bpp != 32) || width <= 0 || height == 0) {
        return false;
    }

    u32 real_height = (height < 0) ? (u32)(-height) : (u32)height;
    u32 pitch = ALIGN_UP((u32)width * (bpp / 8), 4);
    if (pixel_offset + pitch * real_height > size) {
        return false;
    }

    out_image->pixels = data + pixel_offset;
    out_image->width = (u32)width;
    out_image->height = real_height;
    out_image->pitch = pitch;
    out_image->bpp = (u8)bpp;
    out_image->bottom_up = (height > 0);
    out_image->bgr = true;
    return true;
}

static bool load_tga(const u8* data, u32 size, image_t* out_image) {
    if (size < 18) {
        return false;
    }

    u8 id_length = data[0];
    u8 color_map_type = data[1];
    u8 image_type = data[2];
    u16 width = read_le16(data + 12);
    u16 height = read_le16(data + 14);
    u8 bpp = data[16];
    u8 descriptor = data[17];
    u32 pixel_offset = 18 + id_length;

    if (color_map_type != 0 || image_type != 2 || (bpp != 24 && bpp != 32) || width == 0 || height == 0) {
        return false;
    }

    u32 pitch = (u32)width * (bpp / 8);
    if (pixel_offset + pitch * height > size) {
        return false;
    }

    out_image->pixels = data + pixel_offset;
    out_image->width = width;
    out_image->height = height;
    out_image->pitch = pitch;
    out_image->bpp = bpp;
    out_image->bottom_up = ((descriptor & 0x20) == 0);
    out_image->bgr = true;
    return true;
}

bool image_load_any(const void* data, u32 size, image_t* out_image) {
    const u8* bytes = (const u8*)data;

    if (load_bmp(bytes, size, out_image)) {
        return true;
    }

    return load_tga(bytes, size, out_image);
}

static void read_image_pixel(const image_t* image, u32 source_x, u32 source_y, u8* out_red, u8* out_green, u8* out_blue) {
    u32 bytes_per_pixel = image->bpp / 8;
    const u8* pixel;
    u8 red;
    u8 green;
    u8 blue;

    if (image->bottom_up) {
        source_y = image->height - 1u - source_y;
    }

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

void image_blit_stretch(const image_t* image) {
    u32 screen_width = vga_width();
    u32 screen_height = vga_height();

    for (u32 y = 0; y < screen_height; ++y) {
        u32 source_y = (y * image->height) / screen_height;

        for (u32 x = 0; x < screen_width; ++x) {
            u32 source_x = (x * image->width) / screen_width;
            u8 red;
            u8 green;
            u8 blue;

            read_image_pixel(image, source_x, source_y, &red, &green, &blue);
            put_pixel(x, y, vga_color(red, green, blue));
        }
    }
}

void image_blit_stretch_bilinear(const image_t* image) {
    u32 screen_width = vga_width();
    u32 screen_height = vga_height();

    if (image == NULL || image->width == 0u || image->height == 0u) {
        return;
    }

    if (image->width == 1u || image->height == 1u) {
        image_blit_stretch(image);
        return;
    }

    for (u32 y = 0; y < screen_height; ++y) {
        u32 source_y_fixed = (y * (image->height - 1u) * 256u) / (screen_height > 1u ? (screen_height - 1u) : 1u);
        u32 source_y = source_y_fixed / 256u;
        u32 next_y = source_y + 1u < image->height ? source_y + 1u : source_y;
        u32 frac_y = source_y_fixed & 0xFFu;

        for (u32 x = 0; x < screen_width; ++x) {
            u32 source_x_fixed = (x * (image->width - 1u) * 256u) / (screen_width > 1u ? (screen_width - 1u) : 1u);
            u32 source_x = source_x_fixed / 256u;
            u32 next_x = source_x + 1u < image->width ? source_x + 1u : source_x;
            u32 frac_x = source_x_fixed & 0xFFu;
            u32 inv_x = 256u - frac_x;
            u32 inv_y = 256u - frac_y;
            u8 r00;
            u8 g00;
            u8 b00;
            u8 r10;
            u8 g10;
            u8 b10;
            u8 r01;
            u8 g01;
            u8 b01;
            u8 r11;
            u8 g11;
            u8 b11;
            u32 red;
            u32 green;
            u32 blue;

            read_image_pixel(image, source_x, source_y, &r00, &g00, &b00);
            read_image_pixel(image, next_x, source_y, &r10, &g10, &b10);
            read_image_pixel(image, source_x, next_y, &r01, &g01, &b01);
            read_image_pixel(image, next_x, next_y, &r11, &g11, &b11);

            red = (r00 * inv_x * inv_y + r10 * frac_x * inv_y + r01 * inv_x * frac_y + r11 * frac_x * frac_y) / 65536u;
            green = (g00 * inv_x * inv_y + g10 * frac_x * inv_y + g01 * inv_x * frac_y + g11 * frac_x * frac_y) / 65536u;
            blue = (b00 * inv_x * inv_y + b10 * frac_x * inv_y + b01 * inv_x * frac_y + b11 * frac_x * frac_y) / 65536u;

            put_pixel(x, y, vga_color((u8)red, (u8)green, (u8)blue));
        }
    }
}

void image_blit_fit_rounded(const image_t* image, u32 x, u32 y, u32 width, u32 height, u32 radius) {
    u32 draw_width;
    u32 draw_height;
    u32 draw_x;
    u32 draw_y;

    if (image == NULL || image->width == 0u || image->height == 0u || width == 0u || height == 0u) {
        return;
    }

    draw_width = width;
    draw_height = (width * image->height) / image->width;
    if (draw_height > height) {
        draw_height = height;
        draw_width = (height * image->width) / image->height;
    }

    if (draw_width == 0u) {
        draw_width = 1u;
    }
    if (draw_height == 0u) {
        draw_height = 1u;
    }

    draw_x = x + ((width > draw_width) ? (width - draw_width) / 2u : 0u);
    draw_y = y + ((height > draw_height) ? (height - draw_height) / 2u : 0u);

    if (image->width == 1u || image->height == 1u || draw_width == 1u || draw_height == 1u) {
        for (u32 dst_y = 0; dst_y < draw_height; ++dst_y) {
            u32 source_y = (dst_y * image->height) / draw_height;

            for (u32 dst_x = 0; dst_x < draw_width; ++dst_x) {
                u32 source_x = (dst_x * image->width) / draw_width;
                u8 red;
                u8 green;
                u8 blue;

                if (!rounded_rect_contains(dst_x, dst_y, draw_width, draw_height, radius)) {
                    continue;
                }

                read_image_pixel(image, source_x, source_y, &red, &green, &blue);
                put_pixel(draw_x + dst_x, draw_y + dst_y, vga_color(red, green, blue));
            }
        }
        return;
    }

    for (u32 dst_y = 0; dst_y < draw_height; ++dst_y) {
        u32 source_y_fixed = (dst_y * (image->height - 1u) * 256u) / (draw_height > 1u ? (draw_height - 1u) : 1u);
        u32 source_y = source_y_fixed / 256u;
        u32 next_y = source_y + 1u < image->height ? source_y + 1u : source_y;
        u32 frac_y = source_y_fixed & 0xFFu;
        u32 inv_y = 256u - frac_y;

        for (u32 dst_x = 0; dst_x < draw_width; ++dst_x) {
            u32 source_x_fixed = (dst_x * (image->width - 1u) * 256u) / (draw_width > 1u ? (draw_width - 1u) : 1u);
            u32 source_x = source_x_fixed / 256u;
            u32 next_x = source_x + 1u < image->width ? source_x + 1u : source_x;
            u32 frac_x = source_x_fixed & 0xFFu;
            u32 inv_x = 256u - frac_x;
            u8 r00;
            u8 g00;
            u8 b00;
            u8 r10;
            u8 g10;
            u8 b10;
            u8 r01;
            u8 g01;
            u8 b01;
            u8 r11;
            u8 g11;
            u8 b11;
            u32 red;
            u32 green;
            u32 blue;

            if (!rounded_rect_contains(dst_x, dst_y, draw_width, draw_height, radius)) {
                continue;
            }

            read_image_pixel(image, source_x, source_y, &r00, &g00, &b00);
            read_image_pixel(image, next_x, source_y, &r10, &g10, &b10);
            read_image_pixel(image, source_x, next_y, &r01, &g01, &b01);
            read_image_pixel(image, next_x, next_y, &r11, &g11, &b11);

            red = (r00 * inv_x * inv_y + r10 * frac_x * inv_y + r01 * inv_x * frac_y + r11 * frac_x * frac_y) / 65536u;
            green = (g00 * inv_x * inv_y + g10 * frac_x * inv_y + g01 * inv_x * frac_y + g11 * frac_x * frac_y) / 65536u;
            blue = (b00 * inv_x * inv_y + b10 * frac_x * inv_y + b01 * inv_x * frac_y + b11 * frac_x * frac_y) / 65536u;

            put_pixel(draw_x + dst_x, draw_y + dst_y, vga_color((u8)red, (u8)green, (u8)blue));
        }
    }
}

void image_blit_fit_rounded_transparent(const image_t* image, u32 x, u32 y, u32 width, u32 height, u32 radius, u32 transparent_color) {
    u32 draw_width;
    u32 draw_height;
    u32 draw_x;
    u32 draw_y;
    u8 t_r, t_g, t_b;

    if (image == NULL || image->width == 0u || image->height == 0u || width == 0u || height == 0u) {
        return;
    }

    t_r = (u8)(transparent_color >> 16);
    t_g = (u8)(transparent_color >> 8);
    t_b = (u8)transparent_color;

    draw_width = width;
    draw_height = (width * image->height) / image->width;
    if (draw_height > height) {
        draw_height = height;
        draw_width = (height * image->width) / image->height;
    }

    if (draw_width == 0u) {
        draw_width = 1u;
    }
    if (draw_height == 0u) {
        draw_height = 1u;
    }

    draw_x = x + ((width > draw_width) ? (width - draw_width) / 2u : 0u);
    draw_y = y + ((height > draw_height) ? (height - draw_height) / 2u : 0u);

    for (u32 dst_y = 0; dst_y < draw_height; ++dst_y) {
        u32 source_y_fixed = (dst_y * (image->height - 1u) * 256u) / (draw_height > 1u ? (draw_height - 1u) : 1u);
        u32 source_y = source_y_fixed / 256u;
        u32 next_y = source_y + 1u < image->height ? source_y + 1u : source_y;
        u32 frac_y = source_y_fixed & 0xFFu;
        u32 inv_y = 256u - frac_y;

        for (u32 dst_x = 0; dst_x < draw_width; ++dst_x) {
            u32 source_x_fixed = (dst_x * (image->width - 1u) * 256u) / (draw_width > 1u ? (draw_width - 1u) : 1u);
            u32 source_x = source_x_fixed / 256u;
            u32 next_x = source_x + 1u < image->width ? source_x + 1u : source_x;
            u32 frac_x = source_x_fixed & 0xFFu;
            u32 inv_x = 256u - frac_x;
            u8 r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
            u32 red, green, blue;

            if (!rounded_rect_contains(dst_x, dst_y, draw_width, draw_height, radius)) {
                continue;
            }

            read_image_pixel(image, source_x, source_y, &r00, &g00, &b00);
            read_image_pixel(image, next_x, source_y, &r10, &g10, &b10);
            read_image_pixel(image, source_x, next_y, &r01, &g01, &b01);
            read_image_pixel(image, next_x, next_y, &r11, &g11, &b11);

            red = (r00 * inv_x * inv_y + r10 * frac_x * inv_y + r01 * inv_x * frac_y + r11 * frac_x * frac_y) / 65536u;
            green = (g00 * inv_x * inv_y + g10 * frac_x * inv_y + g01 * inv_x * frac_y + g11 * frac_x * frac_y) / 65536u;
            blue = (b00 * inv_x * inv_y + b10 * frac_x * inv_y + b01 * inv_x * frac_y + b11 * frac_x * frac_y) / 65536u;

            if (red == t_r && green == t_g && blue == t_b) {
                continue;
            }

            put_pixel(draw_x + dst_x, draw_y + dst_y, vga_color((u8)red, (u8)green, (u8)blue));
        }
    }
}
