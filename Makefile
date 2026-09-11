CROSS ?= x86_64-linux-gnu
CC := $(CROSS)-gcc
CXX := $(CROSS)-g++
LD := $(CROSS)-ld
AS := nasm
PYTHON ?= python3
QEMU ?= qemu-system-x86_64
QEMU_IMG ?= qemu-img
QEMU_MEMORY ?= 512M
AUDIO_PREPROCESS_RATE ?= 16000
# Default audio backend for WSL/Linux QEMU.
# If audio causes issues, you can disable it:
#   make run QEMU_AUDIO_ARGS=
QEMU_AUDIO_ARGS ?= -audiodev pa,id=audio0 -device sb16,audiodev=audio0

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
ISO_DIR := $(BUILD_DIR)/iso
INITRD_DIR := $(BUILD_DIR)/initrd
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO_IMAGE := JABULOS30K.iso
DISK_IMAGE := $(BUILD_DIR)/disk.img
UPDATE_INBOX_DIR := updates/inbox
UPDATE_ARCHIVE_DIR := updates/archive
UPDATE_SERVICE_STATE := $(BUILD_DIR)/update-service/state.json

CFLAGS := -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -m64 -mno-red-zone -mcmodel=kernel -mgeneral-regs-only -O2 -Wall -Wextra -Werror -Iinclude -I$(BUILD_DIR)/generated
CXXFLAGS := -std=gnu++17 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -fno-stack-protector -fno-pic -fno-pie -fno-builtin -m64 -mno-red-zone -mcmodel=kernel -mgeneral-regs-only -O2 -Wall -Wextra -Werror -Iinclude -I$(BUILD_DIR)/generated
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T boot/linker.ld
ASMFLAGS := -f elf64
GENERATED_FONT_HEADER := $(BUILD_DIR)/generated/ui_font.h
BIOS_BOOT_IMG := $(BUILD_DIR)/bios.img
UEFI_BOOT_EFI := $(BUILD_DIR)/BOOTX64.EFI
UEFI_BOOT_IMG := $(BUILD_DIR)/efiboot.img
GRUB_FONT := /usr/share/grub/unicode.pf2
GRUB_BIOS_MODULE_DIR := /usr/lib/grub/i386-pc
GRUB_UEFI_MODULE_DIR := /usr/lib/grub/x86_64-efi

C_SOURCES := \
    kernel/kernel.c \
    kernel/gdt.c \
    kernel/idt.c \
    kernel/serial.c \
    kernel/pmm.c \
    kernel/pci.c \
    kernel/xhci.c \
    kernel/pcnet.c \
    kernel/net.c \
    kernel/transport.c \
    kernel/vga.c \
    kernel/initrd.c \
    kernel/timer.c \
    kernel/rtc.c \
    kernel/ata.c \
    kernel/ahci.c \
    kernel/nvme.c \
    kernel/storage.c \
    kernel/pe.c \
    kernel/installer.c \
    lib/string.c

CPP_SOURCES := \
    kernel/gui.cpp \
    kernel/image.cpp \
    kernel/display_driver.cpp \
    kernel/mouse_driver.cpp

ASM_SOURCES := \
    boot/boot.asm \
    boot/interrupts.asm

C_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(C_SOURCES))
CPP_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.cpp.o,$(CPP_SOURCES))
ASM_OBJECTS := $(patsubst %.asm,$(OBJ_DIR)/%.asm.o,$(ASM_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS)

.PHONY: all clean initrd iso run run-live update-service stage-update rebuild-iso-wsl

all: iso

$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASMFLAGS) $< -o $@

$(OBJ_DIR)/kernel/vga.c.o: $(GENERATED_FONT_HEADER)

$(GENERATED_FONT_HEADER): tools/generate_font.py
	@mkdir -p $(dir $@)
	$(PYTHON) $< $@

$(BIOS_BOOT_IMG): grub/bios-bootstrap.cfg
	@mkdir -p $(dir $@)
	grub-mkimage -O i386-pc-eltorito -o $@ -p /boot/grub -c $< \
		biosdisk iso9660 normal configfile search search_fs_file multiboot2 font gfxterm all_video

$(UEFI_BOOT_EFI): grub/efi-bootstrap.cfg
	@mkdir -p $(dir $@)
	grub-mkstandalone -O x86_64-efi -o $@ --locales="" --fonts="unicode" \
		--modules="part_gpt part_msdos fat iso9660 normal configfile search search_fs_file multiboot2 font gfxterm all_video" \
		"boot/grub/grub.cfg=$<"

$(UEFI_BOOT_IMG): $(UEFI_BOOT_EFI)
	@mkdir -p $(dir $@)
	rm -f $@
	dd if=/dev/zero of=$@ bs=1M count=16
	mformat -i $@ -F ::
	mmd -i $@ ::/EFI ::/EFI/BOOT
	mcopy -i $@ $(UEFI_BOOT_EFI) ::/EFI/BOOT/BOOTX64.EFI

$(DISK_IMAGE):
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f $@ ]; then $(QEMU_IMG) create -f raw $@ 1G; fi

$(KERNEL_ELF): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

