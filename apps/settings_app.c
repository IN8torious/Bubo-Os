// =============================================================================
// Deep Flow OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/settings_app.c — System Settings
// Voice calibration, theme, race defaults, accessibility options.
// =============================================================================

#include "font.h"

// ── Settings state ────────────────────────────────────────────────────────────

// ── Theme defines ─────────────────────────────────────────────────────────────
#define THEME_AKATSUKI  0
#define THEME_RAVEN     1
#define THEME_SOVEREIGN 2

typedef struct {
    uint32_t voice_sensitivity;
    uint32_t slur_tolerance;
    uint32_t glow_brightness;
    uint32_t ui_tier;
    uint32_t rinnegan_glow;
    uint32_t particle_count;
    uint32_t theme;
    uint32_t race_difficulty;
    bool     voice_enabled;
    bool     dysarthria_mode;
    bool     calm_mode;
    bool     particles_enabled;
    bool     hud_enabled;
    bool     race_auto_launch;
    bool     open;
    int32_t  active_tab;
    int32_t  win_x, win_y, win_w, win_h;
    char     user_name[32];
} settings_t;

static settings_t g_settings = {
    .voice_sensitivity   = 75,
    .slur_tolerance      = 80,
    .glow_brightness     = 90,
    .particle_count      = 200,
    .theme               = THEME_AKATSUKI,
    .voice_enabled       = true,
    .particles_enabled   = true,
    .hud_enabled         = true,
    .race_auto_launch    = false,
    .race_difficulty     = 2,
    .open                = false,
    .active_tab          = 0,
    .win_x = 200, .win_y = 80, .win_w = 480, .win_h = 400
};

static const char* TAB_NAMES[] = { "Voice", "Display", "Race", "About" };
#define TAB_COUNT 4

// ── Draw helpers ──────────────────────────────────────────────────────────────
static void draw_slider(int32_t x, int32_t y, int32_t w,
                        const char* label, uint8_t value, uint32_t color) {
    font_draw_string(x, y, label, 0xCCCCCC, 0, true);
    fb_fill_rect((uint32_t)x, (uint32_t)(y+14), (uint32_t)w, 8, 0x1A1030);
    uint32_t fill = (uint32_t)((w * value) / 100);
    if (fill > 0) polish_gradient_rect(x, y+14, (int32_t)fill, 8, color, 0x440000, false);
    // Value label
    char vbuf[8];
    uint8_t v = value;
    vbuf[0] = '0' + (v/100); vbuf[1] = '0' + ((v/10)%10); vbuf[2] = '0' + (v%10);
    vbuf[3] = '%'; vbuf[4] = 0;
    font_draw_string(x + w + 6, y + 10, vbuf, 0xFFFFFF, 0, true);
}

static void draw_toggle(int32_t x, int32_t y, const char* label, bool on) {
    font_draw_string(x, y, label, 0xCCCCCC, 0, true);
    uint32_t col = on ? 0x00CC44 : 0x440000;
    fb_fill_rect((uint32_t)(x+160), (uint32_t)y, 40, 14, col);
    font_draw_string(x+163, y+2, on ? "ON " : "OFF", 0xFFFFFF, 0, true);
}

// ── Tab renderers ─────────────────────────────────────────────────────────────
static void draw_voice_tab(int32_t x, int32_t y) {
    font_draw_string(x, y,    "VOICE & ACCESSIBILITY SETTINGS", 0xFF4444, 0, true);
    font_draw_string(x, y+16, "CORVUS adapts to your voice. No perfect speech required.", 0x888888, 0, true);
    draw_slider(x, y+40,  300, "Voice Sensitivity",  g_settings.voice_sensitivity,  0xCC2200);
    draw_slider(x, y+72,  300, "Slur Tolerance",     g_settings.slur_tolerance,     0xFF6600);
    draw_toggle(x, y+104, "Voice Commands",           g_settings.voice_enabled);
    font_draw_string(x, y+130, "Calibrate Voice — Say your name 3 times:", 0xCCCCCC, 0, true);
    fb_fill_rect((uint32_t)x, (uint32_t)(y+146), 300, 24, 0x1A1030);
    font_draw_string(x+8, y+152, "Press ENTER to begin calibration", 0x666666, 0, true);
    font_draw_string(x, y+182, "Dysarthria Engine: ACTIVE", 0x00CC44, 0, true);
    font_draw_string(x, y+198, "Phonetic fuzzy matching: ENABLED", 0x00CC44, 0, true);
    font_draw_string(x, y+214, "Partial word recognition: ENABLED", 0x00CC44, 0, true);
}

static void draw_display_tab(int32_t x, int32_t y) {
    font_draw_string(x, y,    "DISPLAY & VISUAL SETTINGS", 0xFF4444, 0, true);
    draw_slider(x, y+30,  300, "Glow Brightness",    g_settings.glow_brightness,    0xFF2244);
    draw_slider(x, y+62,  300, "Particle Count",     g_settings.particle_count/2,   0x8800CC);
    draw_toggle(x, y+94,  "Particle System",          g_settings.particles_enabled);
    draw_toggle(x, y+116, "HUD Overlay",              g_settings.hud_enabled);
    font_draw_string(x, y+146, "Theme:", 0xCCCCCC, 0, true);
    const char* themes[] = { "Akatsuki (Default)", "Void Black", "Crimson Storm" };
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t tc = (g_settings.theme == (uint32_t)i) ? 0x8B0000 : 0x1A1030;
        fb_fill_rect((uint32_t)x, (uint32_t)(y+162+(int32_t)(i*20)), 240, 16, tc);
        font_draw_string(x+6, y+164+(int32_t)(i*20), themes[i], 0xCCCCCC, 0, true);
    }
}

