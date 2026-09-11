#include "jabulos.h"

#define PMM_MAX_MEMORY_BYTES (8ULL * 1024ULL * 1024ULL * 1024ULL)
#define PMM_MAX_FRAMES (PMM_MAX_MEMORY_BYTES / PAGE_SIZE)
#define PMM_BITMAP_SIZE (PMM_MAX_FRAMES / 8)

static u8 frame_bitmap[PMM_BITMAP_SIZE];
static u64 frame_limit;
static u64 available_bytes;
static u64 free_bytes;
static u64 next_hint;

static void bitmap_mark(u64 frame, bool used) {
    if (frame >= frame_limit) {
        return;
    }

    u8 mask = (u8)(1u << (frame & 7u));
    u8* cell = &frame_bitmap[frame >> 3];
    bool currently_used = ((*cell & mask) != 0);

    if (currently_used == used) {
        return;
    }

    if (used) {
        *cell |= mask;
        free_bytes -= PAGE_SIZE;
    } else {
        *cell &= (u8)~mask;
        free_bytes += PAGE_SIZE;
    }
}

static void mark_region(u64 base, u64 size, bool used) {
    u64 start = ALIGN_DOWN(base, PAGE_SIZE);
    u64 end = ALIGN_UP(base + size, PAGE_SIZE);

    for (u64 address = start; address < end; address += PAGE_SIZE) {
        bitmap_mark(address / PAGE_SIZE, used);
    }
}

void pmm_initialize(const multiboot_tag_mmap_t* mmap_tag,
                    u64 kernel_start,
                    u64 kernel_end,
                    u64 mb_start,
                    u64 mb_end,
                    u64 initrd_start,
                    u64 initrd_size,
                    const framebuffer_info_t* framebuffer) {
    memset(frame_bitmap, 0xFF, sizeof(frame_bitmap));
    available_bytes = 0;
    free_bytes = 0;
    next_hint = 0;
    frame_limit = PMM_MAX_FRAMES;

    if (mmap_tag == NULL) {
        return;
    }

    const u8* cursor = (const u8*)mmap_tag->entries;
    const u8* end = ((const u8*)mmap_tag) + mmap_tag->size;

    while (cursor + sizeof(multiboot_mmap_entry_t) <= end) {
        const multiboot_mmap_entry_t* entry = (const multiboot_mmap_entry_t*)cursor;
        u64 clipped_end = entry->addr + entry->len;
        if (clipped_end > PMM_MAX_MEMORY_BYTES) {
            clipped_end = PMM_MAX_MEMORY_BYTES;
        }

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && clipped_end > entry->addr) {
            u64 clipped_len = clipped_end - entry->addr;
            available_bytes += clipped_len;
            mark_region(entry->addr, clipped_len, false);
        }

        cursor += mmap_tag->entry_size;
    }

    mark_region(0, 0x100000, true);
    mark_region(kernel_start, kernel_end - kernel_start, true);
    mark_region(mb_start, mb_end - mb_start, true);

    if (initrd_start != 0 && initrd_size != 0) {
        mark_region(initrd_start, initrd_size, true);
    }

    if (framebuffer != NULL && framebuffer->address != 0) {
        mark_region(framebuffer->address, (u64)framebuffer->pitch * framebuffer->height, true);
    }
}

u64 pmm_alloc_frame(void) {
    for (u64 frame = next_hint; frame < frame_limit; ++frame) {
        u8 mask = (u8)(1u << (frame & 7u));
        if ((frame_bitmap[frame >> 3] & mask) == 0) {
            bitmap_mark(frame, true);
            next_hint = frame + 1;
            return frame * PAGE_SIZE;
        }
    }

    for (u64 frame = 0; frame < next_hint; ++frame) {
        u8 mask = (u8)(1u << (frame & 7u));
        if ((frame_bitmap[frame >> 3] & mask) == 0) {
            bitmap_mark(frame, true);
            next_hint = frame + 1;
            return frame * PAGE_SIZE;
        }
    }

    return 0;
}

void pmm_free_frame(u64 address) {
    bitmap_mark(address / PAGE_SIZE, false);
    if ((address / PAGE_SIZE) < next_hint) {
        next_hint = address / PAGE_SIZE;
    }
}

u64 pmm_total_memory(void) {
    return available_bytes;
}

u64 pmm_free_memory(void) {
    return free_bytes;
}
