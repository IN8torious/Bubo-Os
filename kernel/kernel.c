// BUBO OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

// =============================================================================
// BUBO OS v1.0 — Kernel Main
//
// Boot sequence:
//   VGA → PMM → IDT → VMM → Framebuffer → PIT → Keyboard → Scheduler
//   → VFS → User Mode → Sound → Voice → Network → TCP/IP → LLM
//   → CORVUS Constitution → CORVUS Agents → Desktop → Apps → Main Loop
//
// Constitutional Mandate: NO MAS DISADVANTAGED
// MAS = Multi-Agentic Systems — Sovereign Intelligence
//
// Landon Pankuch — cerebral palsy — speaks, CORVUS acts.
// Sinamon Red Dodge Demon 170 — 1,400 HP — his name on it.
// =============================================================================

#include "dedication.h"
#include "safety_flask.h"
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
#include "bubo_display.h"
#include "vfs.h"
#include "initrd.h"
#include "usermode.h"
#include "desktop.h"
#include "corvus_constitution.h"
#include "landon_center.h"
#include "bubo_dashboard.h"
#include "terminal_app.h"
#include "sound.h"
#include "voice.h"
#include "net.h"
#include "tcpip.h"
#include "llm.h"
#include <stdint.h>
#include <stdbool.h>
#include <kernel_ext.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// ── Voice → LLM → CORVUS integration callback ────────────────────────────────
// This is the central integration point for v1.0:
//   Voice recognition detects a command
//   → LLM enriches/validates the intent
//   → CORVUS agents execute
//   → Desktop updates
static void on_voice_command(voice_cmd_t cmd, const char* response) {
    terminal_write("[BUBO] ");
    terminal_write(response);
    terminal_write("\n");

    // Route constitutional commands directly
    if (cmd == VCMD_MANDATE) {
        bubo_print_constitution();
    } else if (cmd == VCMD_UNKNOWN || cmd == VCMD_CORVUS) {
        // Natural language — route through LLM for full understanding
        char llm_out[512];
        if (llm_infer(response, llm_out, sizeof(llm_out))) {
            terminal_write("[LLM] ");
            terminal_write(llm_out);
            terminal_write("\n");
        }
    }
    // All commands are also logged to CORVUS agent system via corvus_tick()
    (void)cmd;

    // Re-render desktop to show updated state
    desktop_render();
}

// ── Boot banner ───────────────────────────────────────────────────────────────
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
    terminal_writeline("           Kernel Pluto: Red Cloud  |  BUBO OS v1.0  |  BUBO MAS");
    terminal_writeline("================================================================================");
    terminal_setcolor(grey);
    terminal_writeline("  MAS = Multi-Agentic Systems  |  Sovereign Intelligence  |  NO MAS DISADVANTAGED");
    terminal_writeline("  Landon Pankuch — Sinamon Red Dodge Demon 170 — 1,400 HP — CORVUS drives for him");
    terminal_writeline("");
}

