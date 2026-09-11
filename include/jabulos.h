#ifndef JABULOS_H
#define JABULOS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned long long size_t;

#include "multiboot2.h"

#ifndef __cplusplus
typedef enum {
    false = 0,
    true = 1
} bool;
#endif

#ifdef __cplusplus
#define NULL nullptr
#else
#define NULL ((void*)0)
#endif
#define PAGE_SIZE 4096ULL
#define ALIGN_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))
#define ALIGN_DOWN(value, alignment) ((value) & ~((alignment) - 1))

static inline void io_wait(void) {
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}

static inline void outb(u16 port, u8 value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outw(u16 port, u16 value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outl(u16 port, u32 value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline u16 inw(u16 port) {
    u16 value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline u32 inl(u16 port) {
    u32 value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void insw(u16 port, void* destination, u32 count) {
    asm volatile ("rep insw" : "+D"(destination), "+c"(count) : "d"(port) : "memory");
}

static inline void outsw(u16 port, const void* source, u32 count) {
    asm volatile ("rep outsw" : "+S"(source), "+c"(count) : "d"(port));
}

static inline void halt_forever(void) {
    for (;;) {
        asm volatile ("cli; hlt");
    }
}

static inline void reboot_system(void) {
    asm volatile ("cli");
    for (u32 attempt = 0; attempt < 0x10000u; ++attempt) {
        if ((inb(0x64) & 0x02u) == 0u) {
            break;
        }
        io_wait();
    }
    outb(0x64, 0xFE);
    halt_forever();
}

static inline void power_off_system(void) {
    asm volatile ("cli");
    outw(0x604, 0x2000);
    io_wait();
    outw(0xB004, 0x2000);
    halt_forever();
}

static inline u8 mmio_read8(u64 address) {
    return *(volatile u8*)address;
}

static inline u16 mmio_read16(u64 address) {
    return *(volatile u16*)address;
}

static inline u32 mmio_read32(u64 address) {
    return *(volatile u32*)address;
}

static inline u64 mmio_read64(u64 address) {
    return *(volatile u64*)address;
}

static inline void mmio_write8(u64 address, u8 value) {
    *(volatile u8*)address = value;
}

static inline void mmio_write16(u64 address, u16 value) {
    *(volatile u16*)address = value;
}

static inline void mmio_write32(u64 address, u32 value) {
    *(volatile u32*)address = value;
}

static inline void mmio_write64(u64 address, u64 value) {
    *(volatile u64*)address = value;
}

typedef struct {
    u64 address;
    u32 pitch;
    u32 width;
    u32 height;
    u8 bpp;
    u8 red_position;
    u8 red_mask_size;
    u8 green_position;
    u8 green_mask_size;
    u8 blue_position;
    u8 blue_mask_size;
} framebuffer_info_t;

typedef struct {
    framebuffer_info_t framebuffer;
    const void* mmap_tag;
    u64 mb_info_start;
    u64 mb_info_end;
    u64 kernel_start;
    u64 kernel_end;
    u64 initrd_start;
    u64 initrd_size;
    const char* initrd_name;
} boot_info_t;

typedef struct {
    const u8* pixels;
    u32 width;
    u32 height;
    u32 pitch;
    u8 bpp;
    bool bottom_up;
    bool bgr;
} image_t;

typedef struct {
    const void* data;
    u32 size;
    const char* name;
} initrd_file_t;

typedef struct {
    u32 x;
    u32 y;
    bool initialized;
    bool left_down;
    bool right_down;
    u8 packet[3];
    u8 packet_index;
} mouse_driver_state_t;

#define ATA_PRIMARY_MASTER_PARTITION_COUNT 4u

typedef struct {
    bool present;
    bool bootable;
    u8 partition_type;
    u32 start_lba;
    u32 sector_count;
} ata_partition_info_t;

typedef struct {
    bool present;
    u32 total_sectors;
    char model[41];
    ata_partition_info_t partitions[ATA_PRIMARY_MASTER_PARTITION_COUNT];
} ata_device_info_t;

typedef struct {
    bool present;
    u8 bus;
    u8 device;
    u8 function;
    u16 vendor_id;
    u16 device_id;
    u8 class_code;
    u8 subclass;
    u8 prog_if;
    u8 revision_id;
    u32 bar[6];
    u8 irq_line;
    u8 irq_pin;
} pci_device_info_t;

typedef struct {
    bool present;
    bool initialized;
    bool io_space_enabled;
    bool bus_master_enabled;
    u16 vendor_id;
    u16 device_id;
    u16 io_base;
    u8 irq_line;
    u8 irq_pin;
    u8 mac[6];
} pcnet_status_t;

#define NET_MAC_ADDRESS_LENGTH 6u
#define NET_IPV4_ADDRESS_LENGTH 4u
#define NET_IPV4_PAYLOAD_MAX 1480u
#define NET_UDP_PAYLOAD_MAX 1472u
#define NET_TCP_PAYLOAD_MAX 1024u
#define NET_DNS_NAME_MAX 256u
#define NET_HTTP_URL_MAX 256u
#define NET_HTTP_CONTENT_TYPE_MAX 48u

#define NET_IPV4_PROTOCOL_ICMP 1u
#define NET_IPV4_PROTOCOL_TCP 6u
#define NET_IPV4_PROTOCOL_UDP 17u

typedef struct {
    u8 source_mac[NET_MAC_ADDRESS_LENGTH];
    u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 protocol;
    u16 payload_length;
    u8 payload[NET_IPV4_PAYLOAD_MAX];
} net_ipv4_packet_t;

typedef struct {
    bool initialized;
    bool ipv4_configured;
    u8 mac[NET_MAC_ADDRESS_LENGTH];
    u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH];
    u8 subnet_mask[NET_IPV4_ADDRESS_LENGTH];
    u8 gateway[NET_IPV4_ADDRESS_LENGTH];
    u32 rx_frames;
    u32 tx_frames;
    u32 dropped_frames;
    u32 queued_ipv4_packets;
    u32 arp_cache_entries;
} net_status_t;

typedef struct {
    u8 source_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH];
    u16 source_port;
    u16 destination_port;
    u16 payload_length;
    u8 payload[NET_UDP_PAYLOAD_MAX];
} net_udp_packet_t;

typedef struct {
    bool attempted;
    bool configured;
    u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH];
    u8 subnet_mask[NET_IPV4_ADDRESS_LENGTH];
    u8 gateway[NET_IPV4_ADDRESS_LENGTH];
    u8 dns_server[NET_IPV4_ADDRESS_LENGTH];
    u8 dhcp_server[NET_IPV4_ADDRESS_LENGTH];
    u32 lease_time_seconds;
} net_dhcp_status_t;

typedef struct {
    bool success;
    bool truncated;
    bool redirected;
    u16 status_code;
    u32 body_length;
    char final_url[NET_HTTP_URL_MAX];
    char content_type[NET_HTTP_CONTENT_TYPE_MAX];
    char message[96];
} net_http_result_t;

typedef enum {
    PE_PARSE_OK = 0,
    PE_PARSE_NULL_INPUT = 1,
    PE_PARSE_TOO_SMALL = 2,
    PE_PARSE_BAD_DOS_SIGNATURE = 3,
    PE_PARSE_BAD_PE_SIGNATURE = 4,
    PE_PARSE_BAD_OPTIONAL_HEADER = 5,
    PE_PARSE_UNSUPPORTED_MAGIC = 6,
    PE_PARSE_TRUNCATED_HEADERS = 7
} pe_parse_status_t;

typedef struct {
    bool valid;
    bool pe32_plus;
    pe_parse_status_t status;
    u16 machine;
    u16 section_count;
    u16 characteristics;
    u16 subsystem;
    u16 dll_characteristics;
    u32 pe_offset;
    u32 entry_rva;
    u32 image_base_low;
    u32 size_of_image;
    u32 size_of_headers;
    char first_section_name[9];
} pe_image_info_t;

#define PE_MACHINE_AMD64 0x8664u
#define PE_HOST_API_MAGIC 0x4A50454Cu
#define PE_HOST_API_VERSION 1u
#define PE_HOST_MESSAGE_CAPACITY 96u

typedef struct {
    u32 magic;
    u32 version;
    u32 status_code;
    u32 reserved;
    char message[PE_HOST_MESSAGE_CAPACITY];
} pe_host_api_t;

typedef enum {
    PE_LAUNCH_OK = 0,
    PE_LAUNCH_NULL_INPUT = 1,
    PE_LAUNCH_FILE_NOT_FOUND = 2,
    PE_LAUNCH_PARSE_FAILED = 3,
    PE_LAUNCH_UNSUPPORTED_MACHINE = 4,
    PE_LAUNCH_UNSUPPORTED_FORMAT = 5,
    PE_LAUNCH_BAD_LAYOUT = 6,
    PE_LAUNCH_NO_MEMORY = 7,
    PE_LAUNCH_BAD_ENTRY = 8
} pe_launch_status_t;

typedef struct {
    bool launched;
    pe_launch_status_t status;
    u32 return_code;
    pe_image_info_t image;
    char message[PE_HOST_MESSAGE_CAPACITY];
} pe_launch_result_t;

typedef struct {
    u16 scancode;
    char ascii;
    bool pressed;
    bool extended;
} game_key_event_t;

typedef struct {
    bool active;
    u32 width;
    u32 height;
    u32 pitch_bytes;
    u32 frame_count;
    u32 average_fps;
    u64 started_ms;
    const char* title;
} game_runtime_info_t;

typedef enum {
    GAME_FILE_SEEK_SET = 0,
    GAME_FILE_SEEK_CURRENT = 1,
    GAME_FILE_SEEK_END = 2
} game_file_seek_origin_t;

void* memset(void* destination, int value, size_t count);
void* memcpy(void* destination, const void* source, size_t count);
void* memmove(void* destination, const void* source, size_t count);
int memcmp(const void* left, const void* right, size_t count);
size_t strlen(const char* string);
char* strcpy(char* destination, const char* source);
char* strcat(char* destination, const char* source);
int strcmp(const char* left, const char* right);
int strncmp(const char* left, const char* right, size_t count);
char* strstr(const char* haystack, const char* needle);

void gdt_initialize(void);
void idt_initialize(void);
void isr_default_handler(u64* interrupt_frame);
void serial_initialize(void);
void serial_write(const char* text);
void serial_write_hex64(u64 value);
void serial_write_hex32(u32 value);
bool serial_read_byte(u8* out_value);

void pmm_initialize(const multiboot_tag_mmap_t* mmap_tag,
                    u64 kernel_start,
                    u64 kernel_end,
                    u64 mb_start,
                    u64 mb_end,
                    u64 initrd_start,
                    u64 initrd_size,
                    const framebuffer_info_t* framebuffer);
u64 pmm_alloc_frame(void);
void pmm_free_frame(u64 address);
u64 pmm_total_memory(void);
u64 pmm_free_memory(void);

void vga_initialize(const framebuffer_info_t* framebuffer);
u32 vga_color(u8 red, u8 green, u8 blue);
void put_pixel(u32 x, u32 y, u32 color);
void draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color);
void draw_rect_outline(u32 x, u32 y, u32 width, u32 height, u32 thickness, u32 color);
void draw_rounded_rect(u32 x, u32 y, u32 width, u32 height, u32 radius, u32 color);
void draw_rounded_rect_outline(u32 x, u32 y, u32 width, u32 height, u32 radius, u32 thickness, u32 color);
void vga_fill_rect_alpha(u32 x, u32 y, u32 width, u32 height, u8 red, u8 green, u8 blue, u8 alpha);
void vga_fill_rounded_rect_alpha(u32 x, u32 y, u32 width, u32 height, u32 radius, u8 red, u8 green, u8 blue, u8 alpha);
void draw_char(u32 x, u32 y, char character, u32 color);
void draw_text(u32 x, u32 y, const char* text, u32 color);
u32 vga_text_advance_x(void);
u32 vga_text_height(void);
void vga_clear(u32 color);
void vga_present(void);
void vga_present_region(u32 x, u32 y, u32 width, u32 height);
void vga_put_pixel_front(u32 x, u32 y, u32 color);
void vga_restore_frontbuffer_from_drawbuffer_region(u32 x, u32 y, u32 width, u32 height);
void vga_set_clip_rect(u32 x, u32 y, u32 width, u32 height);
void vga_clear_clip_rect(void);
u32 vga_width(void);
u32 vga_height(void);
u32 vga_framebuffer_bytes(void);
void vga_copy_drawbuffer(void* destination, u32 size);
void vga_copy_drawbuffer_region(void* destination, u32 size, u32 x, u32 y, u32 width, u32 height);
void vga_restore_drawbuffer(const void* source, u32 size);
void vga_restore_drawbuffer_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height);
void vga_restore_drawbuffer_surface_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height);
void vga_blit_rgbx32_fit(const u32* pixels, u32 source_width, u32 source_height, u32 clear_color);

