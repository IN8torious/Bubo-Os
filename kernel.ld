/* Instinct OS — Kernel Linker Script */
/* 64-bit kernel: loads at 1MB physical, identity mapped */

ENTRY(_start)

SECTIONS {
    /* Load at 1MB — standard for x86 kernels */
    . = 1M;

    /* Multiboot2 header must be within first 32KB */
    .multiboot2 : {
        KEEP(*(.multiboot2))
    }

    /* Kernel text (code) */
    .text ALIGN(4K) : {
        *(.text*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    /* Initialized data */
    .data ALIGN(4K) : {
        *(.data*)
    }

    /* Uninitialized data (zero-filled) */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss*)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}
