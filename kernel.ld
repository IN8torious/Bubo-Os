/* Raven OS — Kernel Linker Script */
/* Links the multiboot2 header, kernel code, and data into a flat binary */

ENTRY(_start)

SECTIONS {
    /* Load at 1MB — standard for x86 kernels */
    . = 1M;

    /* Multiboot2 header must be within first 32KB */
    .multiboot2 : {
        KEEP(*(.multiboot2))
    }

    /* Kernel text (code) */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.text)
    }

    /* Read-only data */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* Initialized data */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* Uninitialized data (zero-filled) */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}