bool image_load_any(const void* data, u32 size, image_t* out_image);
void image_blit_stretch(const image_t* image);
void image_blit_stretch_bilinear(const image_t* image);
void image_blit_fit_rounded(const image_t* image, u32 x, u32 y, u32 width, u32 height, u32 radius);
void image_blit_fit_rounded_transparent(const image_t* image, u32 x, u32 y, u32 width, u32 height, u32 radius, u32 transparent_color);

void display_driver_initialize(const framebuffer_info_t* framebuffer);
u32 display_driver_width(void);
u32 display_driver_height(void);
void display_driver_present(void);
void display_driver_present_region(u32 x, u32 y, u32 width, u32 height);
void display_driver_put_pixel_front(u32 x, u32 y, u32 color);
void display_driver_restore_frontbuffer_from_drawbuffer_region(u32 x, u32 y, u32 width, u32 height);
void display_driver_set_clip_rect(u32 x, u32 y, u32 width, u32 height);
void display_driver_clear_clip_rect(void);
void display_driver_draw_wallpaper(const image_t* image);
u32 display_driver_framebuffer_bytes(void);
void display_driver_copy_drawbuffer(void* destination, u32 size);
void display_driver_copy_drawbuffer_region(void* destination, u32 size, u32 x, u32 y, u32 width, u32 height);
void display_driver_restore_drawbuffer(const void* source, u32 size);
void display_driver_restore_drawbuffer_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height);
void display_driver_restore_drawbuffer_surface_region(const void* source, u32 size, u32 x, u32 y, u32 width, u32 height);
void display_driver_blit_rgbx32_fit(const u32* pixels, u32 source_width, u32 source_height, u32 clear_color);

