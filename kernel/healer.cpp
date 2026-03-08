// =============================================================================
// BUBO OS — Self-Healing Layer Implementation
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
// =============================================================================

#include <new>           // freestanding placement new (bare-metal)
#include "healer.hpp"
#include "vga.h"
#include "framebuffer.h"

// ── Global healer instance ────────────────────────────────────────────────────
HealerCore g_healer;

// ── Framebuffer Subsystem ─────────────────────────────────────────────────────

// ── Static subsystem instances (no heap — bare metal) ────────────────────────
// Placed in BSS — initialized to zero, then constructed manually
static uint8_t fb_sub_storage[sizeof(FramebufferSubsystem)];
static uint8_t snd_sub_storage[sizeof(SoundSubsystem)];
static uint8_t net_sub_storage[sizeof(NetworkSubsystem)];

FramebufferSubsystem* g_fb_subsystem  = nullptr;
SoundSubsystem*       g_snd_subsystem = nullptr;
NetworkSubsystem*     g_net_subsystem = nullptr;

// ── healer_init — call from kernel_main before any subsystem init ─────────────
extern "C" void healer_init(uint32_t mb2_info_addr) {
    // Placement new — no heap required
    g_fb_subsystem  = new(fb_sub_storage)  FramebufferSubsystem();
    g_snd_subsystem = new(snd_sub_storage) SoundSubsystem();
    g_net_subsystem = new(net_sub_storage) NetworkSubsystem();

    g_fb_subsystem->mb2_info_addr = mb2_info_addr;

    g_healer.register_subsystem(g_fb_subsystem);
    g_healer.register_subsystem(g_snd_subsystem);
    g_healer.register_subsystem(g_net_subsystem);

    terminal_writeline("  [HEAL ] Self-healing layer online — 3 subsystems registered");
}

// ── healer_boot_subsystems — boot all registered subsystems with healing ───────
extern "C" void healer_boot_subsystems(void) {
    for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
        SubsystemBase* s = g_healer.subsystems[i];
        bool alive = g_healer.boot(s);

        // Print status line
        uint8_t color = alive
            ? (s->is_healthy()
                ? vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK)
                : vga_entry_color(VGA_YELLOW, VGA_BLACK))
            : vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);

        terminal_setcolor(color);
        terminal_write("  [HEAL ] ");
        terminal_write(s->name);
        if (s->is_healthy())  terminal_writeline(": healthy");
        else if (s->is_degraded()) terminal_writeline(": degraded (fallback active)");
        else terminal_writeline(": ISOLATED");
    }

    // Summary
    uint8_t white = vga_entry_color(VGA_WHITE, VGA_BLACK);
    terminal_setcolor(white);
}

// ── healer_tick — call from PIT interrupt handler every 1000ms ────────────────
extern "C" void healer_tick(void) {
    g_healer.tick_check();
}

// ── healer_status — returns number of isolated subsystems ─────────────────────
extern "C" uint32_t healer_isolated_count(void) {
    return g_healer.isolated_count();
}

// ── C API: watchdog tick (called from kernel.c main loop) ────────────────────
extern "C" void healer_watchdog_tick(void) {
    g_healer.tick_check();
}
