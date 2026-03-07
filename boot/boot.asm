; =============================================================================
; Raven OS — Boot Entry Point
; Multiboot2 compliant bootloader stub
; Hands control to kernel_main() in C
; =============================================================================

bits 32

; Multiboot2 header constants
MULTIBOOT2_MAGIC        equ 0xE85250D6
MULTIBOOT2_ARCH         equ 0           ; i386 protected mode
MULTIBOOT2_HEADER_LEN   equ (multiboot2_header_end - multiboot2_header_start)
MULTIBOOT2_CHECKSUM     equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LEN)

; Stack size: 16KB
STACK_SIZE equ 16384

section .multiboot2
align 8
multiboot2_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_HEADER_LEN
    dd MULTIBOOT2_CHECKSUM

    ; Framebuffer tag — request 1024x768x32
    align 8
    dw 5            ; type: framebuffer
    dw 0            ; flags
    dd 20           ; size
    dd 1024         ; width
    dd 768          ; height
    dd 32           ; depth

    ; End tag
    align 8
    dw 0            ; type: end
    dw 0            ; flags
    dd 8            ; size
multiboot2_header_end:

section .bss
align 16
stack_bottom:
    resb STACK_SIZE
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Set up the stack
    mov esp, stack_top

    ; Push multiboot2 info pointer and magic for kernel_main
    push ebx        ; multiboot info struct pointer
    push eax        ; multiboot magic (should be 0x36d76289)

    ; Clear EFLAGS
    push 0
    popf

    ; Call the C kernel
    call kernel_main

    ; If kernel_main returns, halt forever
.halt:
    cli
    hlt
    jmp .halt
