// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
// =============================================================================

// =============================================================================
// BUBO OS — BUBO MAS Dashboard
//
// Akatsuki Theme: Pure Black, Deep Crimson, Gold. No other colors.
// 2x5 Grid layout for 10 agents.
// =============================================================================

#include "bubo_dashboard.h"
#include "framebuffer.h"
#include "deepflow_colors.h"
#include "font.h"
#include "vga.h"

// Strict Akatsuki Palette overrides
#define AK_BLACK    DF_BG_DEEP
#define AK_PANEL    DF_BG_PANEL
#define AK_CRIMSON  DF_ERROR_STANDARD
#define AK_BLOOD    DF_ERROR_WARNING
#define AK_GOLD     DF_SOL_TESTING
#define AK_GRAY     DF_RED_DIM

// Agent definitions
static const char* AGENT_NAMES[10] = {
    "CROW", "HEALER", "SECURITY", "MEMORY", "NETWORK",
    "PLANNER", "LEARNER", "PHYSICS", "VOICE", "DRIVER"
};

static const char* AGENT_ROLES[10] = {
    "Orchestrator", "Self-repair", "Threat intel", "Vector store", "Comms API",
    "GOAP engine", "RL adapter", "World model", "NLP engine", "Demon 170"
};

// Simulated agent states (to be wired to real agent states)
static uint8_t agent_health[10]  = {100,98,100,95,90,97,88,100,100,100};
static uint8_t agent_load[10]    = {45, 12, 8,  30, 5, 22, 15, 10, 60, 80};
static bool    agent_active[10]  = {true,true,true,true,true,true,true,true,true,true};

// Maslow needs levels (0-100)
static uint8_t needs_survival    = 95;
static uint8_t needs_safety      = 90;
static uint8_t needs_connection  = 75;
static uint8_t needs_purpose     = 88;
static uint8_t needs_growth      = 70;

void bubo_dashboard_init(void) {
    terminal_write("[BUBO DASHBOARD] Initialized — Akatsuki theme active\n");
}

static void draw_bar(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t pct, uint32_t col_fill, uint32_t col_bg) {
    fb_fill_rect(x, y, w, h, col_bg);
    uint32_t fill = (w * pct) / 100;
    if (fill > 0) fb_fill_rect(x, y, fill, h, col_fill);
    fb_draw_rect(x, y, w, h, AK_BLOOD, 1);
}

void bubo_dashboard_render(uint32_t wx, uint32_t wy, uint32_t ww, uint32_t wh) {
    uint32_t cx = wx + 1;
    uint32_t cy = wy + 24;
    uint32_t cw = ww - 2;
    uint32_t ch = wh - 25;

    // Pure black background
    fb_fill_rect(cx, cy, cw, ch, AK_BLACK);

    // ── Header ──────────────────────────────────────────────────────────────
    fb_fill_rect(cx, cy, cw, 36, AK_PANEL);
    fb_fill_rect(cx, cy + 36, cw, 2, AK_CRIMSON); // Thin crimson border line
    font_draw_string_scaled(cx + 16, cy + 8, "BUBO MAS", AK_CRIMSON, 0, true, 2);
    font_draw_string(cx + cw - 180, cy + 12, "NO MAS DISADVANTAGED", AK_GOLD, 0, true);

    // ── Agent Grid (2 rows x 5 columns) ──────────────────────────────────────
    uint32_t grid_y = cy + 56;
    uint32_t card_w = (cw - 60) / 5;
    uint32_t card_h = 80;

    for (int i = 0; i < 10; i++) {
        uint32_t row = i / 5;
        uint32_t col = i % 5;
        uint32_t card_x = cx + 10 + col * (card_w + 10);
        uint32_t card_y = grid_y + row * (card_h + 10);

        // Card background and border
        fb_fill_rect(card_x, card_y, card_w, card_h, AK_PANEL);
        fb_draw_rect(card_x, card_y, card_w, card_h, AK_CRIMSON, 1);

        // Status dot
        uint32_t dot_col = agent_active[i] ? AK_CRIMSON : AK_GRAY;
        fb_fill_rect(card_x + 8, card_y + 8, 6, 6, dot_col);

        // Name and Role
        font_draw_string(card_x + 20, card_y + 6, AGENT_NAMES[i], AK_GOLD, 0, true);
        font_draw_string(card_x + 8, card_y + 24, AGENT_ROLES[i], AK_GRAY, 0, false);

        // Health bar
        font_draw_string(card_x + 8, card_y + 44, "HP", AK_GRAY, 0, false);
        draw_bar(card_x + 30, card_y + 44, card_w - 40, 6, agent_health[i], AK_CRIMSON, AK_BLACK);

        // Load bar
        font_draw_string(card_x + 8, card_y + 58, "LD", AK_GRAY, 0, false);
        draw_bar(card_x + 30, card_y + 58, card_w - 40, 6, agent_load[i], AK_BLOOD, AK_BLACK);
    }

    // ── Maslow Needs Model & Mandate Banner ──────────────────────────────────
    uint32_t bot_y = grid_y + 2 * (card_h + 10) + 16;
    
    // Needs (Bottom Left)
    const char* need_names[] = {"SURVIVAL","SAFETY","CONNECTION","PURPOSE","GROWTH"};
    uint8_t need_vals[] = {needs_survival, needs_safety, needs_connection, needs_purpose, needs_growth};
    uint32_t nx = cx + 10;
    uint32_t ny = bot_y;
    for (int i = 0; i < 5; i++) {
        font_draw_string(nx, ny, need_names[i], AK_GOLD, 0, true);
        draw_bar(nx + 90, ny + 2, 80, 8, need_vals[i], AK_CRIMSON, AK_BLACK);
        ny += 16;
    }

    // Mandate Banner (Bottom Right)
    uint32_t ban_w = cw - 220;
    uint32_t ban_h = 40;
    uint32_t ban_x = cx + 200;
    uint32_t ban_y = bot_y + 20;
    fb_fill_rect(ban_x, ban_y, ban_w, ban_h, AK_CRIMSON);
    font_draw_string_scaled(ban_x + (ban_w/2) - 150, ban_y + 12, "MANDATE: NO MAS DISADVANTAGED", AK_GOLD, 0, true, 1);
}