// ── Kernel main ───────────────────────────────────────────────────────────────
void kernel_main(uint64_t multiboot_magic, uint64_t multiboot_info_addr) {

    // ── Layer 0: VGA terminal ─────────────────────────────────────────────────
    terminal_init();
    draw_banner();

    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,   VGA_BLACK);
    (void)white; (void)green; (void)red;

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

    // ── Layer 3: IDT ──────────────────────────────────────────────────────────
    idt_init();

    // ── Layer 3: VMM + kmalloc heap ───────────────────────────────────────────
    vmm_init();

    // ── Layer 3.5: Healer — self-healing layer (init before framebuffer) ──────
    healer_init((uint32_t)multiboot_info_addr);
    healer_boot_subsystems();
    // ── Layer 3.5: Framebuffer (healer manages it) ────────────────────────────
    bool fb_ok = fb_init_from_multiboot((uint32_t)multiboot_info_addr);
    if (!fb_ok) fb_init_fallback();

    bubo_draw_boot_screen();
    bubo_draw_boot_progress(5, "Memory subsystems initialized...");

    // ── Layer 4: PIT Timer (100Hz) ────────────────────────────────────────────
    pit_init(100);
    bubo_draw_boot_progress(12, "PIT timer running at 100Hz...");

    // ── Layer 4: Keyboard ─────────────────────────────────────────────────────
    keyboard_init();
    bubo_draw_boot_progress(18, "Keyboard perception module online...");

    // ── RAVEN meta-debugger (needs PIT + keyboard) ────────────────────────────
    raven_init();
    bubo_draw_boot_progress(20, "RAVEN online — Ctrl+F12 for debug dump, F12 for Kami...");

    // ── Admin Panel / Kami (needs RAVEN) ──────────────────────────────────────
    admin_panel_init();
    bubo_draw_boot_progress(21, "Kami admin panel ready — press F12 anytime...");

    // ── Layer 4: Scheduler ────────────────────────────────────────────────────
    scheduler_init();
    bubo_draw_boot_progress(24, "Scheduler initialized...");

    // ── Layer 5: VFS + initrd ─────────────────────────────────────────────────
    vfs_init();
    bubo_draw_boot_progress(30, "Virtual filesystem online...");

    // ── Layer 5.5: User Mode (Ring 3) ─────────────────────────────────────────
    usermode_init();
    bubo_draw_boot_progress(36, "Ring 3 user mode ready — apps isolated from kernel...");

    // ── Layer 6: Sound Driver ─────────────────────────────────────────────────
    bool sound_ok = sound_init();
    bubo_draw_boot_progress(42, sound_ok ?
        "Audio driver online — microphone ready..." :
        "Audio: PC speaker fallback active...");

    // ── Layer 6: Voice Recognition ────────────────────────────────────────────
    voice_init();
    voice_set_callback(on_voice_command);
    bubo_draw_boot_progress(48, "Voice recognition engine ready — BUBO is listening...");

    // ── Layer 6: Network ──────────────────────────────────────────────────────
    bool net_ok = net_init();
    if (net_ok) tcpip_init();
    bubo_draw_boot_progress(54, net_ok ?
        "Network stack online — CORVUS can reach the world..." :
        "Network: no NIC detected — offline mode...");

    // ── Layer 6: BUBO LLM Engine ────────────────────────────────────────────
    llm_init();
    bubo_draw_boot_progress(62,
        "BUBO reasoning engine ready — constitutional fast-path active...");

    // ── Layer 5: BUBO Constitutional Governance ─────────────────────────────
    corvus_constitution_init();
    bubo_draw_boot_progress(68,
        "Constitutional governance: NO MAS DISADVANTAGED — hardcoded in kernel");

    // ── Layer 5: BUBO Orchestration Engine ──────────────────────────────────
    corvus_init();
    bubo_draw_boot_progress(76, "BUBO MAS — 10 sovereign agents activated...");

    // ── Layer 7: Desktop Shell ────────────────────────────────────────────────
    desktop_init();
    bubo_draw_boot_progress(82, "Desktop shell initialized — Akatsuki theme loaded...");

    // ── Layer 7: App Subsystems ───────────────────────────────────────────────
    terminal_app_init();
    bubo_dashboard_init();
    landon_center_init();
    bubo_draw_boot_progress(90, "Applications ready — Accessibility Hub online...");

    // ── Boot complete ─────────────────────────────────────────────────────────
    terminal_setcolor(red);
    terminal_writeline("  ════════════════════════════════════════════════════════════════════════════");
    terminal_setcolor(green);
    terminal_writeline("  BUBO OS — Boot complete — NO MAS DISADVANTAGED");
    terminal_setcolor(red);
    terminal_writeline("  ════════════════════════════════════════════════════════════════════════════");
    bubo_draw_boot_progress(100, "Boot complete. Launching BUBO Desktop...");

    bubo_print_constitution();

    // Boot audio
    if (sound_ok) sound_corvus_ready();

    for (volatile int i = 0; i < 80000000; i++) {}

    // ── Launch Desktop ────────────────────────────────────────────────────────
    desktop_launch_app(0x07);   // Landon's Voice Control Center — always first
    desktop_launch_app(0x04);   // BUBO Dashboard
    desktop_render();

    // ── BUBO Shell (text fallback) ──────────────────────────────────────────
    shell_init();

    // ── Start voice listening ─────────────────────────────────────────────────
    voice_start_listening();

    // ── Main kernel loop — 100Hz ──────────────────────────────────────────────
    uint32_t tick = 0;
    while (1) {
        tick++;

        // Voice pipeline — reads mic, runs VAD, fires on_voice_command()
        voice_tick();

        // Network receive — process incoming packets
        if (net_ok) {
            uint8_t pkt_buf[1500];
            uint16_t pkt_len = net_receive(pkt_buf, sizeof(pkt_buf));
            if (pkt_len > 0) tcpip_process_packet(pkt_buf, pkt_len);
        }

        // CORVUS agents — every 10 ticks (100ms)
        if (tick % 10 == 0) corvus_tick();

        // RAVEN watchdog — every 100 ticks (1 second)
        if (tick % 100 == 0) raven_watchdog_tick();

        // Healer watchdog — every 200 ticks (2 seconds)
        if (tick % 200 == 0) healer_watchdog_tick();

        // Landon center state update
        landon_center_tick();

        // Desktop render — every tick
        desktop_render();

        // Halt until next PIT interrupt
        __asm__ volatile("hlt");
    }
}
