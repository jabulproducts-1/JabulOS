#include "jabulos.h"

void* memset(void* destination, int value, size_t count) {
    u64* d64 = (u64*)destination;
    u8* d8;
    u64 v64 = (u8)value;
    v64 |= v64 << 8;
    v64 |= v64 << 16;
    v64 |= v64 << 32;

    size_t count64 = count / 8;
    size_t count8 = count % 8;

    while (count64-- > 0) {
        *d64++ = v64;
    }

    d8 = (u8*)d64;
    while (count8-- > 0) {
        *d8++ = (u8)value;
    }

    return destination;
}

void* memcpy(void* destination, const void* source, size_t count) {
    u64* d64 = (u64*)destination;
    const u64* s64 = (const u64*)source;
    u8* d8;
    const u8* s8;

    size_t count64 = count / 8;
    size_t count8 = count % 8;

    while (count64-- > 0) {
        *d64++ = *s64++;
    }

    d8 = (u8*)d64;
    s8 = (const u8*)s64;
    while (count8-- > 0) {
        *d8++ = *s8++;
    }

    return destination;
}

void* memmove(void* destination, const void* source, size_t count) {
    u8* dst = (u8*)destination;
    const u8* src = (const u8*)source;

    if (dst == src || count == 0) {
        return destination;
    }

    if (dst < src) {
        while (count-- > 0) {
            *dst++ = *src++;
        }
    } else {
        dst += count;
        src += count;
        while (count-- > 0) {
            *--dst = *--src;
        }
    }

    return destination;
}

int memcmp(const void* left, const void* right, size_t count) {
    const u8* lhs = (const u8*)left;
    const u8* rhs = (const u8*)right;

    while (count-- > 0) {
        if (*lhs != *rhs) {
            return (int)*lhs - (int)*rhs;
        }
        ++lhs;
        ++rhs;
    }

    return 0;
}

size_t strlen(const char* string) {
    size_t length = 0;
    while (string[length] != '\0') {
        ++length;
    }
    return length;
}

char* strcpy(char* destination, const char* source) {
    char* out = destination;
    while ((*destination++ = *source++) != '\0') {
    }
    return out;
}

char* strcat(char* destination, const char* source) {
    char* out = destination;
    while (*destination != '\0') {
        destination++;
    }
    while ((*destination++ = *source++) != '\0') {
    }
    return out;
}

int strcmp(const char* left, const char* right) {
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') {
        return (char*)haystack;
    }

    for (; *haystack != '\0'; haystack++) {
        if (*haystack != *needle) {
            continue;
        }

        const char* h = haystack;
        const char* n = needle;
        while (*h != '\0' && *n != '\0' && *h == *n) {
            h++;
            n++;
        }

        if (*n == '\0') {
            return (char*)haystack;
        }
    }

    return NULL;
}

int strncmp(const char* left, const char* right, size_t count) {
    while (count > 0 && *left != '\0' && *left == *right) {
        ++left;
        ++right;
        --count;
    }

    if (count == 0) {
        return 0;
    }

    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}
