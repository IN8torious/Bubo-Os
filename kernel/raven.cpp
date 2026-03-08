// =============================================================================
// BUBO OS — RAVEN Meta-Debugger Implementation
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================

#include <new>
#include "raven.hpp"
#include "healer.hpp"
#include "admin_panel.hpp"
#include "vga.h"
#include "framebuffer.h"
#include <stdint.h>
#include <stddef.h>

// ── Global RAVEN instance (no heap — static storage) ─────────────────────────
static uint8_t raven_storage[sizeof(RavenCore)];
RavenCore* g_raven_ptr = nullptr;
RavenCore& g_raven = *reinterpret_cast<RavenCore*>(raven_storage);

// ── PIT tick counter (incremented by PIT handler) ────────────────────────────
extern "C" volatile uint32_t pit_ticks;

// ── String helpers ────────────────────────────────────────────────────────────
static uint32_t rv_strlen(const char* s) {
    uint32_t n = 0; while (s[n]) n++; return n;
}
static void rv_strcpy(char* d, const char* s, uint32_t max) {
    uint32_t i = 0;
    while (s[i] && i < max-1) { d[i] = s[i]; i++; }
    d[i] = '\0';
}
static void rv_append(char* d, const char* s, uint32_t* len, uint32_t max) {
    uint32_t i = 0;
    while (s[i] && *len < max-1) { d[(*len)++] = s[i++]; }
    d[*len] = '\0';
}
static void rv_u64_hex(char* buf, uint64_t v) {
    const char* hex = "0123456789ABCDEF";
    buf[0]='0'; buf[1]='x';
    for (int i = 15; i >= 2; i--) {
        buf[i] = hex[v & 0xF];
        v >>= 4;
    }
    buf[16] = '\0';
}
static void rv_u32_dec(char* buf, uint32_t v) {
    if (v == 0) { buf[0]='0'; buf[1]='\0'; return; }
    char tmp[12]; int i = 0;
    while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

// ── RavenCore::log ────────────────────────────────────────────────────────────
void RavenCore::log(RavenEvent ev, uint8_t severity,
                    const char* msg, uint64_t data) {
    RavenEntry& e = ring[head % RING_SIZE];
    e.timestamp = pit_ticks;
    e.event     = ev;
    e.severity  = severity;
    e.data      = data;
    rv_strcpy(e.message, msg ? msg : "", sizeof(e.message));
    head = (head + 1) % RING_SIZE;
    total_count++;

    // Mirror critical events to VGA terminal immediately
    if (severity >= 2) {
        uint8_t color = severity >= 3
            ? vga_entry_color(VGA_LIGHT_RED,    VGA_BLACK)
            : vga_entry_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
        terminal_setcolor(color);
        terminal_write("  [RAVEN] ");
        terminal_writeline(msg ? msg : "?");
        terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
    }
}

// ── RavenCore::watchdog_tick ──────────────────────────────────────────────────
// Called every second from PIT. Checks that critical systems are alive.
void RavenCore::watchdog_tick(void) {
    // Check healer is still registered
    if (g_healer.subsystem_count == 0) {
        log(RavenEvent::WATCHDOG_TRIGGER, 3,
            "WATCHDOG: Healer has 0 subsystems — possible corruption", 0);
        return;
    }

    // Check for newly isolated subsystems since last tick
    for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
        SubsystemBase* s = g_healer.subsystems[i];
        if (s->is_isolated() && s->fail_count == 1) {
            // First time isolated — log it
            log(RavenEvent::SUBSYSTEM_ISOLATED, 2, s->name, i);
        }
    }

    // Check admin panel isn't stuck open with no input for > 5 minutes
    // (300 seconds = 300 * 100 PIT ticks at 100Hz = 30000 ticks)
    // TODO: add last_activity timestamp to admin panel
}