void mouse_driver_initialize(mouse_driver_state_t* mouse);
bool mouse_driver_poll(mouse_driver_state_t* mouse);

typedef bool (*initrd_list_callback_t)(const initrd_file_t* file, void* user);

bool initrd_find_file(const void* archive, u32 size, const char* name, initrd_file_t* out_file);
bool initrd_find_wallpaper(const void* archive, u32 size, initrd_file_t* out_file);
void initrd_list_files(const void* archive, u32 size, initrd_list_callback_t callback, void* user);

void gui_show_boot_sequence(const boot_info_t* boot_info);
void gui_boot_log_reset(void);
void gui_boot_log_append(const char* text);
void gui_run_desktop(const image_t* wallpaper,
                     const image_t* boot_splash_image,
                     const image_t* splash_logo_image,
                     const image_t* lock_bg,
                     const image_t* lock_profile,
                     const boot_info_t* boot_info,
                     bool wallpaper_loaded,
                     bool ata_ready);
void timer_initialize(void);
u64 timer_ticks_ms(void);
void timer_sleep_ms(u32 milliseconds);
void game_runtime_initialize(const boot_info_t* boot_info);
bool game_runtime_start_fullscreen(u32 width, u32 height, const char* title);
void game_runtime_stop(void);
bool game_runtime_is_active(void);
u32* game_runtime_framebuffer(void);
u32 game_runtime_framebuffer_width(void);
u32 game_runtime_framebuffer_height(void);
u32 game_runtime_framebuffer_pitch_bytes(void);
void game_runtime_present(void);
void game_runtime_get_info(game_runtime_info_t* out_info);
void game_runtime_clear_key_events(void);
void game_runtime_push_key_event(u16 scancode, bool pressed, bool extended, char ascii);
bool game_runtime_poll_key_event(game_key_event_t* out_event);
void* game_runtime_alloc(u32 size);
void game_runtime_free(void* base, u32 size);
bool game_runtime_file_exists(const char* path);
int game_runtime_file_open(const char* path);
u32 game_runtime_file_size(int handle);
u32 game_runtime_file_tell(int handle);
bool game_runtime_file_seek(int handle, s32 offset, game_file_seek_origin_t origin);
u32 game_runtime_file_read(int handle, void* destination, u32 size);
void game_runtime_file_close(int handle);
void rtc_read_time_string(char* out_buffer);
void rtc_read_date_string(char* out_buffer);
u32 rtc_read_seconds_of_day(void);

