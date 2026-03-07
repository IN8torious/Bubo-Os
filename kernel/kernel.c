// =============================================================================
// Raven AOS v0.7 — Kernel Main
//
// "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence
//
// Boot sequence:
//   VGA → PMM → IDT → VMM → Framebuffer → PIT → Keyboard → Scheduler
//   → VFS → Usermode → CORVUS Constitution → CORVUS Agents
//   → Desktop Shell → Landon Accessibility Center → Main Loop
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
#include "framebuffer.h"
#include "corvus_display.h"
#include "vfs.h"
#include "initrd.h"
#include "usermode.h"
#include "desktop.h"
#include "corvus_constitution.h"
#include "landon_center.h"
#include "corvus_dashboard.h"
#include "terminal_app.h"
#include <stdint.h>
#include <stdbool.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// ── Draw the Raven OS boot banner (VGA text mode) ─────────────────────────────
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
    terminal_writeline("           Kernel Pluto: Red Cloud  |  Raven AOS v0.7  |  CORVUS MAS");
    terminal_writeline("================================================================================");
    terminal_setcolor(grey);
    terminal_writeline("  MAS = Multi-Agentic Systems  |  Sovereign Intelligence  |  NO MAS DISADVANTAGED");
    terminal_writeline("");
}

// ── Kernel main ───────────────────────────────────────────────────────────────
void kernel_main(uint64_t multiboot_magic, uint64_t multiboot_info_addr) {

    // ── Layer 0: VGA terminal (text mode fallback) ────────────────────────────
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

    // ── Layer 3.5: Framebuffer — VESA pixel display ───────────────────────────
    bool fb_ok = fb_init_from_multiboot((uint32_t)multiboot_info_addr);
    if (!fb_ok) fb_init_fallback();

    // Draw Akatsuki boot splash screen
    corvus_draw_boot_screen();
    corvus_draw_boot_progress(10, "Memory subsystems initialized...");

    // ── Layer 4: PIT Timer (100Hz — drives CORVUS cognitive loop) ────────────
    pit_init(100);
    corvus_draw_boot_progress(25, "PIT timer running at 100Hz...");

    // ── Layer 4: Keyboard (CORVUS perception module) ──────────────────────────
    keyboard_init();
    corvus_draw_boot_progress(35, "Keyboard perception module online...");

    // ── Layer 4: Scheduler ────────────────────────────────────────────────────
    scheduler_init();
    corvus_draw_boot_progress(45, "Scheduler initialized...");

    // ── Layer 3.6: VFS + initrd ───────────────────────────────────────────────
    vfs_init();
    corvus_draw_boot_progress(55, "Virtual filesystem online...");
    usermode_init();
    corvus_draw_boot_progress(60, "Ring 3 user mode ready...");

    // ── Layer 5: CORVUS Constitutional Governance ─────────────────────────────
    corvus_constitution_init();
    corvus_draw_boot_progress(65, "Constitutional governance: NO MAS DISADVANTAGED");

    // ── Layer 5: CORVUS Orchestration Engine ──────────────────────────────────
    corvus_init();
    corvus_draw_boot_progress(75, "CORVUS MAS — 10 agents activated...");

    // ── Layer 6: Desktop Shell ────────────────────────────────────────────────
    desktop_init();
    corvus_draw_boot_progress(82, "Desktop shell initialized...");

    // ── Layer 6: App Subsystems ───────────────────────────────────────────────
    terminal_app_init();
    corvus_dashboard_init();
    landon_center_init();
    corvus_draw_boot_progress(90, "Applications ready — Landon's center online...");

    // ── Boot complete ─────────────────────────────────────────────────────────
    terminal_setcolor(red);
    terminal_writeline("  ════════════════════════════════════════════════════════════════");
    terminal_setcolor(green);
    terminal_writeline("  Raven AOS v0.7 boot complete — NO MAS DISADVANTAGED");
    terminal_setcolor(red);
    terminal_writeline("  ════════════════════════════════════════════════════════════════");

    corvus_draw_boot_progress(100, "Boot complete. Launching Raven Desktop...");

    // Print the constitution
    corvus_print_constitution();

    // Brief pause so user can see the boot screen
    for (volatile int i = 0; i < 80000000; i++) {}

    // ── Layer 7: Launch the Desktop Shell ────────────────────────────────────
    // Open Landon's accessibility center by default
    desktop_launch_app(0x07);  // Landon's Voice Control Center
    desktop_launch_app(0x04);  // CORVUS Dashboard

    // Initial render
    desktop_render();

    // ── Layer 7: CORVUS Shell (text fallback) ─────────────────────────────────
    shell_init();

    // ── Main kernel loop ──────────────────────────────────────────────────────
    // The desktop renders on every PIT tick.
    // Keyboard events are routed to the focused window.
    // CORVUS agents run in the scheduler.
    // Landon's voice commands are processed here.
    while (1) {
        // Tick all subsystems
        landon_center_tick();

        // Re-render the desktop
        desktop_render();

        // Halt until next interrupt (PIT fires at 100Hz)
        __asm__ volatile("hlt");
    }
}
