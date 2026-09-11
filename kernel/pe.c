#include "jabulos.h"

struct __attribute__((packed)) pe_section_header {
    char name[8];
    u32 virtual_size;
    u32 virtual_address;
    u32 size_of_raw_data;
    u32 pointer_to_raw_data;
    u32 pointer_to_relocations;
    u32 pointer_to_linenumbers;
    u16 number_of_relocations;
    u16 number_of_linenumbers;
    u32 characteristics;
};

typedef u32 (*pe_entry_point_t)(pe_host_api_t* host_api);

static u16 pe_read_le16(const u8* data) {
    return (u16)data[0] | ((u16)data[1] << 8);
}

static u32 pe_read_le32(const u8* data) {
    return (u32)data[0] |
           ((u32)data[1] << 8) |
           ((u32)data[2] << 16) |
           ((u32)data[3] << 24);
}

static u32 pe_min_u32(u32 left, u32 right) {
    return left < right ? left : right;
}

static u32 pe_max_u32(u32 left, u32 right) {
    return left > right ? left : right;
}

static void pe_copy_text(char* destination, const char* source, u32 capacity) {
    u32 index = 0;

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

static void* pe_alloc_contiguous(u32 size) {
    u32 frame_count;
    u64 first_frame = 0;

    if (size == 0u) {
        return NULL;
    }

    frame_count = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    for (u32 index = 0; index < frame_count; ++index) {
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
    return (void*)(u64)first_frame;
}

static void pe_free_contiguous(void* base, u32 size) {
    u32 frame_count;
    u64 address;

    if (base == NULL || size == 0u) {
        return;
    }

    frame_count = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    address = (u64)base;
    for (u32 index = 0; index < frame_count; ++index) {
        pmm_free_frame(address + (u64)index * PAGE_SIZE);
    }
}

bool pe_parse_image(const void* data, u32 size, pe_image_info_t* out_info) {
    const u8* bytes = (const u8*)data;
    u32 pe_offset;
    u32 coff_offset;
    u32 optional_offset;
    u32 optional_size;
    u16 magic;
    u32 section_table_offset;
    u64 section_table_end;

    if (out_info == NULL) {
        return false;
    }

    memset(out_info, 0, sizeof(*out_info));

    if (data == NULL) {
        out_info->status = PE_PARSE_NULL_INPUT;
        return false;
    }

    if (size < 0x40u) {
        out_info->status = PE_PARSE_TOO_SMALL;
        return false;
    }

    if (bytes[0] != 'M' || bytes[1] != 'Z') {
        out_info->status = PE_PARSE_BAD_DOS_SIGNATURE;
        return false;
    }

    pe_offset = pe_read_le32(bytes + 0x3Cu);
    out_info->pe_offset = pe_offset;
    if ((u64)pe_offset + 24u > size) {
        out_info->status = PE_PARSE_TRUNCATED_HEADERS;
        return false;
    }

    if (bytes[pe_offset] != 'P' || bytes[pe_offset + 1u] != 'E' || bytes[pe_offset + 2u] != 0 || bytes[pe_offset + 3u] != 0) {
        out_info->status = PE_PARSE_BAD_PE_SIGNATURE;
        return false;
    }

    coff_offset = pe_offset + 4u;
    out_info->machine = pe_read_le16(bytes + coff_offset);
    out_info->section_count = pe_read_le16(bytes + coff_offset + 2u);
    optional_size = pe_read_le16(bytes + coff_offset + 16u);
    out_info->characteristics = pe_read_le16(bytes + coff_offset + 18u);

    optional_offset = coff_offset + 20u;
    if ((u64)optional_offset + optional_size > size || optional_size < 0x44u) {
        out_info->status = PE_PARSE_BAD_OPTIONAL_HEADER;
        return false;
    }

    magic = pe_read_le16(bytes + optional_offset);
    if (magic != 0x10Bu && magic != 0x20Bu) {
        out_info->status = PE_PARSE_UNSUPPORTED_MAGIC;
        return false;
    }

    out_info->pe32_plus = (magic == 0x20Bu);
    out_info->entry_rva = pe_read_le32(bytes + optional_offset + 16u);
    if (out_info->pe32_plus) {
        out_info->image_base_low = pe_read_le32(bytes + optional_offset + 24u);
        out_info->subsystem = pe_read_le16(bytes + optional_offset + 68u);
        out_info->dll_characteristics = pe_read_le16(bytes + optional_offset + 70u);
    } else {
        out_info->image_base_low = pe_read_le32(bytes + optional_offset + 28u);
        out_info->subsystem = pe_read_le16(bytes + optional_offset + 68u);
        out_info->dll_characteristics = pe_read_le16(bytes + optional_offset + 70u);
    }
    out_info->size_of_image = pe_read_le32(bytes + optional_offset + 56u);
    out_info->size_of_headers = pe_read_le32(bytes + optional_offset + 60u);

    section_table_offset = optional_offset + optional_size;
    section_table_end = (u64)section_table_offset + (u64)out_info->section_count * sizeof(struct pe_section_header);
    if (section_table_end > size) {
        out_info->status = PE_PARSE_TRUNCATED_HEADERS;
        return false;
    }

    if (out_info->section_count > 0u) {
        for (u32 index = 0; index < 8u; ++index) {
            char ch = (char)bytes[section_table_offset + index];
            out_info->first_section_name[index] = ch;
            if (ch == '\0') {
                break;
            }
        }
        out_info->first_section_name[8] = '\0';
    }

    out_info->valid = true;
    out_info->status = PE_PARSE_OK;
    return true;
}

bool pe_launch_image(const void* data, u32 size, pe_launch_result_t* out_result) {
    const u8* bytes = (const u8*)data;
    u32 coff_offset;
    u32 optional_offset;
    u32 optional_size;
    u32 section_table_offset;
    const struct pe_section_header* section_headers;
    void* image_base = NULL;
    pe_host_api_t host_api;
    pe_entry_point_t entry_point;

    if (out_result == NULL) {
        return false;
    }

    memset(out_result, 0, sizeof(*out_result));

    if (data == NULL) {
        out_result->status = PE_LAUNCH_NULL_INPUT;
        return false;
    }

    if (!pe_parse_image(data, size, &out_result->image)) {
        out_result->status = PE_LAUNCH_PARSE_FAILED;
        return false;
    }

    if (!out_result->image.pe32_plus) {
        out_result->status = PE_LAUNCH_UNSUPPORTED_FORMAT;
        return false;
    }

    if (out_result->image.machine != PE_MACHINE_AMD64) {
        out_result->status = PE_LAUNCH_UNSUPPORTED_MACHINE;
        return false;
    }

    if (out_result->image.size_of_image == 0u ||
        out_result->image.size_of_headers == 0u ||
        out_result->image.size_of_headers > out_result->image.size_of_image ||
        out_result->image.entry_rva >= out_result->image.size_of_image) {
        out_result->status = PE_LAUNCH_BAD_LAYOUT;
        return false;
    }

    coff_offset = out_result->image.pe_offset + 4u;
    optional_size = pe_read_le16(bytes + coff_offset + 16u);
    optional_offset = coff_offset + 20u;
    section_table_offset = optional_offset + optional_size;
    section_headers = (const struct pe_section_header*)(bytes + section_table_offset);

    image_base = pe_alloc_contiguous(out_result->image.size_of_image);
    if (image_base == NULL) {
        out_result->status = PE_LAUNCH_NO_MEMORY;
        return false;
    }

    memcpy(image_base, data, pe_min_u32(out_result->image.size_of_headers, size));
    for (u32 index = 0; index < out_result->image.section_count; ++index) {
        const struct pe_section_header* section = &section_headers[index];
        u32 memory_size = pe_max_u32(section->virtual_size, section->size_of_raw_data);

        if (section->virtual_address >= out_result->image.size_of_image ||
            memory_size > out_result->image.size_of_image - section->virtual_address) {
            out_result->status = PE_LAUNCH_BAD_LAYOUT;
            pe_free_contiguous(image_base, out_result->image.size_of_image);
            return false;
        }

        if (section->size_of_raw_data > 0u) {
            if (section->pointer_to_raw_data >= size ||
                section->size_of_raw_data > size - section->pointer_to_raw_data) {
                out_result->status = PE_LAUNCH_BAD_LAYOUT;
                pe_free_contiguous(image_base, out_result->image.size_of_image);
                return false;
            }

            memcpy((u8*)image_base + section->virtual_address,
                   bytes + section->pointer_to_raw_data,
                   section->size_of_raw_data);
        }
    }

    entry_point = (pe_entry_point_t)((u8*)image_base + out_result->image.entry_rva);
    if (entry_point == NULL) {
        out_result->status = PE_LAUNCH_BAD_ENTRY;
        pe_free_contiguous(image_base, out_result->image.size_of_image);
        return false;
    }

    memset(&host_api, 0, sizeof(host_api));
    host_api.magic = PE_HOST_API_MAGIC;
    host_api.version = PE_HOST_API_VERSION;
    host_api.message[0] = '\0';

    out_result->return_code = entry_point(&host_api);
    out_result->launched = true;
    out_result->status = PE_LAUNCH_OK;
    pe_copy_text(out_result->message, host_api.message, sizeof(out_result->message));

    pe_free_contiguous(image_base, out_result->image.size_of_image);
    return true;
}

bool pe_launch_initrd_app(const void* archive, u32 archive_size, const char* name, pe_launch_result_t* out_result) {
    initrd_file_t file;

    if (out_result == NULL) {
        return false;
    }

    memset(out_result, 0, sizeof(*out_result));

    if (archive == NULL || name == NULL) {
        out_result->status = PE_LAUNCH_NULL_INPUT;
        return false;
    }

    if (!initrd_find_file(archive, archive_size, name, &file)) {
        out_result->status = PE_LAUNCH_FILE_NOT_FOUND;
        return false;
    }

    return pe_launch_image(file.data, file.size, out_result);
}
