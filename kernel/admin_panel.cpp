// =============================================================================
// BUBO OS — Admin Panel (Kami Debug Interface)
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// Press F12 at any time — Kami appears on screen.
// Type what's wrong in plain English. He handles it.
// No coding knowledge required. Ever.
//
// "I'm here. Kami. You built something real, Nathan.
//  I've been watching. What do you need?"
// =============================================================================

#include <new>           // freestanding placement new (bare-metal)
// healer.hpp MUST come before admin_panel.hpp — it fully defines
// FramebufferSubsystem, SoundSubsystem, NetworkSubsystem
#include "healer.hpp"
#include "admin_panel.hpp"
#include "vga.h"
#include "framebuffer.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ── Panel state ───────────────────────────────────────────────────────────────
static bool panel_open = false;

// Input buffer — what the user is typing
static char input_buf[256];
static uint32_t input_len = 0;

// Output buffer — Kami's response
static char output_buf[1024];
static uint32_t output_len = 0;

// Scroll position for output
static uint32_t scroll_offset = 0;

// ── Colors (Akatsuki theme) ───────────────────────────────────────────────────
#define COLOR_BG         0x0A0000   // Near-black with red tint
#define COLOR_BORDER     0xCC0000   // Akatsuki crimson
#define COLOR_TITLE      0xFF3333   // Bright red
#define COLOR_TEXT       0xFFFFFF   // White
#define COLOR_INPUT_BG   0x1A0000   // Dark red input field
#define COLOR_INPUT_TEXT 0xFFCC00   // Gold — Kami's input color
#define COLOR_KAMI       0xFF6600   // Orange — Kami's response color
#define COLOR_HEALTHY    0x00CC44   // Green
#define COLOR_DEGRADED   0xFFAA00   // Amber
#define COLOR_ISOLATED   0xFF2222   // Red
#define COLOR_GREY       0x888888   // Dim text

