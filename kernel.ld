/* =============================================================================
 * BUBO OS — Kernel Linker Script
 * Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
 * Built for Landon Pankuch. Built for everyone who was told they couldn't.
 *
 * Memory Layout:
 *   0x00000000 - 0x000FFFFF  (1MB)   : Reserved (BIOS, VGA, etc.)
 *   0x00100000 - 0x7FFFFFFF  (2GB-1M): BUBO OS kernel + agents + stack
 *   0x80000000 - 0x27FFFFFFF (8GB)   : Windows VM guest memory (VMX)
 *
 * The kernel loads at 1MB — the standard for x86 bare-metal kernels.
 * Everything BUBO needs lives in the first 2GB. Windows gets the rest.
 * ============================================================================= */

ENTRY(_start)

SECTIONS {
    /* Load at 1MB — standard for x86 kernels */
    . = 1M;

    /* ── Multiboot2 Header ─────────────────────────────────────────────────── */
    /* Must be within the first 32KB of the kernel image */
    .multiboot2 : {
        KEEP(*(.multiboot2))
    }

    /* ── Kernel Text (Code) ────────────────────────────────────────────────── */
    .text ALIGN(4K) : {
        *(.text*)
    }

    /* ── Read-Only Data ────────────────────────────────────────────────────── */
    /* Color definitions, constitutional rules, agent identity strings */
    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    /* ── Initialized Data ──────────────────────────────────────────────────── */
    /* Agent state tables, calibration matrices, FCN weights */
    .data ALIGN(4K) : {
        *(.data*)
    }

    /* ── BUBO Agent Data ───────────────────────────────────────────────────── */
    /* Dedicated section for agent state — makes debugging easier */
    .agents ALIGN(4K) : {
        PROVIDE(__agents_start = .);
        *(.agents*)
        PROVIDE(__agents_end = .);
    }

    /* ── Akashic Records ───────────────────────────────────────────────────── */
    /* ARCHIVIST's static record table — sealed truths live here */
    .akashic ALIGN(4K) : {
        PROVIDE(__akashic_start = .);
        *(.akashic*)
        PROVIDE(__akashic_end = .);
    }

    /* ── FCN Neural Network Weights ────────────────────────────────────────── */
    /* Initial weights baked into the kernel — updated by Hyperbolic Chamber */
    .fcn_weights ALIGN(4K) : {
        PROVIDE(__fcn_weights_start = .);
        *(.fcn_weights*)
        PROVIDE(__fcn_weights_end = .);
    }

    /* ── VMX Regions ───────────────────────────────────────────────────────── */
    /* VMXON region and VMCS must be 4KB aligned — linker enforces this */
    .vmx ALIGN(4K) : {
        PROVIDE(__vmx_start = .);
        *(.vmx*)
        PROVIDE(__vmx_end = .);
    }

    /* ── BSS (Uninitialized Data — Zero Filled) ────────────────────────────── */
    /* Replay buffer, EPT tables, IOMMU tables, stack */
    .bss ALIGN(4K) : {
        PROVIDE(__bss_start = .);
        *(COMMON)
        *(.bss*)
        PROVIDE(__bss_end = .);
    }

    /* ── Kernel Stack ──────────────────────────────────────────────────────── */
    /* 64KB stack at the top of the kernel image */
    .stack ALIGN(4K) : {
        PROVIDE(__stack_bottom = .);
        . += 64K;
        PROVIDE(__stack_top = .);
    }

    /* ── End of Kernel ─────────────────────────────────────────────────────── */
    PROVIDE(__kernel_end = .);

    /* ── Discard Unnecessary Sections ─────────────────────────────────────── */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
        *(.gcc_except_table)
    }
}
