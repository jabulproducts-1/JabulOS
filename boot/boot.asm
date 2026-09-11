%define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
%define MULTIBOOT2_ARCHITECTURE_I386 0
%define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289
%define CR4_PAE (1 << 5)
%define CR0_PG  (1 << 31)
%define EFER_MSR 0xC0000080
%define EFER_LME (1 << 8)
%define EARLY_PD_COUNT 512
%define EARLY_TOTAL_PAGES (2 + EARLY_PD_COUNT)
%define EARLY_2M_PAGES (EARLY_PD_COUNT * 512)

section .multiboot
align 8
multiboot_header_start:
    dd MULTIBOOT2_HEADER_MAGIC
    dd MULTIBOOT2_ARCHITECTURE_I386
    dd multiboot_header_end - multiboot_header_start
    dd -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header_start))

align 8
    dw 5
    dw 0
    dd 20
    dd 1024 ; Width
    dd 768  ; Height
    dd 32   ; BPP

align 8
    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .data
align 8
mb_info_ptr: dq 0

gdt64:
    dq 0x0000000000000000
    dq 0x00209A0000000000
    dq 0x0000920000000000
.gdtr:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32
global start
extern kernel_main

start:
    cli
    cmp eax, MULTIBOOT2_BOOTLOADER_MAGIC
    jne boot_failure

    mov [mb_info_ptr], ebx
    mov dword [mb_info_ptr + 4], 0

    call check_long_mode
    jc boot_failure

    lea esp, [stack_top]
    call setup_page_tables

    lgdt [gdt64.gdtr]

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    mov ecx, EFER_MSR
    rdmsr
    or eax, EFER_LME
    wrmsr

    mov eax, page_table_l4
    mov cr3, eax

    mov eax, cr0
    or eax, CR0_PG
    mov cr0, eax

    jmp 0x08:long_mode_entry

boot_failure:
    cli
.hang:
    hlt
    jmp .hang

check_long_mode:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    test eax, 1 << 21
    jz .fail

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .fail

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .fail
    clc
    ret
.fail:
    stc
    ret

setup_page_tables:
    mov edi, page_table_l4
    mov ecx, (4096 * EARLY_TOTAL_PAGES) / 4
    xor eax, eax
    rep stosd

    mov eax, page_table_l3
    or eax, 0x003
    mov [page_table_l4], eax
    mov dword [page_table_l4 + 4], 0

    xor ecx, ecx
.pdpt_loop:
    mov eax, page_table_l2_tables
    mov edx, 0
    mov ebx, ecx
    shl ebx, 12
    add eax, ebx
    adc edx, 0
    or eax, 0x003
    mov [page_table_l3 + ecx * 8], eax
    mov [page_table_l3 + ecx * 8 + 4], edx
    inc ecx
    cmp ecx, EARLY_PD_COUNT
    jne .pdpt_loop

    xor ecx, ecx
.map_loop:
    mov eax, ecx
    xor edx, edx
    shld edx, eax, 21
    shl eax, 21
    or eax, 0x083
    mov [page_table_l2_tables + ecx * 8], eax
    mov [page_table_l2_tables + ecx * 8 + 4], edx
    inc ecx
    cmp ecx, EARLY_2M_PAGES
    jne .map_loop
    ret

bits 64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    lea rsp, [stack_top]

    mov rdi, [mb_info_ptr]
    mov rsi, MULTIBOOT2_BOOTLOADER_MAGIC
    call kernel_main

.done:
    cli
    hlt
    jmp .done

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2_tables:
    resb (4096 * EARLY_PD_COUNT)

section .note.GNU-stack noalloc noexec nowrite progbits
