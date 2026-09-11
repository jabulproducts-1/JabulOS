#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB 1

typedef struct __attribute__((packed)) {
    u32 total_size;
    u32 reserved;
} multiboot_info_t;

typedef struct __attribute__((packed)) {
    u32 type;
    u32 size;
} multiboot_tag_t;

typedef struct __attribute__((packed)) {
    u32 type;
    u32 size;
    u32 mod_start;
    u32 mod_end;
    char cmdline[];
} multiboot_tag_module_t;

typedef struct __attribute__((packed)) {
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
} multiboot_mmap_entry_t;

typedef struct __attribute__((packed)) {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    multiboot_mmap_entry_t entries[];
} multiboot_tag_mmap_t;

typedef struct __attribute__((packed)) {
    u32 type;
    u32 size;
    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u8 framebuffer_bpp;
    u8 framebuffer_type;
    u16 reserved;
    u8 red_field_position;
    u8 red_mask_size;
    u8 green_field_position;
    u8 green_mask_size;
    u8 blue_field_position;
    u8 blue_mask_size;
} multiboot_tag_framebuffer_t;

#endif
