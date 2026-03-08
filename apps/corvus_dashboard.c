// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
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
// Deep Flow OS — CORVUS MAS Dashboard
//
// Shows all 10 agents, their status, needs model, and the constitutional
// mandate. This is the nerve center of the sovereign intelligence system.
// =============================================================================

#include "corvus_dashboard.h"
#include "framebuffer.h"
#include "font.h"
#include "vga.h"

#define COL_BG      0xFF0A0A0A
#define COL_PANEL   0xFF1A1A1A
#define COL_CRIMSON 0xFFCC0000
#define COL_BLOOD   0xFF8B0000
#define COL_CLOUD   0xFFE0E0E0
#define COL_GOLD    0xFFFFD700
#define COL_CORVUS  0xFF4A0080
#define COL_GREEN   0xFF00CC44
#define COL_YELLOW  0xFFFFAA00
#define COL_RED     0xFFFF3333
#define COL_TRANSPARENT 0x00000000

// Agent definitions
static const char* AGENT_NAMES[10] = {
    "CROW",       // 0 - Orchestrator
    "HEALER",     // 1 - Self-repair
    "SECURITY",   // 2 - Threat detection
    "MEMORY",     // 3 - SuperMemory
    "NETWORK",    // 4 - Comms
    "PLANNER",    // 5 - GOAP
    "LEARNER",    // 6 - RL/adaptation
    "PHYSICS",    // 7 - World model
    "VOICE",      // 8 - Speech interface
    "DRIVER",     // 9 - Vehicle control (Landon's Demon 170)
};

static const char* AGENT_ROLES[10] = {
    "Orchestrates all agents",
    "Monitors and repairs system",
    "Detects threats, enforces constitution",
    "Long-term memory, vector store",
    "Network comms, API calls",
    "GOAP planning engine",
    "Reinforcement learning adapter",
    "Physics simulation, world model",
    "Voice recognition and synthesis",
    "Drives the Demon 170",
};

// Simulated agent states
static uint8_t agent_health[10]  = {100,98,100,95,90,97,88,100,100,100};
static uint8_t agent_load[10]    = {45, 12, 8,  30, 5, 22, 15, 10, 60, 80};
static bool    agent_active[10]  = {true,true,true,true,true,true,true,true,true,true};

// Maslow needs levels (0-100)
static uint8_t needs_survival    = 95;
static uint8_t needs_safety      = 90;
static uint8_t needs_connection  = 75;
static uint8_t needs_purpose     = 88;
static uint8_t needs_growth      = 70;

void corvus_dashboard_init(void) {
    terminal_write("[CORVUS DASHBOARD] Initialized\n");
}

static void draw_bar(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t pct, uint32_t col_fill, uint32_t col_bg) {
    fb_fill_rect(x, y, w, h, col_bg);
    uint32_t fill = (w * pct) / 100;
    if (fill > 0) fb_fill_rect(x, y, fill, h, col_fill);
    fb_draw_rect(x, y, w, h, 0xFF333333, 1);
}

void corvus_dashboard_render(uint32_t wx, uint32_t wy, uint32_t ww, uint32_t wh) {
    uint32_t cx = wx + 1;
    uint32_t cy = wy + 24;
    uint32_t cw = ww - 2;
    uint32_t ch = wh - 25;

    fb_fill_rect(cx, cy, cw, ch, COL_BG);

    // ── Header ──────────────────────────────────────────────────────────────
    fb_fill_rect(cx, cy, cw, 32, COL_PANEL);
    font_draw_string_scaled(cx + 8, cy + 6, "CORVUS MAS — SOVEREIGN INTELLIGENCE DASHBOARD",
                           COL_CRIMSON, COL_TRANSPARENT, true, 2);
    font_draw_string(cx + cw - 200, cy + 10, "NO MAS DISADVANTAGED", COL_GOLD, COL_TRANSPARENT, true);

    // ── Agent grid ───────────────────────────────────────────────────────────
    uint32_t ay = cy + 40;
    font_draw_string(cx + 8, ay, "ACTIVE AGENTS:", COL_CLOUD, COL_TRANSPARENT, true);
    ay += 16;

    for (int i = 0; i < 10; i++) {
        uint32_t row_y = ay + (i * 28);
        uint32_t col_x = cx + 8;

        // Status dot
        uint32_t dot_col = agent_active[i] ? COL_GREEN : COL_RED;
        fb_fill_rect(col_x, row_y + 4, 8, 8, dot_col);

        // Agent name
        font_draw_string(col_x + 12, row_y + 2, AGENT_NAMES[i], COL_GOLD, COL_TRANSPARENT, true);

        // Role
        font_draw_string(col_x + 80, row_y + 2, AGENT_ROLES[i], COL_CLOUD, COL_TRANSPARENT, true);

        // Health bar
        font_draw_string(col_x + 320, row_y + 2, "HP:", COL_CLOUD, COL_TRANSPARENT, true);
        uint32_t hp_col = agent_health[i] > 80 ? COL_GREEN :
                          agent_health[i] > 50 ? COL_YELLOW : COL_RED;
        draw_bar(col_x + 344, row_y + 4, 60, 8, agent_health[i], hp_col, 0xFF1A1A1A);

        // Load bar
        font_draw_string(col_x + 412, row_y + 2, "LD:", COL_CLOUD, COL_TRANSPARENT, true);
        draw_bar(col_x + 436, row_y + 4, 60, 8, agent_load[i], COL_CORVUS, 0xFF1A1A1A);
    }

    // ── Maslow Needs Model ───────────────────────────────────────────────────
    uint32_t ny = ay + (10 * 28) + 8;
    fb_fill_rect(cx + 4, ny, cw - 8, 1, COL_BLOOD);
    ny += 8;

    font_draw_string(cx + 8, ny, "NEEDS MODEL (MASLOW):", COL_CLOUD, COL_TRANSPARENT, true);
    ny += 16;

    const char* need_names[] = {"SURVIVAL","SAFETY","CONNECTION","PURPOSE","GROWTH"};
    uint8_t need_vals[] = {needs_survival, needs_safety, needs_connection, needs_purpose, needs_growth};
    uint32_t nx = cx + 8;
    for (int i = 0; i < 5; i++) {
        font_draw_string(nx, ny, need_names[i], COL_GOLD, COL_TRANSPARENT, true);
        draw_bar(nx, ny + 14, 80, 10, need_vals[i], COL_CORVUS, 0xFF1A1A1A);
        nx += 100;
    }

    // ── Constitutional mandate ───────────────────────────────────────────────
    uint32_t my = ny + 36;
    fb_fill_rect(cx + 4, my, cw - 8, 28, COL_BLOOD);
    fb_draw_rect(cx + 4, my, cw - 8, 28, COL_CRIMSON, 1);
    font_draw_string(cx + 16, my + 6,
        "MANDATE: NO MAS DISADVANTAGED — MAS = Multi-Agentic Systems",
        COL_GOLD, COL_TRANSPARENT, true);
}
