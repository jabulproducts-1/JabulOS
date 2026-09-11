#include "jabulos.h"

#define AC97_BDL_ENTRY_COUNT 16u
#define AC97_BUFFER_BYTES 4096u
#define AC97_PCM_OUT_BASE 0x10u

#define AC97_MIXER_RESET 0x00u
#define AC97_MIXER_MASTER_VOLUME 0x02u
#define AC97_MIXER_PCM_VOLUME 0x18u
#define AC97_MIXER_EXTENDED_CAPS 0x28u
#define AC97_MIXER_EXTENDED_CTRL 0x2Au
#define AC97_MIXER_FRONT_DAC_RATE 0x2Cu

#define AC97_BM_BDBAR 0x00u
#define AC97_BM_CIV 0x04u
#define AC97_BM_LVI 0x05u
#define AC97_BM_SR 0x06u
#define AC97_BM_PIV 0x0Au
#define AC97_BM_CR 0x0Bu

#define AC97_CR_RUN 0x01u
#define AC97_CR_RESET 0x02u

#define AC97_SR_DCH 0x0001u
#define AC97_SR_CELV 0x0002u
#define AC97_SR_LVBCI 0x0004u
#define AC97_SR_BCIS 0x0008u
#define AC97_SR_FIFOE 0x0010u

#define AC97_EXTCAP_VRA 0x0001u

typedef struct {
    u32 address;
    u32 control_length;
} ac97_bdl_entry_t;

typedef struct {
    bool probed;
    bool available;
    bool playing;
    pci_device_info_t pci_device;
    u16 mixer_base;
    u16 bus_master_base;
    u32 output_rate;
    const u8* source_data;
    u32 source_sample_count;
    u32 source_index;
    u32 source_rate;
    u32 resample_accumulator;
    bool source_drained;
    u8 next_fill_index;
    u8 last_valid_index;
    ac97_bdl_entry_t* bdl;
    u8* dma_buffers;
} ac97_state_t;

static ac97_state_t g_ac97;

static void* ac97_alloc_contiguous(u32 size) {
    u32 frame_count;
    u64 first_frame = 0u;

    if (size == 0u) {
        return NULL;
    }

    frame_count = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    for (u32 index = 0u; index < frame_count; ++index) {
        u64 frame = pmm_alloc_frame();
        if (frame == 0u) {
            for (u32 release = 0u; release < index; ++release) {
                pmm_free_frame(first_frame + (u64)release * PAGE_SIZE);
            }
            return NULL;
        }

        if (index == 0u) {
            first_frame = frame;
        } else if (frame != first_frame + (u64)index * PAGE_SIZE) {
            for (u32 release = 0u; release < index; ++release) {
                pmm_free_frame(first_frame + (u64)release * PAGE_SIZE);
            }
            pmm_free_frame(frame);
            return NULL;
        }
    }

    memset((void*)(u64)first_frame, 0, size);
    return (void*)(u64)first_frame;
}

static void ac97_free_contiguous(void* base, u32 size) {
    u32 frame_count;
    u64 address;

    if (base == NULL || size == 0u) {
        return;
    }

    frame_count = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    address = (u64)base;
    for (u32 index = 0u; index < frame_count; ++index) {
        pmm_free_frame(address + (u64)index * PAGE_SIZE);
    }
}

static u16 ac97_mixer_port(u16 offset) {
    return (u16)(g_ac97.mixer_base + offset);
}

static u16 ac97_bus_port(u16 offset) {
    return (u16)(g_ac97.bus_master_base + AC97_PCM_OUT_BASE + offset);
}

static void ac97_clear_status(void) {
    outw(ac97_bus_port(AC97_BM_SR), AC97_SR_LVBCI | AC97_SR_BCIS | AC97_SR_FIFOE);
}

static void ac97_reset_stream(void) {
    outb(ac97_bus_port(AC97_BM_CR), AC97_CR_RESET);
    for (u32 attempt = 0u; attempt < 100000u; ++attempt) {
        if ((inb(ac97_bus_port(AC97_BM_CR)) & AC97_CR_RESET) == 0u) {
            break;
        }
    }
    outb(ac97_bus_port(AC97_BM_CR), 0u);
    ac97_clear_status();
}

static bool ac97_prepare_dma_memory(void) {
    if (g_ac97.bdl != NULL && g_ac97.dma_buffers != NULL) {
        return true;
    }

    g_ac97.bdl = (ac97_bdl_entry_t*)ac97_alloc_contiguous(sizeof(ac97_bdl_entry_t) * AC97_BDL_ENTRY_COUNT);
    if (g_ac97.bdl == NULL) {
        return false;
    }

    g_ac97.dma_buffers = (u8*)ac97_alloc_contiguous(AC97_BDL_ENTRY_COUNT * AC97_BUFFER_BYTES);
    if (g_ac97.dma_buffers == NULL) {
        ac97_free_contiguous(g_ac97.bdl, sizeof(ac97_bdl_entry_t) * AC97_BDL_ENTRY_COUNT);
        g_ac97.bdl = NULL;
        return false;
    }

    return true;
}

