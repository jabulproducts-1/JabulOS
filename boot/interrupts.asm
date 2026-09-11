section .text
bits 64

global isr_stub_0
global isr_stub_table
extern isr_default_handler

%macro ISR_NOERR 1
isr_stub_%1:
    push 0
    push %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
isr_stub_%1:
    push %1
    jmp isr_common_stub
%endmacro

%assign i 0
%rep 256
    %if i = 8 || i = 10 || i = 11 || i = 12 || i = 13 || i = 14 || i = 17 || i = 21 || i = 29 || i = 30
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
    %assign i i + 1
%endrep

isr_common_stub:
    cld
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call isr_default_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16
    iretq

section .rodata
align 8
isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
    %assign i i + 1
%endrep

section .note.GNU-stack noalloc noexec nowrite progbits
