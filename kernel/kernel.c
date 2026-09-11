#include "jabulos.h"
#include "xhci.h"

extern u8 __kernel_start[];
extern u8 __kernel_end[];

static bool parse_boot_info(u64 multiboot_addr, u64 magic, boot_info_t* out_info) {
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC || out_info == NULL) {
        return false;
    }

    memset(out_info, 0, sizeof(*out_info));
    out_info->mb_info_start = multiboot_addr;

    const multiboot_info_t* info = (const multiboot_info_t*)multiboot_addr;
    out_info->mb_info_end = multiboot_addr + info->total_size;
    out_info->kernel_start = (u64)__kernel_start;
    out_info->kernel_end = (u64)__kernel_end;

    const u8* cursor = (const u8*)multiboot_addr + 8;
    const u8* end = (const u8*)multiboot_addr + info->total_size;

    while (cursor + sizeof(multiboot_tag_t) <= end) {
        const multiboot_tag_t* tag = (const multiboot_tag_t*)cursor;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            out_info->mmap_tag = (const void*)tag;
        } else if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            const multiboot_tag_framebuffer_t* fb_tag = (const multiboot_tag_framebuffer_t*)tag;
            if (fb_tag->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB &&
                fb_tag->framebuffer_width != 0 &&
                fb_tag->framebuffer_height != 0 &&
                fb_tag->framebuffer_bpp >= 24) {
                out_info->framebuffer.address = fb_tag->framebuffer_addr;
                out_info->framebuffer.pitch = fb_tag->framebuffer_pitch;
                out_info->framebuffer.width = fb_tag->framebuffer_width;
                out_info->framebuffer.height = fb_tag->framebuffer_height;
                out_info->framebuffer.bpp = fb_tag->framebuffer_bpp;
                out_info->framebuffer.red_position = fb_tag->red_field_position;
                out_info->framebuffer.red_mask_size = fb_tag->red_mask_size;
                out_info->framebuffer.green_position = fb_tag->green_field_position;
                out_info->framebuffer.green_mask_size = fb_tag->green_mask_size;
                out_info->framebuffer.blue_position = fb_tag->blue_field_position;
                out_info->framebuffer.blue_mask_size = fb_tag->blue_mask_size;
            }
        } else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            const multiboot_tag_module_t* module = (const multiboot_tag_module_t*)tag;
            if (out_info->initrd_start == 0 || strcmp(module->cmdline, "initrd") == 0) {
                out_info->initrd_start = module->mod_start;
                out_info->initrd_size = module->mod_end - module->mod_start;
                out_info->initrd_name = module->cmdline;
            }
        }

        cursor += ALIGN_UP(tag->size, 8);
    }

    return out_info->framebuffer.address != 0 && out_info->mmap_tag != NULL;
}

static void boot_log_write(const char* text, bool show_on_screen) {
    serial_write(text);
    if (show_on_screen) {
        gui_boot_log_append(text);
    }
}

