// =============================================================================
// Raven AOS — Dedicated to Landon Pankuch
// =============================================================================
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
// Raven AOS — CORVUS Visual Display
// Renders the Akatsuki boot screen, CORVUS dashboard, and agent status panels
// on the VESA framebuffer. This is CORVUS's face.
// =============================================================================

#include "framebuffer.h"
#include "font.h"
#include "../include/corvus.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ── Akatsuki color palette ────────────────────────────────────────────────────
#define AK_BLACK        0x000000
#define AK_DEEP_BLACK   0x080808
#define AK_CRIMSON      0xCC0000
#define AK_BLOOD_RED    0x8B0000
#define AK_BRIGHT_RED   0xFF2222
#define AK_DARK_RED     0x440000
#define AK_WHITE        0xFFFFFF
#define AK_GREY         0x333333
#define AK_LIGHT_GREY   0xAAAAAA
#define AK_DARK_GREY    0x1A1A1A
#define AK_ORANGE       0xFF6600   // Naruto orange
#define AK_PURPLE       0x6600CC   // Rinnegan purple
#define AK_CYAN         0x00CCCC   // CORVUS accent

// ── Screen layout constants ───────────────────────────────────────────────────
#define SCREEN_W    1024
#define SCREEN_H    768
#define TASKBAR_H   32
#define SIDEBAR_W   200
#define PANEL_PAD   8

// ── Internal helpers ──────────────────────────────────────────────────────────

static void draw_text(int32_t x, int32_t y, const char* str, uint32_t color) {
    font_draw_string(x, y, str, color, 0, false);
}

static void draw_text_bg(int32_t x, int32_t y, const char* str,
                          uint32_t fg, uint32_t bg) {
    font_draw_string(x, y, str, fg, bg, true);
}

static void draw_panel(int32_t x, int32_t y, int32_t w, int32_t h,
                        uint32_t bg, uint32_t border) {
    fb_fill_rect(x, y, w, h, bg);
    fb_draw_rect(x, y, w, h, 1, border);
}

// ── Rinnegan eye symbol (concentric circles with tomoe lines) ─────────────────
static void draw_rinnegan(int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    // Outer ring
    fb_draw_circle(cx, cy, r,     color);
    fb_draw_circle(cx, cy, r-1,   color);
    // Middle ring
    fb_draw_circle(cx, cy, r*2/3, color);
    // Inner dot
    fb_fill_circle(cx, cy, r/6,   color);
    // 6 spoke lines (Rinnegan pattern)
    for (int i = 0; i < 6; i++) {
        // Approximate 6 directions using integer math
        // angles: 0, 60, 120, 180, 240, 300 degrees
        // cos/sin approximations for 6 directions
        static const int32_t dx6[6] = { 10,  5, -5, -10, -5,  5};
        static const int32_t dy6[6] = {  0,  9,  9,   0, -9, -9};
        int32_t x1 = cx + (dx6[i] * r / 12);
        int32_t y1 = cy + (dy6[i] * r / 12);
        int32_t x2 = cx + (dx6[i] * (r-3) / 12);
        int32_t y2 = cy + (dy6[i] * (r-3) / 12);
        fb_draw_line(x1, y1, x2, y2, color);
    }
}

// ── Red cloud pattern (Akatsuki cloak symbol) ─────────────────────────────────
static void draw_red_cloud(int32_t x, int32_t y, int32_t size) {
    // Draw a simplified cloud shape using circles and rectangles
    int32_t r = size / 4;
    // Base rectangle
    fb_fill_rect(x, y + r, size, r, AK_WHITE);
    // Cloud bumps
    fb_fill_circle(x + r,         y + r, r, AK_WHITE);
    fb_fill_circle(x + size/2,    y + r/2, r + r/3, AK_WHITE);
    fb_fill_circle(x + size - r,  y + r, r, AK_WHITE);
    // Red outline
    fb_draw_circle(x + r,         y + r, r, AK_CRIMSON);
    fb_draw_circle(x + size/2,    y + r/2, r + r/3, AK_CRIMSON);
    fb_draw_circle(x + size - r,  y + r, r, AK_CRIMSON);
}

