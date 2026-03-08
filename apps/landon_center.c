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
// Deep Flow OS — Landon Pankuch Accessibility Center
//
// This is Landon's command center. Every pixel here exists for him.
// CORVUS drives the Demon 170 based on his voice commands.
// He doesn't need to touch a keyboard. He just speaks.
//
// "NO MAS DISADVANTAGED" — Landon is empowered, not assisted.
// =============================================================================

#include "landon_center.h"
#include "voice.h"
#include "desktop.h"
#include "framebuffer.h"
#include "font.h"
#include "vga.h"
#include "corvus_constitution.h"

// ── Colors ────────────────────────────────────────────────────────────────────
#define COL_BG        0xFF001122
#define COL_PANEL     0xFF002244
#define COL_LANDON    0xFF0066CC
#define COL_GOLD      0xFFFFD700
#define COL_CLOUD     0xFFE0E0E0
#define COL_CRIMSON   0xFFCC0000
#define COL_GREEN     0xFF00CC44
#define COL_DEMON     0xFF8B0000   // Sinamon Red
#define COL_TRANSPARENT 0x00000000

// ── Voice command table ───────────────────────────────────────────────────────
static const landon_voice_cmd_t VOICE_COMMANDS[] = {
    { "FASTER",    VCMD_FASTER,    "CORVUS increases throttle on the Demon 170" },
    { "BRAKE",     VCMD_BRAKE,     "CORVUS applies brakes" },
    { "DRIFT",     VCMD_DRIFT,     "CORVUS initiates controlled drift" },
    { "NITRO",     VCMD_NITRO,     "CORVUS activates nitrous — 1,400 HP unleashed" },
    { "OVERTAKE",  VCMD_OVERTAKE,  "CORVUS plans and executes an overtake" },
    { "TURN LEFT", VCMD_TURN_LEFT,      "CORVUS steers left" },
    { "TURN RIGHT",VCMD_TURN_RIGHT,     "CORVUS steers right" },
    { "PIT STOP",  VCMD_PIT_STOP,       "CORVUS pulls into pit lane" },
    { "STATUS",    VCMD_STATUS,    "CORVUS reports race status" },
    { "STOP",      VCMD_STOP,      "CORVUS brings the car to a full stop" },
    { "LAUNCH",    VCMD_LAUNCH,    "CORVUS performs a launch control start" },
    { "MUSIC",     VCMD_MUSIC,     "CORVUS toggles in-car music" },
    { "HELP",      VCMD_HELP,      "CORVUS lists available commands" },
};
#define NUM_VOICE_CMDS 13

// ── State ─────────────────────────────────────────────────────────────────────
static landon_center_t g_landon;
static char g_voice_buffer[256];
static int  g_voice_len = 0;
static char g_last_cmd[64];
static char g_corvus_response[256];

// ── Initialize Landon's center ────────────────────────────────────────────────
void landon_center_init(void) {
    g_landon.active            = true;
    g_landon.voice_active      = true;
    g_landon.race_running      = false;
    g_landon.corvus_driving    = false;
    g_landon.speed_mph         = 0;
    g_landon.lap               = 0;
    g_landon.position          = 1;
    g_landon.nitro_available   = true;

    for (int i = 0; i < 256; i++) g_voice_buffer[i] = 0;
    for (int i = 0; i < 64;  i++) g_last_cmd[i] = 0;
    for (int i = 0; i < 256; i++) g_corvus_response[i] = 0;

    terminal_write("[LANDON] Accessibility Center initialized\n");
    terminal_write("[LANDON] CORVUS voice interface ACTIVE\n");
    terminal_write("[LANDON] Landon Pankuch — NO MAS DISADVANTAGED\n");
}