initrd:
	@mkdir -p $(INITRD_DIR)
	rm -rf $(INITRD_DIR)/*
	$(PYTHON) tools/generate_wallpaper.py $(INITRD_DIR)/wallpaper.bmp assets/themes/AlanBliss.png
	$(PYTHON) tools/generate_wallpaper.py $(INITRD_DIR)/boot-splash.bmp assets/boot-splash.jpg contain
	$(PYTHON) tools/generate_wallpaper.py $(INITRD_DIR)/JabulSplash.bmp assets/JabulSplash.png contain 1920 1080
	$(PYTHON) tools/generate_pe_sample.py $(INITRD_DIR)/pe-lab.exe
	@if [ -d assets/themes ]; then mkdir -p $(INITRD_DIR)/themes; for theme in assets/themes/*.png; do [ -f "$$theme" ] || continue; output="$(INITRD_DIR)/themes/$$(basename "$${theme%.png}").bmp"; $(PYTHON) tools/generate_wallpaper.py "$$output" "$$theme"; done; fi
	@if [ -d icons ]; then mkdir -p $(INITRD_DIR)/icons; for icon in icons/*; do [ -f "$$icon" ] || continue; output="$(INITRD_DIR)/icons/$$(basename "$${icon%.*}").bmp"; $(PYTHON) tools/generate_icon.py "$$output" "$$icon"; done; fi
	@if [ -d photos ]; then cp -r photos $(INITRD_DIR)/; fi
	@if [ -d Documents ]; then cp -r Documents $(INITRD_DIR)/; fi
	@if [ -f assets/update.html ]; then cp assets/update.html $(INITRD_DIR)/; fi
	@if [ -f assets/lockscreen/background.jpg ]; then $(PYTHON) tools/generate_wallpaper.py $(INITRD_DIR)/background.bmp assets/lockscreen/background.jpg cover 1024 768; fi
	@if [ -f assets/lockscreen/profile.png ]; then $(PYTHON) tools/generate_wallpaper.py $(INITRD_DIR)/profile.bmp assets/lockscreen/profile.png cover 120 120; fi
	find assets -type f \( -iname '*.mp3' -o -iname '*.wav' -o -iname '*.flac' -o -iname '*.aac' -o -iname '*.ogg' -o -iname '*.m4a' \) -exec cp --parents -f {} $(INITRD_DIR)/ \;
	find assets -type f \( -iname '*.mp3' -o -iname '*.wav' -o -iname '*.flac' -o -iname '*.aac' -o -iname '*.ogg' -o -iname '*.m4a' \) -exec $(PYTHON) tools/preprocess_audio.py {} . $(INITRD_DIR) --sample-rate $(AUDIO_PREPROCESS_RATE) \;
	cd $(INITRD_DIR) && tar --format=ustar -cf ../initrd.tar *

iso: $(KERNEL_ELF) initrd $(BIOS_BOOT_IMG) $(UEFI_BOOT_EFI) $(UEFI_BOOT_IMG)
	rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	@mkdir -p $(ISO_DIR)/boot/grub/fonts
	@mkdir -p $(ISO_DIR)/boot/grub/i386-pc
	@mkdir -p $(ISO_DIR)/boot/grub/x86_64-efi
	@mkdir -p $(ISO_DIR)/boot/initrd
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	cp $(BIOS_BOOT_IMG) $(ISO_DIR)/boot/grub/bios.img
	cp $(GRUB_FONT) $(ISO_DIR)/boot/grub/fonts/unicode.pf2
	cp $(GRUB_BIOS_MODULE_DIR)/*.mod $(ISO_DIR)/boot/grub/i386-pc/
	cp $(GRUB_UEFI_MODULE_DIR)/*.mod $(ISO_DIR)/boot/grub/x86_64-efi/
	cp $(BUILD_DIR)/initrd.tar $(ISO_DIR)/boot/initrd/initrd.tar
	cp $(UEFI_BOOT_EFI) $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	xorriso -as mkisofs \
		-r -J -joliet-long -V "JABULOS_OSP" \
		-c boot.catalog \
		-b boot/grub/bios.img \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-eltorito-alt-boot \
		-append_partition 2 0xef $(UEFI_BOOT_IMG) \
		-appended_part_as_gpt \
		-e --interval:appended_partition_2:all:: \
		-no-emul-boot \
		-o $(ISO_IMAGE) \
		$(ISO_DIR)
	xorriso -indev $(ISO_IMAGE) -report_el_torito plain 2>/dev/null | grep -q "UEFI"

run: iso $(DISK_IMAGE)
	$(QEMU) $(QEMU_AUDIO_ARGS) -cdrom $(ISO_IMAGE) -drive file=$(DISK_IMAGE),format=raw,index=0,media=disk -m $(QEMU_MEMORY)

run-debug: iso $(DISK_IMAGE)
	$(QEMU) $(QEMU_AUDIO_ARGS) -cdrom $(ISO_IMAGE) -drive file=$(DISK_IMAGE),format=raw,index=0,media=disk -m $(QEMU_MEMORY) -serial file:debug-384f43.log

run-live: run

update-service: $(DISK_IMAGE)
	@mkdir -p $(UPDATE_INBOX_DIR) $(UPDATE_ARCHIVE_DIR) $(dir $(UPDATE_SERVICE_STATE))
	$(PYTHON) tools/update_service.py watch --disk $(DISK_IMAGE) --inbox $(UPDATE_INBOX_DIR) --archive $(UPDATE_ARCHIVE_DIR) --state $(UPDATE_SERVICE_STATE) --skip-rebuild

stage-update: $(DISK_IMAGE)
	@test -n "$(ISO)" || (echo "Usage: make stage-update ISO=/path/to/update.iso" && false)
	@mkdir -p $(UPDATE_ARCHIVE_DIR) $(dir $(UPDATE_SERVICE_STATE))
	$(PYTHON) tools/update_service.py stage --disk $(DISK_IMAGE) --iso $(ISO) --archive $(UPDATE_ARCHIVE_DIR) --state $(UPDATE_SERVICE_STATE) --skip-rebuild

rebuild-iso-wsl:
	./build_iso.sh

clean:
	rm -rf $(BUILD_DIR) $(ISO_IMAGE)