// ── Boot splash screen ────────────────────────────────────────────────────────
void corvus_draw_boot_screen(void) {
    if (!fb_is_ready()) return;

    // Black background
    fb_clear(AK_DEEP_BLACK);

    // Gradient scanlines (subtle dark red at top)
    for (int y = 0; y < 200; y++) {
        uint32_t intensity = (uint32_t)(y * 0x08 / 200);
        uint32_t color = (intensity << 16); // dark red tint
        fb_draw_hline(0, y, SCREEN_W, color);
    }

    // Large Rinnegan in center
    int32_t cx = SCREEN_W / 2;
    int32_t cy = SCREEN_H / 2 - 60;
    draw_rinnegan(cx, cy, 80, AK_PURPLE);
    draw_rinnegan(cx, cy, 78, AK_PURPLE);

    // Inner Rinnegan rings in crimson
    fb_draw_circle(cx, cy, 40, AK_CRIMSON);
    fb_fill_circle(cx, cy, 12, AK_CRIMSON);

    // "RAVEN AOS" title
    draw_text(cx - 120, cy + 110, "RAVEN  AOS", AK_WHITE);
    draw_text(cx - 121, cy + 109, "RAVEN  AOS", AK_LIGHT_GREY); // shadow

    // "CORVUS" subtitle in crimson
    draw_text(cx - 72, cy + 130, "CORVUS  v0.4", AK_CRIMSON);

    // Red cloud decorations
    draw_red_cloud(50,  SCREEN_H - 120, 80);
    draw_red_cloud(SCREEN_W - 140, SCREEN_H - 120, 80);

    // Bottom bar
    fb_fill_rect(0, SCREEN_H - 40, SCREEN_W, 40, AK_DARK_RED);
    fb_draw_hline(0, SCREEN_H - 40, SCREEN_W, AK_CRIMSON);
    draw_text(SCREEN_W/2 - 100, SCREEN_H - 28,
              "Initializing CORVUS agents...", AK_LIGHT_GREY);

    // Horizontal separator lines
    fb_draw_hline(cx - 200, cy + 100, 400, AK_CRIMSON);
    fb_draw_hline(cx - 180, cy + 102, 360, AK_DARK_RED);
}

// ── Progress bar during boot ──────────────────────────────────────────────────
void corvus_draw_boot_progress(uint32_t percent, const char* label) {
    if (!fb_is_ready()) return;

    int32_t bar_x = SCREEN_W/2 - 200;
    int32_t bar_y = SCREEN_H - 70;
    int32_t bar_w = 400;
    int32_t bar_h = 12;

    // Clear bar area
    fb_fill_rect(bar_x - 2, bar_y - 20, bar_w + 4, bar_h + 24, AK_DEEP_BLACK);

    // Label
    if (label) draw_text(bar_x, bar_y - 16, label, AK_LIGHT_GREY);

    // Background
    fb_fill_rect(bar_x, bar_y, bar_w, bar_h, AK_DARK_GREY);
    fb_draw_rect(bar_x, bar_y, bar_w, bar_h, 1, AK_GREY);

    // Fill
    uint32_t fill_w = (bar_w * percent) / 100;
    if (fill_w > 0) {
        fb_fill_rect(bar_x, bar_y, (int32_t)fill_w, bar_h, AK_CRIMSON);
        // Highlight
        fb_draw_hline(bar_x, bar_y, (int32_t)fill_w, AK_BRIGHT_RED);
    }
}