// ── RavenCore::enter_dump_mode ────────────────────────────────────────────────
void RavenCore::enter_dump_mode(void) {
    dump_mode = true;
    log(RavenEvent::RAVEN_DUMP, 0, "RAVEN dump mode activated", 0);

    // Clear screen and show raw dump
    terminal_init();

    uint8_t cyan  = vga_entry_color(VGA_CYAN,        VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,    VGA_BLACK);
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN,  VGA_BLACK);
    uint8_t amber = vga_entry_color(VGA_BROWN,  VGA_BLACK);

    terminal_setcolor(red);
    terminal_writeline("=================================================================");
    terminal_writeline("  RAVEN META-DEBUGGER — BUBO OS  |  N8torious AI");
    terminal_writeline("  Ctrl+F12 to exit  |  Paste this output to Manus for help");
    terminal_writeline("=================================================================");

    // Subsystem status
    terminal_setcolor(cyan);
    terminal_writeline("\nSUBSYSTEM STATUS:");
    terminal_setcolor(white);
    for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
        SubsystemBase* s = g_healer.subsystems[i];
        terminal_write("  ");
        terminal_write(s->name);
        terminal_write(": ");
        if (s->is_healthy()) {
            terminal_setcolor(green);
            terminal_writeline("HEALTHY");
        } else if (s->is_degraded()) {
            terminal_setcolor(amber);
            terminal_writeline("DEGRADED (fallback active)");
        } else if (s->is_isolated()) {
            terminal_setcolor(red);
            terminal_writeline("ISOLATED");
        } else {
            terminal_setcolor(white);
            terminal_writeline("UNINIT");
        }
        terminal_setcolor(white);
    }

    // Last 16 RAVEN events
    terminal_setcolor(cyan);
    terminal_writeline("\nLAST 16 RAVEN EVENTS:");
    terminal_setcolor(white);

    uint32_t start = total_count > 16 ? total_count - 16 : 0;
    for (uint32_t i = start; i < total_count; i++) {
        RavenEntry& e = ring[i % RING_SIZE];

        // Timestamp
        char ts[12];
        rv_u32_dec(ts, (uint32_t)(e.timestamp / 100)); // seconds
        terminal_write("  [");
        terminal_write(ts);
        terminal_write("s] ");

        // Severity color
        if (e.severity >= 3)      terminal_setcolor(red);
        else if (e.severity >= 2) terminal_setcolor(amber);
        else if (e.severity >= 1) terminal_setcolor(vga_entry_color(VGA_YELLOW, VGA_BLACK));
        else                      terminal_setcolor(white);

        terminal_write(e.message);

        // Data if nonzero
        if (e.data) {
            char hex[18];
            rv_u64_hex(hex, e.data);
            terminal_write(" @ ");
            terminal_write(hex);
        }
        terminal_putchar('\n');
        terminal_setcolor(white);
    }

    // Framebuffer info
    terminal_setcolor(cyan);
    terminal_writeline("\nFRAMEBUFFER:");
    terminal_setcolor(white);
    fb_info_t* fb = fb_get_info();
    if (fb) {
        char tmp[16];
        terminal_write("  ");
        rv_u32_dec(tmp, fb->width);  terminal_write(tmp);
        terminal_write("x");
        rv_u32_dec(tmp, fb->height); terminal_write(tmp);
        terminal_write("x");
        rv_u32_dec(tmp, fb->bpp);    terminal_write(tmp);
        terminal_write("bpp  addr=");
        char hex[18]; rv_u64_hex(hex, fb->addr);
        terminal_writeline(hex);
    } else {
        terminal_writeline("  Not initialized");
    }

    // Footer
    terminal_setcolor(red);
    terminal_writeline("\n=================================================================");
    terminal_writeline("  PASTE EVERYTHING ABOVE TO MANUS — github.com/IN8torious/Bubo-Os");
    terminal_writeline("  NO MAS DISADVANTAGED");
    terminal_writeline("=================================================================");
    terminal_setcolor(white);
}