bool ata_identify_primary_master(void);
bool ata_read_primary_master_info(ata_device_info_t* out_info);
bool ata_pio_read_sectors(u32 lba, u8 count, void* buffer);
bool ata_pio_write_sectors(u32 lba, u8 count, const void* buffer);

bool ahci_initialize(void);
bool ahci_read_sectors(u32 lba, u32 count, void* buffer);
bool ahci_write_sectors(u32 lba, u32 count, const void* buffer);
u32 ahci_get_total_sectors(void);

bool nvme_initialize(void);
bool nvme_read_sectors(u64 lba, u32 count, void* buffer);
bool nvme_write_sectors(u64 lba, u32 count, const void* buffer);

bool storage_initialize(void);
bool storage_read_sectors(u64 lba, u32 count, void* buffer);
bool storage_write_sectors(u64 lba, u32 count, const void* buffer);
bool storage_get_info(ata_device_info_t* out_info);

u32 pci_read_config_dword(u8 bus, u8 device, u8 function, u8 offset);
u16 pci_read_config_word(u8 bus, u8 device, u8 function, u8 offset);
u8 pci_read_config_byte(u8 bus, u8 device, u8 function, u8 offset);
void pci_write_config_dword(u8 bus, u8 device, u8 function, u8 offset, u32 value);
void pci_write_config_word(u8 bus, u8 device, u8 function, u8 offset, u16 value);
bool pci_find_class_device(u8 class_code, u8 subclass, pci_device_info_t* out_device);
bool pci_find_device(u16 vendor_id, u16 device_id, pci_device_info_t* out_device);