// ── Render Landon's center in a window ───────────────────────────────────────
void landon_center_render(uint32_t wx, uint32_t wy, uint32_t ww, uint32_t wh) {
    uint32_t cx = wx + 1;
    uint32_t cy = wy + 24;  // Below title bar
    uint32_t cw = ww - 2;
    uint32_t ch = wh - 25;

    // Background
    fb_fill_rect(cx, cy, cw, ch, COL_BG);

    // ── Header ──────────────────────────────────────────────────────────────
    fb_fill_rect(cx, cy, cw, 40, COL_PANEL);
    font_draw_string_scaled(cx + 8, cy + 8,
        "LANDON PANKUCH — VOICE COMMAND CENTER",
        COL_LANDON, COL_TRANSPARENT, true, 2);
    font_draw_string(cx + cw - 200, cy + 12,
        "NO MAS DISADVANTAGED", COL_GOLD, COL_TRANSPARENT, true);

    // ── CORVUS driving status ───────────────────────────────────────────────
    uint32_t sy = cy + 48;
    fb_fill_rect(cx + 4, sy, cw - 8, 60, COL_PANEL);
    fb_draw_rect(cx + 4, sy, cw - 8, 60, COL_DEMON, 2);

    font_draw_string(cx + 12, sy + 8, "CORVUS STATUS:", COL_CLOUD, COL_TRANSPARENT, true);
    if (g_landon.corvus_driving) {
        font_draw_string(cx + 120, sy + 8, "DRIVING — 1,400 HP DEMON 170",
                         COL_GREEN, COL_TRANSPARENT, true);
    } else {
        font_draw_string(cx + 120, sy + 8, "STANDING BY",
                         COL_GOLD, COL_TRANSPARENT, true);
    }

    // Speed gauge
    char speed_str[32] = "SPEED: ";
    char spd[8]; itoa_simple(g_landon.speed_mph, spd);
    str_append(speed_str, spd); str_append(speed_str, " MPH");
    font_draw_string(cx + 12, sy + 28, speed_str, COL_CLOUD, COL_TRANSPARENT, true);

    // Lap / position
    char lap_str[32] = "LAP: ";
    char lap_n[4]; itoa_simple(g_landon.lap, lap_n);
    str_append(lap_str, lap_n);
    font_draw_string(cx + 160, sy + 28, lap_str, COL_CLOUD, COL_TRANSPARENT, true);

    char pos_str[32] = "POS: P";
    char pos_n[4]; itoa_simple(g_landon.position, pos_n);
    str_append(pos_str, pos_n);
    font_draw_string(cx + 260, sy + 28, pos_str, COL_GOLD, COL_TRANSPARENT, true);

    if (g_landon.nitro_available) {
        font_draw_string(cx + 360, sy + 28, "NITRO: READY", COL_CRIMSON, COL_TRANSPARENT, true);
    }

    // ── Voice input box ─────────────────────────────────────────────────────
    uint32_t vy = sy + 72;
    fb_fill_rect(cx + 4, vy, cw - 8, 36, 0xFF001133);
    fb_draw_rect(cx + 4, vy, cw - 8, 36, COL_LANDON, 2);
    font_draw_string(cx + 12, vy + 10, "VOICE:", COL_LANDON, COL_TRANSPARENT, true);
    font_draw_string_scaled(cx + 72, vy + 6, g_voice_buffer, COL_CLOUD, COL_TRANSPARENT, true, 2);

    // ── Last command & CORVUS response ──────────────────────────────────────
    uint32_t ry = vy + 44;
    font_draw_string(cx + 12, ry, "LAST CMD:", COL_CLOUD, COL_TRANSPARENT, true);
    font_draw_string(cx + 90, ry, g_last_cmd, COL_GOLD, COL_TRANSPARENT, true);

    ry += 18;
    font_draw_string(cx + 12, ry, "CORVUS:", COL_LANDON, COL_TRANSPARENT, true);
    font_draw_string(cx + 72, ry, g_corvus_response, COL_GREEN, COL_TRANSPARENT, true);

    // ── Command reference grid ───────────────────────────────────────────────
    uint32_t gry = ry + 28;
    font_draw_string(cx + 12, gry, "AVAILABLE COMMANDS:", COL_CLOUD, COL_TRANSPARENT, true);
    gry += 16;

    uint32_t col_x = cx + 12;
    uint32_t row_y = gry;
    for (int i = 0; i < NUM_VOICE_CMDS; i++) {
        fb_fill_rect(col_x, row_y, 90, 14, 0xFF001A33);
        fb_draw_rect(col_x, row_y, 90, 14, COL_LANDON, 1);
        font_draw_string(col_x + 3, row_y + 2, VOICE_COMMANDS[i].phrase,
                         COL_GOLD, COL_TRANSPARENT, true);
        col_x += 96;
        if (col_x + 90 > (uint32_t)(cx + cw - 8)) {
            col_x = cx + 12;
            row_y += 18;
        }
    }

    // ── Launch race button ───────────────────────────────────────────────────
    uint32_t btn_y = cy + ch - 44;
    fb_fill_rect(cx + 4, btn_y, 180, 36, COL_DEMON);
    fb_draw_rect(cx + 4, btn_y, 180, 36, COL_GOLD, 2);
    font_draw_string_scaled(cx + 16, btn_y + 8, "LAUNCH RACE", COL_GOLD, COL_TRANSPARENT, true, 2);

    fb_fill_rect(cx + 192, btn_y, 160, 36, COL_PANEL);
    fb_draw_rect(cx + 192, btn_y, 160, 36, COL_LANDON, 2);
    font_draw_string(cx + 200, btn_y + 12, "CORVUS DRIVE FOR ME", COL_LANDON, COL_TRANSPARENT, true);
}