void RavenCore::exit_dump_mode(void) {
    dump_mode = false;
    log(RavenEvent::RAVEN_DUMP, 0, "RAVEN dump mode exited", 0);
    // Redraw whatever was on screen before
    // bubo_draw_desktop();  // TODO: wire to desktop redraw
    terminal_init();
    terminal_writeline("  [RAVEN] Dump mode closed. Press F12 for Kami.");
}

// ── RavenCore::generate_dump ──────────────────────────────────────────────────
void RavenCore::generate_dump(char* buf, uint32_t max) {
    uint32_t len = 0;
    buf[0] = '\0';

    rv_append(buf, "=== RAVEN DUMP | BUBO OS | N8torious AI ===\n", &len, max);
    rv_append(buf, "Nathan Brown | Landon Pankuch | NO MAS DISADVANTAGED\n\n", &len, max);

    rv_append(buf, "SUBSYSTEMS:\n", &len, max);
    for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
        SubsystemBase* s = g_healer.subsystems[i];
        rv_append(buf, "  ", &len, max);
        rv_append(buf, s->name, &len, max);
        rv_append(buf, ": ", &len, max);
        if (s->is_healthy())       rv_append(buf, "HEALTHY\n", &len, max);
        else if (s->is_degraded()) rv_append(buf, "DEGRADED\n", &len, max);
        else if (s->is_isolated()) rv_append(buf, "ISOLATED\n", &len, max);
        else                       rv_append(buf, "UNINIT\n", &len, max);
    }

    rv_append(buf, "\nEVENTS:\n", &len, max);
    uint32_t start = total_count > 32 ? total_count - 32 : 0;
    for (uint32_t i = start; i < total_count; i++) {
        RavenEntry& e = ring[i % RING_SIZE];
        char ts[12]; rv_u32_dec(ts, (uint32_t)(e.timestamp / 100));
        rv_append(buf, "  [", &len, max);
        rv_append(buf, ts, &len, max);
        rv_append(buf, "s] ", &len, max);
        rv_append(buf, e.message, &len, max);
        rv_append(buf, "\n", &len, max);
    }

    rv_append(buf, "\n=== PASTE TO MANUS ===\n", &len, max);
}

bool RavenCore::has_critical(void) {
    // Check last 16 events for severity >= 3
    uint32_t start = total_count > 16 ? total_count - 16 : 0;
    for (uint32_t i = start; i < total_count; i++) {
        if (ring[i % RING_SIZE].severity >= 3) return true;
    }
    return false;
}

// ── C API ─────────────────────────────────────────────────────────────────────
extern "C" void raven_init(void) {
    // Placement new on static storage
    new(raven_storage) RavenCore();
    g_raven.boot_ticks = pit_ticks;
    g_raven.log(RavenEvent::BOOT_START, 0, "RAVEN online — watching everything", 0);

    terminal_setcolor(vga_entry_color(VGA_CYAN, VGA_BLACK));
    terminal_writeline("  [RAVEN] Meta-debugger online — Ctrl+F12 for raw dump");
    terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
}

extern "C" void raven_log(uint8_t event, uint8_t severity,
                           const char* msg, uint64_t data) {
    g_raven.log(static_cast<RavenEvent>(event), severity, msg, data);
}

extern "C" void raven_watchdog_tick(void) {
    g_raven.watchdog_tick();
}

extern "C" void raven_dump_mode_toggle(void) {
    if (g_raven.dump_mode) g_raven.exit_dump_mode();
    else                   g_raven.enter_dump_mode();
}

extern "C" bool raven_dump_mode_active(void) {
    return g_raven.dump_mode;
}

extern "C" void raven_key(char c) {
    // In dump mode, any key exits
    if (g_raven.dump_mode) {
        g_raven.exit_dump_mode();
    }
}

extern "C" void raven_generate_dump(char* buf, uint32_t max) {
    g_raven.generate_dump(buf, max);
}
