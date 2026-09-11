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

void image_blit_stretch(const image_t* image) {
    u32 screen_width = vga_width();
    u32 screen_height = vga_height();
    u32 bytes_per_pixel = image->bpp / 8;

    for (u32 y = 0; y < screen_height; ++y) {
        u32 source_y = (y * image->height) / screen_height;
        if (image->bottom_up) {
            source_y = image->height - 1 - source_y;
        }

        for (u32 x = 0; x < screen_width; ++x) {
            u32 source_x = (x * image->width) / screen_width;
            const u8* pixel = image->pixels + source_y * image->pitch + source_x * bytes_per_pixel;
            u8 blue = pixel[0];
            u8 green = pixel[1];
            u8 red = pixel[2];

            if (!image->bgr) {
                red = pixel[0];
                green = pixel[1];
                blue = pixel[2];
            }

            put_pixel(x, y, vga_color(red, green, blue));
        }
    }
}