// ── Main desktop background ───────────────────────────────────────────────────
void corvus_draw_desktop(void) {
    if (!fb_is_ready()) return;

    // Deep black background
    fb_clear(AK_DEEP_BLACK);

    // Subtle red cloud pattern across the background (decorative)
    for (int i = 0; i < 5; i++) {
        int32_t x = 80 + i * 190;
        int32_t y = SCREEN_H / 2 - 20;
        // Very dark red cloud silhouette
        uint32_t cloud_color = 0x1A0000;
        fb_fill_circle(x,      y, 18, cloud_color);
        fb_fill_circle(x + 25, y - 8, 22, cloud_color);
        fb_fill_circle(x + 50, y, 18, cloud_color);
        fb_fill_rect(x - 2, y, 54, 20, cloud_color);
    }

    // Top taskbar
    fb_fill_rect(0, 0, SCREEN_W, TASKBAR_H, AK_DARK_RED);
    fb_draw_hline(0, TASKBAR_H, SCREEN_W, AK_CRIMSON);
    fb_draw_hline(0, TASKBAR_H - 1, SCREEN_W, AK_BLOOD_RED);

    // Taskbar: CORVUS logo (small Rinnegan)
    draw_rinnegan(16, TASKBAR_H/2, 10, AK_PURPLE);

    // Taskbar: OS name
    draw_text(32, 8, "RAVEN AOS", AK_WHITE);
    draw_text(32, 9, "RAVEN AOS", AK_LIGHT_GREY);

    // Taskbar: right side — time placeholder
    draw_text(SCREEN_W - 80, 8, "CORVUS", AK_CRIMSON);

    // Bottom taskbar
    fb_fill_rect(0, SCREEN_H - TASKBAR_H, SCREEN_W, TASKBAR_H, AK_DARK_GREY);
    fb_draw_hline(0, SCREEN_H - TASKBAR_H, SCREEN_W, AK_CRIMSON);

    // Bottom bar: agent status indicators
    const char* agent_names[10] = {
        "CROW", "CROW2", "CROW3", "CROW4", "CROW5",
        "HEALER", "SECURITY", "MEMORY", "NETWORK", "CORVUS"
    };
    for (int i = 0; i < 10; i++) {
        int32_t ax = 8 + i * 100;
        int32_t ay = SCREEN_H - TASKBAR_H + 6;
        // Green dot = active
        fb_fill_circle(ax + 4, ay + 6, 4, 0x00AA00);
        draw_text(ax + 12, ay, agent_names[i], AK_LIGHT_GREY);
    }
}

// ── CORVUS agent dashboard panel ─────────────────────────────────────────────
void corvus_draw_agent_panel(int32_t x, int32_t y) {
    if (!fb_is_ready()) return;

    int32_t pw = 280, ph = 320;

    // Panel background
    draw_panel(x, y, pw, ph, AK_DARK_GREY, AK_CRIMSON);

    // Header
    fb_fill_rect(x, y, pw, 24, AK_BLOOD_RED);
    draw_text(x + PANEL_PAD, y + 4, "CORVUS AGENT STATUS", AK_WHITE);

    // Rinnegan icon in header
    draw_rinnegan(x + pw - 16, y + 12, 8, AK_PURPLE);

    // Agent rows
    const char* agents[10] = {
        "Crow       ", "Crow II    ", "Crow III   ", "Crow IV    ", "Crow V     ",
        "Healer     ", "Security   ", "Memory     ", "Network    ", "Orchestrator"
    };
    const char* statuses[10] = {
        "ACTIVE", "ACTIVE", "ACTIVE", "IDLE  ", "IDLE  ",
        "ACTIVE", "ACTIVE", "ACTIVE", "SLEEP ", "ACTIVE"
    };
    uint32_t status_colors[10] = {
        0x00CC00, 0x00CC00, 0x00CC00, 0xCCCC00, 0xCCCC00,
        0x00CC00, 0x00CC00, 0x00CC00, 0x888888, AK_CRIMSON
    };

    for (int i = 0; i < 10; i++) {
        int32_t row_y = y + 30 + i * 28;

        // Alternating row background
        if (i % 2 == 0)
            fb_fill_rect(x + 1, row_y, pw - 2, 27, 0x1A1A1A);

        // Status dot
        fb_fill_circle(x + 14, row_y + 13, 5, status_colors[i]);

        // Agent name
        draw_text(x + 24, row_y + 6, agents[i], AK_WHITE);

        // Status text
        draw_text(x + 180, row_y + 6, statuses[i], status_colors[i]);

        // Need bar (visual representation of agent need level)
        int32_t need = 60 + (i * 13) % 40; // 60-100%
        int32_t bar_w = (int32_t)((pw - 24) * need / 100);
        fb_fill_rect(x + 12, row_y + 22, pw - 24, 3, AK_DARK_GREY);
        fb_fill_rect(x + 12, row_y + 22, bar_w, 3, status_colors[i]);
    }
}