void kernel_main(u64 multiboot_addr, u64 magic) {
    boot_info_t boot_info;
    image_t wallpaper;
    image_t boot_splash;
    image_t splash_logo;
    image_t lock_bg;
    image_t lock_profile;
    bool lock_bg_loaded = false;
    bool lock_profile_loaded = false;
    const u8 default_ipv4_address[NET_IPV4_ADDRESS_LENGTH] = {10u, 0u, 2u, 15u};
    const u8 default_subnet_mask[NET_IPV4_ADDRESS_LENGTH] = {255u, 255u, 255u, 0u};
    const u8 default_gateway[NET_IPV4_ADDRESS_LENGTH] = {10u, 0u, 2u, 2u};
    const u8 default_dns_server[NET_IPV4_ADDRESS_LENGTH] = {10u, 0u, 2u, 3u};
    bool network_ready = false;
    bool network_ipv4_ready = false;
    bool wallpaper_loaded = false;
    bool boot_splash_loaded = false;
    bool splash_logo_loaded = false;
    bool ata_ready;

    serial_initialize();
    serial_write("[boot] entered kernel_main\n");
    serial_write("[boot] initializing gdt\n");
    gdt_initialize();
    serial_write("[boot] gdt initialized\n");
    serial_write("[boot] initializing idt\n");
    idt_initialize();
    serial_write("[boot] idt initialized\n");
    serial_write("[boot] descriptor tables initialized\n");
    timer_initialize();

    if (!parse_boot_info(multiboot_addr, magic, &boot_info)) {
        serial_write("[boot] multiboot parse failed at ");
        serial_write_hex64(multiboot_addr);
        serial_write("\n");
        halt_forever();
    }

    serial_write("[boot] multiboot structures parsed\n");

    pmm_initialize((const multiboot_tag_mmap_t*)boot_info.mmap_tag,
                   boot_info.kernel_start,
                   boot_info.kernel_end,
                   boot_info.mb_info_start,
                   boot_info.mb_info_end,
                   boot_info.initrd_start,
                   boot_info.initrd_size,
                   &boot_info.framebuffer);
    serial_write("[boot] physical memory manager initialized\n");
    network_ready = net_initialize();
    if (network_ready) {
        network_ipv4_ready = net_dhcp_negotiate();
        if (network_ipv4_ready) {
            serial_write("[boot] network stack initialized via dhcp\n");
        } else {
            net_dns_set_server(default_dns_server);
            network_ipv4_ready = net_configure_ipv4_static(default_ipv4_address,
                                                           default_subnet_mask,
                                                           default_gateway);
            if (network_ipv4_ready) {
                serial_write("[boot] network stack initialized via static fallback\n");
            } else {
                serial_write("[boot] network stack present but ipv4 configuration failed\n");
            }
        }
    } else {
        serial_write("[boot] network stack unavailable\n");
    }

    display_driver_initialize(&boot_info.framebuffer);
    xhci_initialize();
    gui_boot_log_reset();
    gui_boot_log_append("[boot] entered kernel_main");
    gui_boot_log_append("[boot] initializing gdt");
    gui_boot_log_append("[boot] gdt initialized");
    gui_boot_log_append("[boot] initializing idt");
    gui_boot_log_append("[boot] idt initialized");
    gui_boot_log_append("[boot] descriptor tables initialized");
    gui_boot_log_append("[boot] timer initialized");
    gui_boot_log_append("[boot] multiboot structures parsed");
    gui_boot_log_append("[boot] physical memory manager initialized");
    if (network_ipv4_ready) {
        gui_boot_log_append("[boot] network stack initialized");
    } else if (network_ready) {
        gui_boot_log_append("[boot] network stack present but ipv4 configuration failed");
    } else {
        gui_boot_log_append("[boot] network stack unavailable");
    }
    if (boot_info.initrd_start != 0 && boot_info.initrd_size != 0) {
        initrd_file_t file;
        if (initrd_find_file((const void*)boot_info.initrd_start, (u32)boot_info.initrd_size, "boot-splash.bmp", &file) &&
            image_load_any(file.data, file.size, &boot_splash)) {
            boot_splash_loaded = true;
            boot_log_write("[boot] boot splash asset loaded\n", true);
        }
        if (initrd_find_file((const void*)boot_info.initrd_start, (u32)boot_info.initrd_size, "JabulSplash.bmp", &file) &&
            image_load_any(file.data, file.size, &splash_logo)) {
            splash_logo_loaded = true;
            boot_log_write("[boot] splash logo asset loaded\n", true);
        }
        if (initrd_find_wallpaper((const void*)boot_info.initrd_start, (u32)boot_info.initrd_size, &file) &&
            image_load_any(file.data, file.size, &wallpaper)) {
            wallpaper_loaded = true;
            boot_log_write("[boot] wallpaper asset loaded\n", true);
        }
        if (initrd_find_file((const void*)boot_info.initrd_start, (u32)boot_info.initrd_size, "background.bmp", &file) &&
            image_load_any(file.data, file.size, &lock_bg)) {
            lock_bg_loaded = true;
            boot_log_write("[boot] lockscreen background loaded\n", true);
        }
        if (initrd_find_file((const void*)boot_info.initrd_start, (u32)boot_info.initrd_size, "profile.bmp", &file) &&
            image_load_any(file.data, file.size, &lock_profile)) {
            lock_profile_loaded = true;
            boot_log_write("[boot] lockscreen profile loaded\n", true);
        }
    }

    boot_log_write("[boot] framebuffer initialized\n", true);
 
    ata_ready = storage_initialize();
    boot_log_write("[boot] storage probe finished\n", true);

    gui_show_boot_sequence(&boot_info);

    boot_log_write("[boot] desktop session starting\n", true);
    gui_run_desktop(wallpaper_loaded ? &wallpaper : NULL,
                    boot_splash_loaded ? &boot_splash : NULL,
                    splash_logo_loaded ? &splash_logo : NULL,
                    lock_bg_loaded ? &lock_bg : NULL,
                    lock_profile_loaded ? &lock_profile : NULL,
                    &boot_info,
                    wallpaper_loaded,
                    ata_ready);
}