// ── Process a voice command string ───────────────────────────────────────────
void landon_process_voice(const char* cmd) {
    // Copy to last command
    int i = 0;
    while (cmd[i] && i < 63) { g_last_cmd[i] = cmd[i]; i++; }
    g_last_cmd[i] = 0;

    terminal_write("[LANDON] Voice command: ");
    terminal_write(cmd);
    terminal_write("\n");

    // Match command
    for (int c = 0; c < NUM_VOICE_CMDS; c++) {
        if (str_match(cmd, VOICE_COMMANDS[c].phrase)) {
            corvus_evaluate_action(9, VOICE_COMMANDS[c].phrase);
            landon_execute_cmd(VOICE_COMMANDS[c].id);
            str_copy(g_corvus_response, VOICE_COMMANDS[c].response, 255);
            terminal_write("[CORVUS] ");
            terminal_write(VOICE_COMMANDS[c].response);
            terminal_write("\n");
            return;
        }
    }

    // Unknown command
    str_copy(g_corvus_response, "Command not recognized. Say HELP for options.", 255);
    terminal_write("[CORVUS] Command not recognized\n");
}

// ── Execute a parsed voice command ───────────────────────────────────────────
void landon_execute_cmd(uint32_t cmd_id) {
    switch (cmd_id) {
    case VCMD_FASTER:
        g_landon.speed_mph += 15;
        if (g_landon.speed_mph > 200) g_landon.speed_mph = 200;
        g_landon.corvus_driving = true;
        break;
    case VCMD_BRAKE:
        if (g_landon.speed_mph > 20) g_landon.speed_mph -= 30;
        else g_landon.speed_mph = 0;
        break;
    case VCMD_DRIFT:
        // Drift mode — speed maintained, direction changes
        g_landon.corvus_driving = true;
        break;
    case VCMD_NITRO:
        if (g_landon.nitro_available) {
            g_landon.speed_mph += 40;
            if (g_landon.speed_mph > 200) g_landon.speed_mph = 200;
            g_landon.nitro_available = false;
        }
        break;
    case VCMD_OVERTAKE:
        if (g_landon.position > 1) g_landon.position--;
        g_landon.speed_mph += 10;
        break;
    case VCMD_TURN_LEFT:
    case VCMD_TURN_RIGHT:
        g_landon.corvus_driving = true;
        break;
    case VCMD_PIT_STOP:
        g_landon.speed_mph = 60;
        g_landon.nitro_available = true;
        break;
    case VCMD_STATUS:
        // Status is rendered in the UI
        break;
    case VCMD_STOP:
        g_landon.speed_mph = 0;
        g_landon.corvus_driving = false;
        break;
    case VCMD_LAUNCH:
        g_landon.speed_mph = 0;
        g_landon.race_running = true;
        g_landon.corvus_driving = true;
        g_landon.lap = 1;
        g_landon.position = 8;
        break;
    case VCMD_MUSIC:
        // Toggle music — handled by audio subsystem
        break;
    case VCMD_HELP:
        str_copy(g_corvus_response, "FASTER BRAKE DRIFT NITRO OVERTAKE STOP LAUNCH STATUS", 255);
        break;
    default:
        break;
    }
}

