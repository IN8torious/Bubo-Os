// =============================================================================
// Raven OS — Kernel Main
// Boots the full 7-layer Agentic OS stack:
//   VGA → PMM → IDT → VMM → PIT → Keyboard → Scheduler → CORVUS → Shell
// =============================================================================

#include "vga.h"
#include "pmm.h"
#include "idt.h"
#include "vmm.h"
#include "pit.h"
#include "keyboard.h"
#include "scheduler.h"
#include "corvus.h"
#include "shell.h"
#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// ── Draw the Raven OS boot banner ─────────────────────────────────────────────
static void draw_banner(void) {
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,  VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY,   VGA_BLACK);

    terminal_setcolor(red);
    terminal_writeline("================================================================================");
    terminal_setcolor(white);
    terminal_writeline("        ██████╗  █████╗ ██╗   ██╗███████╗███╗   ██╗     ██████╗ ███████╗");
    terminal_writeline("        ██╔══██╗██╔══██╗██║   ██║██╔════╝████╗  ██║    ██╔═══██╗██╔════╝");
    terminal_writeline("        ██████╔╝███████║██║   ██║█████╗  ██╔██╗ ██║    ██║   ██║███████╗");
    terminal_writeline("        ██╔══██╗██╔══██║╚██╗ ██╔╝██╔══╝  ██║╚██╗██║    ██║   ██║╚════██║");
    terminal_writeline("        ██║  ██║██║  ██║ ╚████╔╝ ███████╗██║ ╚████║    ╚██████╔╝███████║");
    terminal_writeline("        ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝     ╚═════╝ ╚══════╝");
    terminal_setcolor(red);
    terminal_writeline("                     Kernel Pluto: Red Cloud  |  CORVUS AOS v0.3 (x86-64)");
    terminal_writeline("================================================================================");
    terminal_setcolor(grey);
    terminal_writeline("  7-Layer Agentic OS  |  10 Kernel Agents  |  Constitutional Governance");
    terminal_writeline("");
}

// ── Kernel main ───────────────────────────────────────────────────────────────
// In 64-bit: rdi = multiboot2 magic, rsi = multiboot2 info pointer
void kernel_main(uint64_t multiboot_magic, uint64_t multiboot_info_addr) {

    // ── Layer 0: VGA terminal ─────────────────────────────────────────────────
    terminal_init();
    draw_banner();

    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,   VGA_BLACK);
    (void)white; (void)green; (void)red;

    // Verify multiboot2
    terminal_setcolor(white);
    terminal_write("  [ BOOT ] Multiboot2: ");
    if (multiboot_magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        terminal_setcolor(green);
        terminal_writeline("VALID  ✓");
    } else {
        terminal_setcolor(red);
        terminal_writeline("INVALID — continuing anyway");
    }

    // ── Layer 2: Physical Memory Manager ─────────────────────────────────────
    pmm_init((uint32_t)multiboot_info_addr);

    // ── Layer 3: Interrupt Descriptor Table ───────────────────────────────────
    idt_init();

    // ── Layer 3: Virtual Memory Manager + kmalloc heap ────────────────────────
    vmm_init();

    // ── Layer 4: PIT Timer (100Hz — drives CORVUS cognitive loop) ────────────
    pit_init(100);

    // ── Layer 4: Keyboard (CORVUS perception module) ──────────────────────────
    keyboard_init();

    // ── Layer 4: Scheduler ────────────────────────────────────────────────────
    scheduler_init();

    // ── Layer 5: CORVUS Orchestration Engine ──────────────────────────────────
    corvus_init();

    // ── Boot complete ─────────────────────────────────────────────────────────
    terminal_setcolor(red);
    terminal_writeline("  ════════════════════════════════════════════════════════════════");
    terminal_setcolor(green);
    terminal_writeline("  Raven AOS boot sequence complete — all systems operational");
    terminal_setcolor(red);
    terminal_writeline("  ════════════════════════════════════════════════════════════════");

    // ── Layer 7: CORVUS Shell ─────────────────────────────────────────────────
    shell_init();

    // ── Main kernel loop — wait for interrupts ────────────────────────────────
    // The PIT drives CORVUS, the keyboard drives the shell
    // Everything is interrupt-driven from here
    while (1) {
        __asm__ volatile("hlt");
    }
}
