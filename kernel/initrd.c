#include "jabulos.h"

struct __attribute__((packed)) tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
};

static u32 octal_to_u32(const char* value, u32 length) {
    u32 result = 0;
    for (u32 index = 0; index < length && value[index] != '\0'; ++index) {
        if (value[index] < '0' || value[index] > '7') {
            continue;
        }
        result = (result << 3) + (u32)(value[index] - '0');
    }
    return result;
}

static bool has_extension(const char* name, const char* extension) {
    size_t name_len = strlen(name);
    size_t ext_len = strlen(extension);

    if (name_len < ext_len) {
        return false;
    }

    return strcmp(name + name_len - ext_len, extension) == 0;
}

bool initrd_find_file(const void* archive, u32 size, const char* name, initrd_file_t* out_file) {
    if (archive == NULL || out_file == NULL || name == NULL) {
        return false;
    }

    const u8* cursor = (const u8*)archive;
    const u8* end = cursor + size;

    while (cursor + 512 <= end) {
        const struct tar_header* header = (const struct tar_header*)cursor;
        if (header->name[0] == '\0') {
            return false;
        }

        u32 file_size = octal_to_u32(header->size, sizeof(header->size));
        const void* file_data = cursor + 512;

        if (strcmp(header->name, name) == 0 && cursor + 512 + file_size <= end) {
            out_file->data = file_data;
            out_file->size = file_size;
            out_file->name = header->name;
            return true;
        }

        cursor += 512 + ALIGN_UP(file_size, 512);
    }

    return false;
}

bool initrd_find_wallpaper(const void* archive, u32 size, initrd_file_t* out_file) {
    if (initrd_find_file(archive, size, "wallpaper.bmp", out_file)) {
        return true;
    }

    if (archive == NULL || out_file == NULL) {
        return false;
    }

    const u8* cursor = (const u8*)archive;
    const u8* end = cursor + size;

    while (cursor + 512 <= end) {
        const struct tar_header* header = (const struct tar_header*)cursor;
        if (header->name[0] == '\0') {
            return false;
        }

        u32 file_size = octal_to_u32(header->size, sizeof(header->size));
        const void* file_data = cursor + 512;

        if ((has_extension(header->name, ".bmp") || has_extension(header->name, ".tga")) && cursor + 512 + file_size <= end) {
            out_file->data = file_data;
            out_file->size = file_size;
            out_file->name = header->name;
            return true;
        }

        cursor += 512 + ALIGN_UP(file_size, 512);
    }

    return false;
}

void initrd_list_files(const void* archive, u32 size, initrd_list_callback_t callback, void* user) {
    if (archive == NULL || callback == NULL) {
        return;
    }

    const u8* cursor = (const u8*)archive;
    const u8* end = cursor + size;

    while (cursor + 512 <= end) {
        const struct tar_header* header = (const struct tar_header*)cursor;
        if (header->name[0] == '\0') {
            return;
        }

        u32 file_size = octal_to_u32(header->size, sizeof(header->size));
        const void* file_data = cursor + 512;

        if (cursor + 512 + file_size <= end) {
            initrd_file_t file;
            file.data = file_data;
            file.size = file_size;
            file.name = header->name;
            if (!callback(&file, user)) {
                return;
            }
        }

        cursor += 512 + ALIGN_UP(file_size, 512);
    }
}
