#include "jabulos.h"

void display_driver_initialize(const framebuffer_info_t* framebuffer) {
    vga_initialize(framebuffer);
}

u32 display_driver_width(void) {
    return vga_width();
}

u32 display_driver_height(void) {
    return vga_height();
}

void display_driver_present(void) {
    vga_present();
}

void display_driver_present_region(u32 x, u32 y, u32 width, u32 height) {
    vga_present_region(x, y, width, height);
}

void display_driver_put_pixel_front(u32 x, u32 y, u32 color) {
    vga_put_pixel_front(x, y, color);
}

void display_driver_restore_frontbuffer_from_drawbuffer_region(u32 x, u32 y, u32 width, u32 height) {
    vga_restore_frontbuffer_from_drawbuffer_region(x, y, width, height);
}

void display_driver_set_clip_rect(u32 x, u32 y, u32 width, u32 height) {
    vga_set_clip_rect(x, y, width, height);
}

void display_driver_clear_clip_rect(void) {
    vga_clear_clip_rect();
}

void display_driver_draw_wallpaper(const image_t* image) {
    if (image == NULL) {
        return;
    }

    image_blit_stretch_bilinear(image);
}

u32 display_driver_framebuffer_bytes(void) {
    return vga_framebuffer_bytes();
}

void display_driver_copy_drawbuffer(void* destination, u32 size) {
    vga_copy_drawbuffer(destination, size);
}

void display_driver_copy_drawbuffer_region(void* destination, u32 size, u32 x, u32 y, u32 width, u32 height) {
    vga_copy_drawbuffer_region(destination, size, x, y, width, height);
}

void display_driver_restore_drawbuffer(const void* source, u32 size) {
    vga_restore_drawbuffer(source, size);
}

void display_driver_restore_drawbuffer_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height) {
    vga_restore_drawbuffer_region(source, size, x, y, width, height);
}

void display_driver_restore_drawbuffer_surface_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height) {
    vga_restore_drawbuffer_surface_region(source, size, x, y, width, height);
}

void display_driver_blit_rgbx32_fit(const u32* pixels, u32 source_width, u32 source_height, u32 clear_color) {
    vga_blit_rgbx32_fit(pixels, source_width, source_height, clear_color);
}