static void draw_race_tab(int32_t x, int32_t y) {
    font_draw_string(x, y,    "RACE SETTINGS", 0xFF4444, 0, true);
    draw_toggle(x, y+24,  "Auto-Launch on CORVUS READY", g_settings.race_auto_launch);
    font_draw_string(x, y+54, "Difficulty:", 0xCCCCCC, 0, true);
    const char* diffs[] = { "Casual", "Normal", "Hard", "Demon" };
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t tc = (g_settings.race_difficulty == (int32_t)i) ? 0x8B0000 : 0x1A1030;
        fb_fill_rect((uint32_t)x, (uint32_t)(y+70+(int32_t)(i*18)), 180, 14, tc);
        font_draw_string(x+6, y+72+(int32_t)(i*18), diffs[i], 0xCCCCCC, 0, true);
    }
    font_draw_string(x, y+148, "Default World: Nurburgring Nordschleife", 0x888888, 0, true);
    font_draw_string(x, y+164, "Car: Dodge Demon 170 (1,400 HP)", 0x888888, 0, true);
    font_draw_string(x, y+180, "Voice Control: ENABLED", 0x00CC44, 0, true);
}

static void draw_about_tab(int32_t x, int32_t y) {
    font_draw_string(x, y,    "RAVEN AOS v1.1", 0xFF4444, 0, true);
    font_draw_string(x, y+20, "Built by IN8torious | 2025", 0xCCCCCC, 0, true);
    font_draw_string(x, y+40, "In partnership with Manus AI", 0x888888, 0, true);
    font_draw_string(x, y+64, "Pioneer & Inventor:", 0xCCCCCC, 0, true);
    font_draw_string(x, y+80, "First constitutionally governed,", 0xFFAA00, 0, true);
    font_draw_string(x, y+96, "accessibility-first sovereign OS", 0xFFAA00, 0, true);
    font_draw_string(x, y+112,"built from bare metal with embedded", 0xFFAA00, 0, true);
    font_draw_string(x, y+128,"Multi-Agentic intelligence.", 0xFFAA00, 0, true);
    font_draw_string(x, y+156,"Dedicated to Landon Pankuch.", 0xFF4444, 0, true);
    font_draw_string(x, y+176,"Built for everyone told they cannot.", 0xCCCCCC, 0, true);
    font_draw_string(x, y+200,"NO MAS DISADVANTAGED.", 0xFF0000, 0, true);
    font_draw_string(x, y+224,"MIT License — Free for disabilities. Always.", 0x00CC44, 0, true);
    font_draw_string(x, y+248,"github.com/IN8torious/Instinct-Os", 0x4488FF, 0, true);
}

// ── Public API ────────────────────────────────────────────────────────────────
void settings_init(void) {
    // Defaults already set in static initializer
}

void settings_open(void)  { g_settings.open = true; }
void settings_close(void) { g_settings.open = false; }
bool settings_is_open(void){ return g_settings.open; }

settings_t* settings_get(void) { return &g_settings; }

void settings_handle_key(uint8_t key) {
    if (!g_settings.open) return;
    if (key == 0x1B) { settings_close(); return; }
    if (key == 0x4F) { // F5 — next tab
        g_settings.active_tab = (g_settings.active_tab + 1) % TAB_COUNT;
    }
}

void settings_draw(void) {
    if (!g_settings.open) return;
    int32_t x = g_settings.win_x, y = g_settings.win_y;
    int32_t w = g_settings.win_w, h = g_settings.win_h;

    // Window
    polish_glass_rect(x, y, w, h, 0x0D0820, 245);
    polish_gradient_rect(x, y, w, 28, 0x8B0000, 0x1A0A2A, false);
    font_draw_string(x+10, y+8, "SETTINGS", 0xFFFFFF, 0, true);

    // Tab bar
    int32_t tx = x + 4;
    for (int32_t i = 0; i < TAB_COUNT; i++) {
        uint32_t tc = (i == g_settings.active_tab) ? 0x8B0000 : 0x1A1030;
        fb_fill_rect((uint32_t)tx, (uint32_t)(y+32), 110, 20, tc);
        font_draw_string(tx+6, y+36, TAB_NAMES[i], 0xCCCCCC, 0, true);
        tx += 114;
    }

    // Content area
    int32_t cx = x + 12, cy = y + 58;
    fb_fill_rect((uint32_t)x, (uint32_t)cy, (uint32_t)w, (uint32_t)(h-58), 0x0A0818);

    switch (g_settings.active_tab) {
        case 0: draw_voice_tab(cx, cy+8);   break;
        case 1: draw_display_tab(cx, cy+8); break;
        case 2: draw_race_tab(cx, cy+8);    break;
        case 3: draw_about_tab(cx, cy+8);   break;
    }
}