static bool ac97_is_intel_style_controller(const pci_device_info_t* device) {
    if (device == NULL || !device->present) {
        return false;
    }

    if (device->class_code != 0x04u || device->subclass != 0x01u) {
        return false;
    }

    if ((device->bar[0] & 0x1u) == 0u || (device->bar[1] & 0x1u) == 0u) {
        return false;
    }

    return (device->bar[0] & ~0x3u) != 0u && (device->bar[1] & ~0x3u) != 0u;
}

static bool ac97_configure_output_rate(u32 sample_rate) {
    u16 extended_caps;
    u16 extended_ctrl;

    if (sample_rate == 0u) {
        return false;
    }

    extended_caps = inw(ac97_mixer_port(AC97_MIXER_EXTENDED_CAPS));
    if ((extended_caps & AC97_EXTCAP_VRA) != 0u) {
        extended_ctrl = inw(ac97_mixer_port(AC97_MIXER_EXTENDED_CTRL));
        outw(ac97_mixer_port(AC97_MIXER_EXTENDED_CTRL), (u16)(extended_ctrl | AC97_EXTCAP_VRA));
        outw(ac97_mixer_port(AC97_MIXER_FRONT_DAC_RATE), (u16)sample_rate);
        g_ac97.output_rate = sample_rate;
        return true;
    }

    g_ac97.output_rate = 48000u;
    return true;
}

static bool ac97_fill_descriptor(u8 descriptor_index) {
    u8* buffer;
    s16* samples;
    u32 frame_capacity;
    u32 produced_frames = 0u;

    if (g_ac97.source_data == NULL ||
        g_ac97.source_rate == 0u ||
        g_ac97.output_rate == 0u ||
        g_ac97.source_index >= g_ac97.source_sample_count) {
        return false;
    }

    buffer = g_ac97.dma_buffers + (u32)descriptor_index * AC97_BUFFER_BYTES;
    samples = (s16*)buffer;
    frame_capacity = AC97_BUFFER_BYTES / 4u;

    while (produced_frames < frame_capacity && g_ac97.source_index < g_ac97.source_sample_count) {
        s16 value = (s16)(((s32)g_ac97.source_data[g_ac97.source_index] - 128) << 8);
        u32 sample_position = produced_frames * 2u;

        samples[sample_position] = value;
        samples[sample_position + 1u] = value;
        ++produced_frames;

        g_ac97.resample_accumulator += g_ac97.source_rate;
        while (g_ac97.resample_accumulator >= g_ac97.output_rate) {
            g_ac97.resample_accumulator -= g_ac97.output_rate;
            ++g_ac97.source_index;
            if (g_ac97.source_index >= g_ac97.source_sample_count) {
                break;
            }
        }
    }

    if (produced_frames == 0u) {
        return false;
    }

    memset(buffer + produced_frames * 4u, 0, AC97_BUFFER_BYTES - produced_frames * 4u);
    g_ac97.bdl[descriptor_index].address = (u32)(u64)buffer;
    g_ac97.bdl[descriptor_index].control_length = (produced_frames * 4u) / 2u;
    return true;
}

static bool ac97_slot_available(u8 descriptor_index) {
    u8 current_index;
    u8 prefetched_index;

    if (!g_ac97.playing) {
        return descriptor_index != 0u;
    }

    current_index = inb(ac97_bus_port(AC97_BM_CIV));
    prefetched_index = inb(ac97_bus_port(AC97_BM_PIV));
    return descriptor_index != current_index && descriptor_index != prefetched_index;
}

static bool ac97_queue_descriptors(void) {
    bool queued_any = false;

    while (ac97_slot_available(g_ac97.next_fill_index)) {
        if (!ac97_fill_descriptor(g_ac97.next_fill_index)) {
            g_ac97.source_drained = true;
            break;
        }

        g_ac97.last_valid_index = g_ac97.next_fill_index;
        outb(ac97_bus_port(AC97_BM_LVI), g_ac97.last_valid_index);
        g_ac97.next_fill_index = (u8)((g_ac97.next_fill_index + 1u) % AC97_BDL_ENTRY_COUNT);
        queued_any = true;
    }

    return queued_any;
}