// ── Keyboard input to voice buffer ───────────────────────────────────────────
void landon_center_key(char key) {
    if (key == '\n' || key == '\r') {
        // Submit voice buffer as command
        if (g_voice_len > 0) {
            g_voice_buffer[g_voice_len] = 0;
            landon_process_voice(g_voice_buffer);
            g_voice_len = 0;
            g_voice_buffer[0] = 0;
        }
    } else if (key == '\b') {
        if (g_voice_len > 0) g_voice_buffer[--g_voice_len] = 0;
    } else if (g_voice_len < 254) {
        g_voice_buffer[g_voice_len++] = key;
        g_voice_buffer[g_voice_len] = 0;
    }
}

// ── Update race simulation tick ───────────────────────────────────────────────
void landon_center_tick(void) {
    if (!g_landon.race_running || !g_landon.corvus_driving) return;

    // Simulate CORVUS driving — gradually improve position
    static uint32_t tick = 0;
    tick++;

    // Every 200 ticks, advance a lap
    if (tick % 200 == 0) {
        g_landon.lap++;
        if (g_landon.lap > 10) g_landon.lap = 10;
    }

    // Every 300 ticks, gain a position
    if (tick % 300 == 0 && g_landon.position > 1) {
        g_landon.position--;
    }

    // Speed fluctuates naturally
    if (tick % 7 == 0) {
        if (g_landon.speed_mph > 0) {
            g_landon.speed_mph += (tick % 3 == 0) ? 2 : -1;
            if (g_landon.speed_mph > 185) g_landon.speed_mph = 185;
            if (g_landon.speed_mph < 80 && g_landon.race_running) g_landon.speed_mph = 80;
        }
    }
}

// ── Simple string helpers ─────────────────────────────────────────────────────
void itoa_simple(uint32_t n, char* buf) {
    if (n == 0) { buf[0]='0'; buf[1]=0; return; }
    char tmp[12]; int i=0;
    while (n) { tmp[i++]='0'+(n%10); n/=10; }
    int j=0; while(i>0) buf[j++]=tmp[--i]; buf[j]=0;
}

void str_append(char* dst, const char* src) {
    int i=0; while(dst[i]) i++;
    int j=0; while(src[j]) dst[i++]=src[j++]; dst[i]=0;
}

void str_copy(char* dst, const char* src, int max) {
    int i=0; while(src[i]&&i<max-1){dst[i]=src[i];i++;} dst[i]=0;
}

bool str_match(const char* a, const char* b) {
    int i=0;
    while (a[i]&&b[i]) {
        char ca=a[i], cb=b[i];
        if (ca>='a'&&ca<='z') ca-=32;
        if (cb>='a'&&cb<='z') cb-=32;
        if (ca!=cb) return false;
        i++;
    }
    return a[i]==0 && b[i]==0;
}