// ── CORVUS shell window ───────────────────────────────────────────────────────
void corvus_draw_shell_window(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!fb_is_ready()) return;

    // Window chrome
    draw_panel(x, y, w, h, AK_DEEP_BLACK, AK_CRIMSON);

    // Title bar
    fb_fill_rect(x, y, w, 22, AK_BLOOD_RED);
    draw_text(x + 8, y + 4, "CORVUS SHELL  v0.4", AK_WHITE);

    // Window controls (circles)
    fb_fill_circle(x + w - 12, y + 11, 5, AK_CRIMSON);
    fb_fill_circle(x + w - 26, y + 11, 5, 0xCCCC00);
    fb_fill_circle(x + w - 40, y + 11, 5, 0x00CC00);

    // Shell content area
    fb_fill_rect(x + 1, y + 23, w - 2, h - 24, 0x050505);

    // CORVUS prompt
    draw_text(x + 8, y + 30, "RAVEN AOS v0.4 — CORVUS Orchestration Engine", AK_CRIMSON);
    draw_text(x + 8, y + 46, "10 agents initialized. Constitutional governance: ACTIVE", AK_LIGHT_GREY);
    draw_text(x + 8, y + 62, "Type 'help' for commands. Type 'agents' for agent status.", AK_GREY);
    draw_text(x + 8, y + 86, "corvus> _", AK_BRIGHT_RED);
}

// ── Memory usage bar ──────────────────────────────────────────────────────────
void corvus_draw_memory_bar(int32_t x, int32_t y, int32_t w,
                             uint32_t used_kb, uint32_t total_kb) {
    if (!fb_is_ready()) return;

    draw_text(x, y, "MEMORY", AK_LIGHT_GREY);

    int32_t bar_y = y + 14;
    fb_fill_rect(x, bar_y, w, 10, AK_DARK_GREY);
    fb_draw_rect(x, bar_y, w, 10, 1, AK_GREY);

    if (total_kb > 0) {
        uint32_t fill = (uint32_t)w * used_kb / total_kb;
        uint32_t color = (used_kb * 100 / total_kb > 80) ? AK_BRIGHT_RED : AK_CRIMSON;
        fb_fill_rect(x, bar_y, (int32_t)fill, 10, color);
    }
}

// ── Full CORVUS dashboard (combines all panels) ───────────────────────────────
void corvus_draw_dashboard(void) {
    if (!fb_is_ready()) return;

    // Draw desktop base
    corvus_draw_desktop();

    // Agent panel on the right
    corvus_draw_agent_panel(SCREEN_W - 290, TASKBAR_H + 10);

    // Shell window in center
    corvus_draw_shell_window(10, TASKBAR_H + 10, SCREEN_W - 310, 340);

    // Memory bar below shell
    corvus_draw_memory_bar(10, TASKBAR_H + 360, 300, 2048, 262144);

    // Decorative Rinnegan watermark (large, very dark)
    // Bottom right corner
    int32_t wm_x = SCREEN_W - 310;
    int32_t wm_y = SCREEN_H - TASKBAR_H - 160;
    for (int r = 120; r >= 115; r--)
        fb_draw_circle(wm_x, wm_y, r, 0x0A0000);
}