bool ac97_detect(void) {
    pci_device_info_t device;
    u16 pci_command;
    u16 mixer_probe;

    if (g_ac97.probed) {
        return g_ac97.available;
    }

    memset(&g_ac97, 0, sizeof(g_ac97));
    g_ac97.probed = true;

    if (!pci_find_class_device(0x04u, 0x01u, &device)) {
        return false;
    }

    if (!ac97_is_intel_style_controller(&device)) {
        serial_write("[audio] pci audio device is not Intel-style AC97\n");
        return false;
    }

    g_ac97.pci_device = device;
    g_ac97.mixer_base = (u16)(device.bar[0] & ~0x3u);
    g_ac97.bus_master_base = (u16)(device.bar[1] & ~0x3u);

    pci_command = pci_read_config_word(device.bus, device.device, device.function, 0x04u);
    pci_command |= 0x0005u;
    pci_write_config_word(device.bus, device.device, device.function, 0x04u, pci_command);

    outw(ac97_mixer_port(AC97_MIXER_RESET), 0x0001u);
    for (u32 wait = 0u; wait < 65536u; ++wait) {
        io_wait();
    }

    mixer_probe = inw(ac97_mixer_port(AC97_MIXER_RESET));
    if (mixer_probe == 0xFFFFu) {
        return false;
    }

    if (!ac97_prepare_dma_memory()) {
        return false;
    }

    outw(ac97_mixer_port(AC97_MIXER_MASTER_VOLUME), 0x0000u);
    outw(ac97_mixer_port(AC97_MIXER_PCM_VOLUME), 0x0000u);
    ac97_reset_stream();

    serial_write("[audio] ac97 detected vendor=");
    serial_write_hex64(device.vendor_id);
    serial_write(" device=");
    serial_write_hex64(device.device_id);
    serial_write(" mixer=");
    serial_write_hex64(g_ac97.mixer_base);
    serial_write(" bus=");
    serial_write_hex64(g_ac97.bus_master_base);
    serial_write("\n");

    g_ac97.available = true;
    return true;
}

bool ac97_begin_stream(const u8* audio_data, u32 sample_count, u32 sample_rate) {
    if (!ac97_detect() ||
        audio_data == NULL ||
        sample_count == 0u ||
        sample_rate == 0u) {
        return false;
    }

    ac97_end_stream();
    memset(g_ac97.bdl, 0, sizeof(ac97_bdl_entry_t) * AC97_BDL_ENTRY_COUNT);
    memset(g_ac97.dma_buffers, 0, AC97_BDL_ENTRY_COUNT * AC97_BUFFER_BYTES);

    g_ac97.source_data = audio_data;
    g_ac97.source_sample_count = sample_count;
    g_ac97.source_index = 0u;
    g_ac97.source_rate = sample_rate;
    g_ac97.resample_accumulator = 0u;
    g_ac97.source_drained = false;
    g_ac97.next_fill_index = 1u;
    g_ac97.last_valid_index = 0u;

    if (!ac97_configure_output_rate(sample_rate)) {
        return false;
    }

    ac97_reset_stream();
    outl(ac97_bus_port(AC97_BM_BDBAR), (u32)(u64)g_ac97.bdl);

    if (!ac97_fill_descriptor(0u)) {
        return false;
    }

    outb(ac97_bus_port(AC97_BM_LVI), 0u);
    (void)ac97_queue_descriptors();
    ac97_clear_status();
    outb(ac97_bus_port(AC97_BM_CR), AC97_CR_RUN);
    g_ac97.playing = true;
    return true;
}

bool ac97_service(void) {
    u16 status;

    if (!g_ac97.playing) {
        return false;
    }

    status = inw(ac97_bus_port(AC97_BM_SR));
    if ((status & AC97_SR_FIFOE) != 0u) {
        ac97_end_stream();
        return false;
    }

    ac97_clear_status();
    if (!g_ac97.source_drained) {
        (void)ac97_queue_descriptors();
    }

    status = inw(ac97_bus_port(AC97_BM_SR));
    if ((status & AC97_SR_DCH) != 0u) {
        ac97_end_stream();
        return false;
    }

    return true;
}

void ac97_end_stream(void) {
    if (!g_ac97.available) {
        return;
    }

    outb(ac97_bus_port(AC97_BM_CR), 0u);
    ac97_reset_stream();
    g_ac97.playing = false;
    g_ac97.source_data = NULL;
    g_ac97.source_sample_count = 0u;
    g_ac97.source_index = 0u;
    g_ac97.source_rate = 0u;
    g_ac97.resample_accumulator = 0u;
    g_ac97.source_drained = false;
    g_ac97.next_fill_index = 0u;
    g_ac97.last_valid_index = 0u;
}

bool ac97_is_active(void) {
    return g_ac97.playing;
}