bool pcnet_initialize(void);
void pcnet_get_status(pcnet_status_t* out_status);
bool pcnet_send(const void* frame, u16 length);
bool pcnet_poll_receive(void* frame_buffer, u16 buffer_capacity, u16* out_length);

bool net_initialize(void);
bool net_configure_ipv4_static(const u8 address[NET_IPV4_ADDRESS_LENGTH],
                               const u8 subnet_mask[NET_IPV4_ADDRESS_LENGTH],
                               const u8 gateway[NET_IPV4_ADDRESS_LENGTH]);
void net_service(void);
bool net_arp_resolve(const u8 ipv4_address[NET_IPV4_ADDRESS_LENGTH],
                     u8 out_mac[NET_MAC_ADDRESS_LENGTH]);
bool net_ipv4_send(u8 protocol,
                   const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                   const void* payload,
                   u16 payload_length);
bool net_poll_ipv4_packet(net_ipv4_packet_t* out_packet);
void net_get_status(net_status_t* out_status);

void net_transport_service(void);
bool net_udp_send(u16 source_port,
                  const u8 destination_ipv4[NET_IPV4_ADDRESS_LENGTH],
                  u16 destination_port,
                  const void* payload,
                  u16 payload_length);
bool net_udp_poll(u16 local_port, net_udp_packet_t* out_packet);
bool net_dhcp_negotiate(void);
void net_dhcp_get_status(net_dhcp_status_t* out_status);
void net_dns_set_server(const u8 dns_server[NET_IPV4_ADDRESS_LENGTH]);
bool net_dns_resolve_a(const char* host, u8 out_ipv4[NET_IPV4_ADDRESS_LENGTH]);
int net_tcp_connect(const u8 remote_ipv4[NET_IPV4_ADDRESS_LENGTH], u16 remote_port);
bool net_tcp_is_connected(int connection_id);
bool net_tcp_send(int connection_id, const void* data, u16 length);
bool net_tcp_wait_for_data(int connection_id, u32 timeout_seconds);
u16 net_tcp_receive(int connection_id, void* buffer, u16 buffer_capacity);
void net_tcp_close(int connection_id);
bool net_http_get(const char* url, char* body, u32 body_capacity, net_http_result_t* out_result);

bool ac97_detect(void);
bool ac97_begin_stream(const u8* audio_data, u32 sample_count, u32 sample_rate);
bool ac97_service(void);
void ac97_end_stream(void);
bool ac97_is_active(void);

bool pe_parse_image(const void* data, u32 size, pe_image_info_t* out_info);
bool pe_launch_image(const void* data, u32 size, pe_launch_result_t* out_result);
bool pe_launch_initrd_app(const void* archive, u32 archive_size, const char* name, pe_launch_result_t* out_result);

bool installer_detect_existing_install(void);
bool installer_find_existing_install(u32* out_lba, u32* out_sector_count);
bool installer_install_to_target(const void* kernel_image,
                                 u32 kernel_size,
                                 const void* initrd_image,
                                 u32 initrd_size,
                                 u32 target_lba,
                                 u32 target_sector_count);
bool installer_install_to_ata0(const void* kernel_image,
                               u32 kernel_size,
                               const void* initrd_image,
                               u32 initrd_size);

#ifdef __cplusplus
}
#endif

#endif
