// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/live_mode.c — CORVUS Live Mode
// Streaming overlay. One command starts your broadcast.
// Your OS. Your stream. Your story.
// =============================================================================

#include "live_mode.h"
#include "framebuffer.h"
#include "font.h"
#include "polish.h"
#include "corvus.h"
#include <stdint.h>
#include <stdbool.h>

static struct {
    bool     active;
    bool     recording;
    uint32_t frame_count;
    uint32_t tick;
    bool     show_agents;
    bool     show_mandate;
    bool     show_voice;
    bool     show_fps;
} g_live;

void live_mode_init(void) {
    g_live.active      = false;
    g_live.recording   = false;
    g_live.frame_count = 0;
    g_live.tick        = 0;
    g_live.show_agents = true;
    g_live.show_mandate= true;
    g_live.show_voice  = true;
    g_live.show_fps    = true;
}

void live_mode_start(void) { g_live.active = true; }
void live_mode_stop(void)  { g_live.active = false; }
bool live_mode_is_active(void){ return g_live.active; }

void live_mode_tick(void) {
    if (!g_live.active) return;
    g_live.tick++;
    if (g_live.recording) g_live.frame_count++;
}

static void draw_agent_bar(int32_t x, int32_t y, const char* name,
                           uint8_t health, uint32_t color) {
    font_draw_string(x, y, name, 0xCCCCCC, 0, true);
    fb_fill_rect((uint32_t)(x+40), (uint32_t)y, 80, 8, 0x1A1030);
    uint32_t fill = (uint32_t)((80 * health) / 100);
    if (fill > 0) fb_fill_rect((uint32_t)(x+40), (uint32_t)y, fill, 8, color);
}

void live_mode_draw_overlay(void) {
    if (!g_live.active) return;

    uint32_t sw = fb_get_info()->width;

    // Top-right: CORVUS LIVE badge
    uint32_t bx = sw - 140;
    polish_glass_rect((int32_t)bx, 4, 136, 22, 0x8B0000, 220);
    // Pulsing dot
    uint32_t dot_col = (g_live.tick & 32) ? 0xFF0000 : 0x880000;
    fb_fill_rect(bx+6, 10, 10, 10, dot_col);
    font_draw_string((int32_t)(bx+20), 8,
        g_live.recording ? "CORVUS LIVE REC" : "CORVUS LIVE", 0xFFFFFF, 0, true);

    // Mandate strip
    if (g_live.show_mandate) {
        polish_glass_rect(4, 30, 280, 18, 0x0D0820, 200);
        font_draw_string(8, 33, "NO MAS DISADVANTAGED", 0xFF4444, 0, true);
    }

    // Agent status column (bottom-left)
    if (g_live.show_agents) {
        int32_t ax = 4, ay = (int32_t)(fb_get_info()->height) - 120;
        polish_glass_rect(ax, ay, 140, 116, 0x0D0820, 210);
        font_draw_string(ax+4, ay+4, "CORVUS AGENTS", 0xFF4444, 0, true);
        corvus_agent_t* agents = corvus_get_agents();
        for (uint32_t i = 0; i < 10 && i < CORVUS_AGENT_COUNT; i++) {
            uint32_t hcol = agents[i].health > 70 ? 0x00CC44 :
                            agents[i].health > 40 ? 0xFFAA00 : 0xFF2200;
            draw_agent_bar(ax+4, ay+18+(int32_t)(i*9),
                           agents[i].name, agents[i].health, hcol);
        }
    }

    // Voice status (bottom-right)
    if (g_live.show_voice) {
        uint32_t vx = sw - 180;
        uint32_t vy = fb_get_info()->height - 30;
        polish_glass_rect((int32_t)vx, (int32_t)vy, 176, 26, 0x0D0820, 210);
        font_draw_string((int32_t)(vx+6), (int32_t)(vy+6),
            "VOICE: LISTENING", 0x00CC44, 0, true);
    }

    // FPS counter
    if (g_live.show_fps) {
        char fps_buf[16];
        uint32_t fps = (g_live.tick > 0) ? (g_live.frame_count * 60 / (g_live.tick + 1)) : 60;
        fps_buf[0] = '0' + (fps/10); fps_buf[1] = '0' + (fps%10);
        fps_buf[2] = ' '; fps_buf[3] = 'F'; fps_buf[4] = 'P'; fps_buf[5] = 'S'; fps_buf[6] = 0;
        font_draw_string((int32_t)(sw - 60), 8, fps_buf, 0x888888, 0, true);
    }
}