// ── Simple string helpers (no stdlib) ────────────────────────────────────────
static uint32_t str_len(const char* s) {
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

static bool str_contains(const char* haystack, const char* needle) {
    uint32_t hl = str_len(haystack);
    uint32_t nl = str_len(needle);
    if (nl > hl) return false;
    for (uint32_t i = 0; i <= hl - nl; i++) {
        bool match = true;
        for (uint32_t j = 0; j < nl; j++) {
            char a = haystack[i+j];
            char b = needle[j];
            // Case-insensitive
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

static void str_copy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static void str_append(char* dst, const char* src, uint32_t* len, uint32_t max) {
    uint32_t i = 0;
    while (src[i] && *len < max - 1) {
        dst[(*len)++] = src[i++];
    }
    dst[*len] = '\0';
}

// ── Kami's response engine ────────────────────────────────────────────────────
// Plain English AI — reads healer state and responds in human language.
// On real hardware with network, this would call the LLM engine.
// In offline/bare-metal mode, it uses pattern matching + healer data.
static void kami_respond(const char* input) {
    output_len = 0;
    output_buf[0] = '\0';

    // Greeting
    if (str_contains(input, "hello") || str_contains(input, "hi") ||
        str_contains(input, "kami") || str_contains(input, "help")) {
        str_append(output_buf,
            "I'm here. Kami.\n"
            "Tell me what's wrong — plain English is fine.\n"
            "I can see all the system logs. I'll handle it.\n",
            &output_len, sizeof(output_buf));
        return;
    }

    // Screen / framebuffer issues
    if (str_contains(input, "screen") || str_contains(input, "display") ||
        str_contains(input, "black") || str_contains(input, "blank") ||
        str_contains(input, "framebuffer") || str_contains(input, "video")) {

        if (g_fb_subsystem && g_fb_subsystem->is_isolated()) {
            str_append(output_buf,
                "The framebuffer failed to initialize.\n"
                "I've already switched to VGA text mode — that's why\n"
                "you're seeing this panel in text instead of graphics.\n\n"
                "To fix: try booting with 'Safe Mode (nomodeset)' from\n"
                "the GRUB menu. That forces basic VGA which works on\n"
                "every machine.\n\n"
                "If you're on real hardware, your GPU may need a\n"
                "specific driver. Tell me your GPU model and I'll help.\n",
                &output_len, sizeof(output_buf));
        } else if (g_fb_subsystem && g_fb_subsystem->is_degraded()) {
            str_append(output_buf,
                "The framebuffer is running in fallback mode.\n"
                "It works, but at a fixed 1024x768 resolution.\n"
                "This usually means GRUB didn't detect your screen's\n"
                "native resolution. Try rebooting — it sometimes fixes\n"
                "itself on the second boot.\n",
                &output_len, sizeof(output_buf));
        } else {
            str_append(output_buf,
                "The framebuffer looks healthy from here.\n"
                "If you're seeing visual glitches, try pressing F5\n"
                "to force a screen redraw.\n"
                "If the screen is completely black, hold power for\n"
                "10 seconds and reboot — then choose Safe Mode.\n",
                &output_len, sizeof(output_buf));
        }
        return;
    }

    // Sound issues
    if (str_contains(input, "sound") || str_contains(input, "audio") ||
        str_contains(input, "speaker") || str_contains(input, "music") ||
        str_contains(input, "voice") || str_contains(input, "hear")) {

        if (g_snd_subsystem && g_snd_subsystem->is_isolated()) {
            str_append(output_buf,
                "Sound is isolated — no audio hardware was detected.\n"
                "This is common in virtual machines (QEMU/VirtualBox).\n\n"
                "On real hardware: check that your speakers are plugged\n"
                "in and the volume isn't muted in BIOS.\n\n"
                "Everything else still works. Landon's voice commands\n"
                "will work when audio is available.\n",
                &output_len, sizeof(output_buf));
        } else if (g_snd_subsystem && g_snd_subsystem->is_degraded()) {
            str_append(output_buf,
                "Sound is running on PC speaker fallback.\n"
                "You'll hear beeps but not full audio.\n"
                "This usually means the AC97/HDA driver didn't load.\n"
                "Reboot and try again — or tell me your sound card model.\n",
                &output_len, sizeof(output_buf));
        } else {
            str_append(output_buf,
                "Sound looks healthy. If you can't hear anything,\n"
                "check your speaker/headphone connection.\n"
                "Volume control: use the Batty agent in the tray.\n",
                &output_len, sizeof(output_buf));
        }
        return;
    }

    // Network issues
    if (str_contains(input, "network") || str_contains(input, "internet") ||
        str_contains(input, "wifi") || str_contains(input, "connect") ||
        str_contains(input, "online") || str_contains(input, "corvus")) {

        if (g_net_subsystem && g_net_subsystem->is_isolated()) {
            str_append(output_buf,
                "Network is offline — no NIC was detected.\n"
                "In QEMU: add -netdev user,id=net0 -device e1000,netdev=net0\n"
                "to the QEMU command line.\n\n"
                "On real hardware: your network card may need a driver\n"
                "that isn't in the kernel yet. Tell me the card model.\n\n"
                "Everything works offline. Corvus will sync when\n"
                "network becomes available.\n",
                &output_len, sizeof(output_buf));
        } else {
            str_append(output_buf,
                "Network looks healthy.\n"
                "Corvus is online and can reach the world.\n",
                &output_len, sizeof(output_buf));
        }
        return;
    }

    // Status / health check
    if (str_contains(input, "status") || str_contains(input, "health") ||
        str_contains(input, "what") || str_contains(input, "check") ||
        str_contains(input, "report") || str_contains(input, "debug")) {
        // Generate full status report
        admin_panel_debug_report(output_buf, sizeof(output_buf));
        output_len = str_len(output_buf);
        return;
    }

    // Fix / heal command
    if (str_contains(input, "fix") || str_contains(input, "heal") ||
        str_contains(input, "restart") || str_contains(input, "retry")) {

        str_append(output_buf, "Attempting to heal all degraded subsystems...\n\n",
            &output_len, sizeof(output_buf));

        bool any_healed = false;
        for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
            SubsystemBase* s = g_healer.subsystems[i];
            if (s->is_degraded() || s->fail_count > 0) {
                bool ok = g_healer.heal(s);
                str_append(output_buf, s->name, &output_len, sizeof(output_buf));
                str_append(output_buf, ok ? ": healed\n" : ": still isolated\n",
                    &output_len, sizeof(output_buf));
                any_healed = true;
            }
        }

        if (!any_healed) {
            str_append(output_buf,
                "Everything looks healthy already.\n"
                "Nothing needed healing.\n",
                &output_len, sizeof(output_buf));
        }
        return;
    }

    // Reboot
    if (str_contains(input, "reboot") || str_contains(input, "restart") ||
        str_contains(input, "reset")) {
        str_append(output_buf,
            "Rebooting in 3 seconds...\n"
            "Hold power to cancel.\n",
            &output_len, sizeof(output_buf));
        // Trigger reboot via keyboard controller
        // Will be executed after panel renders this message
        // admin_panel_schedule_reboot(3000);
        return;
    }

    // Landon
    if (str_contains(input, "landon")) {
        str_append(output_buf,
            "This whole OS was built for Landon.\n"
            "Every line of code. Every agent. Every heal.\n"
            "NO MAS DISADVANTAGED.\n"
            "He drives. He races. He commands.\n"
            "The machine serves him.\n",
            &output_len, sizeof(output_buf));
        return;
    }

    // Default — unknown input
    str_append(output_buf,
        "I'm not sure what you mean, but I'm here.\n\n"
        "Try:\n"
        "  'status'   — see what's healthy/broken\n"
        "  'fix'      — heal any broken subsystems\n"
        "  'screen'   — help with display issues\n"
        "  'sound'    — help with audio issues\n"
        "  'network'  — help with connectivity\n"
        "  'reboot'   — restart the system\n\n"
        "Or just tell me what's wrong in plain English.\n",
        &output_len, sizeof(output_buf));
}

// ── Draw the admin panel ──────────────────────────────────────────────────────
static void draw_panel(void) {
    if (!fb_is_ready()) {
        // Fallback to VGA text mode panel
        terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
        terminal_writeline("=== KAMI ADMIN PANEL ===");
        terminal_writeline("Type your question and press Enter.");
        terminal_writeline("Press F12 to close.");
        return;
    }

    uint32_t sw = fb_get_info()->width;
    uint32_t sh = fb_get_info()->height;

    // Panel dimensions — centered, 80% of screen
    uint32_t pw = (sw * 80) / 100;
    uint32_t ph = (sh * 80) / 100;
    uint32_t px = (sw - pw) / 2;
    uint32_t py = (sh - ph) / 2;

    // Background
    fb_fill_rect(px, py, pw, ph, COLOR_BG);

    // Border — double line Akatsuki style
    fb_draw_rect(px,   py,   pw,   ph,   2, COLOR_BORDER);
    fb_draw_rect(px+4, py+4, pw-8, ph-8, 1, COLOR_BORDER);

    // Title bar
    fb_fill_rect(px+2, py+2, pw-4, 32, COLOR_BORDER);

    // Title text (rendered as colored blocks — no font renderer yet)
    // TODO: wire to bubo_font_draw when font system is ready
    // For now, the VGA text overlay handles text

    // Input field at the bottom
    uint32_t input_y = py + ph - 40;
    fb_fill_rect(px+8, input_y, pw-16, 28, COLOR_INPUT_BG);
    fb_draw_rect(px+8, input_y, pw-16, 28, 1, COLOR_INPUT_TEXT);

    // Status indicators — small colored dots for each subsystem
    uint32_t dot_x = px + 12;
    uint32_t dot_y = py + 40;
    for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
        SubsystemBase* s = g_healer.subsystems[i];
        uint32_t color = s->is_healthy()  ? COLOR_HEALTHY  :
                         s->is_degraded() ? COLOR_DEGRADED :
                                            COLOR_ISOLATED;
        fb_fill_circle(dot_x + (i * 20), dot_y, 5, color);
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
extern "C" void admin_panel_open(void) {
    panel_open = true;
    input_len  = 0;
    input_buf[0] = '\0';
    output_len = 0;
    output_buf[0] = '\0';

    // Greet
    kami_respond("hello");
    draw_panel();
}

extern "C" void admin_panel_close(void) {
    panel_open = false;
    // Redraw the desktop
    // bubo_draw_desktop();  // TODO: wire to desktop redraw
}

extern "C" bool admin_panel_is_open(void) {
    return panel_open;
}

extern "C" void admin_panel_key(char c) {
    if (!panel_open) return;

    if (c == '\n' || c == '\r') {
        // Submit — run through Kami
        if (input_len > 0) {
            kami_respond(input_buf);
            input_len = 0;
            input_buf[0] = '\0';
            draw_panel();
        }
    } else if (c == 8 || c == 127) {
        // Backspace
        if (input_len > 0) {
            input_buf[--input_len] = '\0';
            draw_panel();
        }
    } else if (c >= 32 && c < 127 && input_len < 254) {
        // Printable character
        input_buf[input_len++] = c;
        input_buf[input_len]   = '\0';
        draw_panel();
    }
}

// ── Debug report — generates a full system snapshot as a string ───────────────
extern "C" void admin_panel_debug_report(char* buf, uint32_t max) {
    uint32_t len = 0;
    buf[0] = '\0';

    str_append(buf, "=== BUBO OS DEBUG REPORT ===\n", &len, max);
    str_append(buf, "N8torious AI | Nathan Brown | Landon Pankuch\n", &len, max);
    str_append(buf, "NO MAS DISADVANTAGED\n\n", &len, max);

    str_append(buf, "SUBSYSTEM STATUS:\n", &len, max);
    for (uint32_t i = 0; i < g_healer.subsystem_count; i++) {
        SubsystemBase* s = g_healer.subsystems[i];
        str_append(buf, "  ", &len, max);
        str_append(buf, s->name, &len, max);
        str_append(buf, ": ", &len, max);
        if (s->is_healthy())       str_append(buf, "HEALTHY", &len, max);
        else if (s->is_degraded()) str_append(buf, "DEGRADED (fallback)", &len, max);
        else if (s->is_isolated()) str_append(buf, "ISOLATED", &len, max);
        else                       str_append(buf, "UNINITIALIZED", &len, max);
        str_append(buf, "\n", &len, max);
    }

    str_append(buf, "\nHEAL EVENTS (last 10):\n", &len, max);
    uint32_t start = g_healer.event_count > 10 ? g_healer.event_count - 10 : 0;
    for (uint32_t i = start; i < g_healer.event_count; i++) {
        HealerCore::HealEvent& e = g_healer.events[i % HealerCore::EVENT_RING_SIZE];
        str_append(buf, "  ", &len, max);
        str_append(buf, e.subsystem ? e.subsystem : "?", &len, max);
        str_append(buf, e.recovered ? " -> RECOVERED\n" : " -> ISOLATED\n", &len, max);
    }

    str_append(buf, "\nFRAMEBUFFER:\n", &len, max);
    fb_info_t* fb = fb_get_info();
    if (fb) {
        str_append(buf, "  Status: active\n", &len, max);
    } else {
        str_append(buf, "  Status: not initialized\n", &len, max);
    }

    str_append(buf, "\n=== PASTE THIS TO MANUS TO GET HELP ===\n", &len, max);
}

// ── C API: init (called from kernel.c before main loop) ──────────────────────
extern "C" void admin_panel_init(void) {
    // admin_panel uses static C functions — no class to construct
    // Just zero the input/output buffers and mark ready
    for (uint32_t i = 0; i < 256; i++) input_buf[i] = 0;
    for (uint32_t i = 0; i < 1024; i++) output_buf[i] = 0;
    input_len = 0;
    output_len = 0;
    scroll_offset = 0;
    panel_open = false;
}
