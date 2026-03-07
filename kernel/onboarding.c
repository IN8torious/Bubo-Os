// =============================================================================
// RAVEN ENGINE — Onboarding & User Profile System
// Raven AOS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/onboarding.c
// First-boot account creation. Disability-aware adaptive UI tier assignment.
// Built on trust, not verification. The constitutional mandate in action.
//
// "We are all human data collectors." — Nathan Samuel
// =============================================================================

#include "onboarding.h"
#include "framebuffer.h"
#include "font.h"
#include "polish.h"
#include "keyboard.h"
#include "vga.h"
#include "vfs.h"
#include <stdint.h>
#include <stdbool.h>

// ── Key code defines (PS/2 scan codes mapped to ASCII-adjacent values) ────────
#define KEY_ENTER     0x0D
#define KEY_BACKSPACE 0x08
#define KEY_UP        0x80
#define KEY_DOWN      0x81
#define KEY_LEFT      0x82
#define KEY_RIGHT     0x83
#define KEY_ESCAPE    0x1B
#define KEY_TAB       0x09



// ── Profile storage ───────────────────────────────────────────────────────────
static raven_user_profile_t g_profile;
static bool g_profile_loaded = false;

// ── Colors ────────────────────────────────────────────────────────────────────
#define COL_BG         0x0A0A14
#define COL_PANEL      0x12122A
#define COL_ACCENT     0xCC2222
#define COL_TEXT       0xEEEEEE
#define COL_DIM        0x888888
#define COL_GREEN      0x22CC66
#define COL_GOLD       0xFFCC44
#define COL_PURPLE     0x8844CC
#define COL_SELECTED   0x1A1A4A
#define COL_BORDER     0x333366

// ── String helpers ────────────────────────────────────────────────────────────
static uint32_t str_len(const char* s) {
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}
static void str_copy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (src[i] && i < max-1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}
static bool str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}

// ── Screen drawing helpers ────────────────────────────────────────────────────
static void draw_screen_base(void) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    // Background
    fb_fill_rect(0, 0, sw, sh, COL_BG);

    // Top bar
    fb_fill_rect(0, 0, sw, 48, COL_ACCENT);
    font_draw_string(16, 14, "RAVEN AOS", COL_TEXT, COL_ACCENT, true);
    font_draw_string((int32_t)(sw - 280), 14,
                     "NO MAS DISADVANTAGED", COL_GOLD, COL_ACCENT, true);

    // Bottom bar
    fb_fill_rect(0, (int32_t)(sh-32), sw, 32, COL_PANEL);
    font_draw_string(16, (int32_t)(sh-22),
                     "Your answers stay on your device. Always.", COL_DIM, COL_PANEL, true);
}

static void draw_centered_text(int32_t y, const char* text,
                                uint32_t color, uint32_t bg) {
    fb_info_t* info = fb_get_info();
    int32_t x = (int32_t)(info->width/2) - (int32_t)(str_len(text)*8/2);
    font_draw_string(x, y, text, color, bg, true);
}

static void draw_button(int32_t x, int32_t y, uint32_t w, uint32_t h,
                         const char* label, bool selected) {
    uint32_t bg = selected ? COL_ACCENT : COL_PANEL;
    uint32_t fg = selected ? COL_TEXT   : COL_DIM;
    polish_rounded_rect(x, y, (int32_t)w, (int32_t)h, 6, bg);
    fb_draw_rect(x, y, (int32_t)w, (int32_t)h, 1, COL_BORDER);
    int32_t tx = x + (int32_t)(w/2) - (int32_t)(str_len(label)*8/2);
    font_draw_string(tx, y+8, label, fg, bg, true);
}

// ── Screen 1 — Welcome ────────────────────────────────────────────────────────
static void screen_welcome(void) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;
    draw_screen_base();

    // Rinnegan symbol (circle)
    polish_glow_circle((int32_t)(sw/2), (int32_t)(sh/2 - 60), 50,
                        COL_ACCENT, COL_PURPLE, 1.0f);

    draw_centered_text((int32_t)(sh/2 - 10),
        "Welcome to Raven AOS", COL_TEXT, COL_BG);
    draw_centered_text((int32_t)(sh/2 + 20),
        "Built for everyone. Especially those who need it most.", COL_DIM, COL_BG);
    draw_centered_text((int32_t)(sh/2 + 50),
        "We have a few questions to set up your experience.", COL_DIM, COL_BG);
    draw_centered_text((int32_t)(sh/2 + 80),
        "You don't have to answer anything you're not comfortable with.", COL_DIM, COL_BG);

    draw_button((int32_t)(sw/2 - 80), (int32_t)(sh - 100), 160, 36,
                "Let's Begin", true);
}

// ── Screen 2 — Basic profile ──────────────────────────────────────────────────
static void screen_basic_profile(const char* name_buf, uint32_t use_sel) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;
    draw_screen_base();

    font_draw_string(60, 80, "Step 1 of 3 — Tell us about yourself", COL_GOLD, COL_BG, true);
    font_draw_string(60, 120, "What would you like CORVUS to call you?", COL_TEXT, COL_BG, true);

    // Name input box
    polish_glass_rect(60, 148, 400, 32, COL_PANEL, 220);
    fb_draw_rect(60, 148, 400, 32, 1, COL_BORDER);
    font_draw_string(68, 156, name_buf, COL_TEXT, 0, true);
    // Cursor
    int32_t cx = 68 + (int32_t)(str_len(name_buf)*8);
    fb_fill_rect((uint32_t)cx, 152, 2, 22, COL_TEXT);

    font_draw_string(60, 210, "What is your primary use?", COL_TEXT, COL_BG, true);

    const char* uses[] = {"Racing", "Creative", "Work", "General", "Accessibility"};
    for (uint32_t i = 0; i < 5; i++) {
        draw_button(60 + (int32_t)(i*180), 238, 160, 36,
                    uses[i], use_sel == i);
    }

    draw_button((int32_t)(sw - 180), (int32_t)(sh - 100), 140, 36, "Next ->", true);
}

// ── Screen 3 — Accessibility questions ───────────────────────────────────────
typedef struct {
    const char* question;
    const char* unlocks;
    bool        answer;
} access_q_t;

static access_q_t g_questions[ONBOARD_Q_COUNT] = {
    {"Do you use voice as your primary input?",
     "Full voice pipeline + dysarthria engine", false},
    {"Do you have difficulty with fine motor control?",
     "Large targets, voice shortcuts, simplified UI", false},
    {"Do you experience differences in speech (slurring, pacing)?",
     "Personal voice calibration + dysarthria adaptation", false},
    {"Do you experience cognitive differences?",
     "Simplified mode, CORVUS explains, slower pacing", false},
    {"Do you experience visual differences?",
     "High contrast, large text, screen reader, audio descriptions", false},
    {"Do you experience hearing differences?",
     "Visual alerts, vibration output, no audio-only info", false},
    {"Do you use a wheelchair or have limited mobility?",
     "Accessibility strip always visible, Sovereign racing mode", false},
    {"Do you experience mental health conditions?",
     "Calm mode, reduced notifications, gentle CORVUS tone", false},
};

static void screen_accessibility(uint32_t highlight) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width;
    draw_screen_base();

    font_draw_string(60, 70, "Step 2 of 3 — Accessibility", COL_GOLD, COL_BG, true);
    font_draw_string(60, 100,
        "These questions help CORVUS adapt to how you communicate.",
        COL_TEXT, COL_BG, true);
    font_draw_string(60, 118,
        "There are no wrong answers. You can change these at any time.",
        COL_DIM, COL_BG, true);

    for (uint32_t i = 0; i < ONBOARD_Q_COUNT; i++) {
        int32_t qy = 150 + (int32_t)(i * 52);
        bool hl = (highlight == i);

        // Row background
        uint32_t row_bg = hl ? COL_SELECTED : COL_BG;
        fb_fill_rect(50, (uint32_t)qy, sw-100, 46, row_bg);
        fb_draw_rect(50, qy, (int32_t)(sw-100), 46, 1, hl ? COL_ACCENT : COL_BORDER);

        // Question text
        font_draw_string(70, qy+6, g_questions[i].question, COL_TEXT, row_bg, true);
        font_draw_string(70, qy+22, g_questions[i].unlocks, COL_DIM, row_bg, true);

        // YES / NO buttons
        draw_button((int32_t)(sw-220), qy+8, 70, 28, "YES",
                     g_questions[i].answer == true && hl);
        draw_button((int32_t)(sw-140), qy+8, 70, 28, "NO",
                     g_questions[i].answer == false && hl);

        // Checkmark if answered yes
        if (g_questions[i].answer)
            font_draw_string((int32_t)(sw-260), qy+10, "[YES]", COL_GREEN, row_bg, true);
    }
}

// ── Screen 4 — Trust statement ────────────────────────────────────────────────
static void screen_trust(ui_tier_t tier) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;
    draw_screen_base();

    font_draw_string(60, 80, "Step 3 of 3 — Your Profile", COL_GOLD, COL_BG, true);

    // Tier display
    const char* tier_name = (tier == TIER_SOVEREIGN) ? "SOVEREIGN" : "STANDARD";
    uint32_t tier_col = (tier == TIER_SOVEREIGN) ? COL_PURPLE : COL_GREEN;

    font_draw_string(60, 130, "Your access tier:", COL_TEXT, COL_BG, true);
    font_draw_string(240, 130, tier_name, tier_col, COL_BG, true);

    if (tier == TIER_SOVEREIGN) {
        font_draw_string(60, 160,
            "Full CORVUS voice control. Dysarthria engine. All accessibility features.",
            COL_DIM, COL_BG, true);
        font_draw_string(60, 178,
            "Everything is free. Forever. No exceptions.", COL_GREEN, COL_BG, true);
    } else {
        font_draw_string(60, 160,
            "Full desktop and all apps. CORVUS assistant.", COL_DIM, COL_BG, true);
        font_draw_string(60, 178,
            "You can update your profile at any time if your needs change.",
            COL_DIM, COL_BG, true);
    }

    // Privacy statement
    polish_glass_rect(60, 220, (int32_t)(sw-120), 80, COL_PANEL, 200);
    font_draw_string(76, 234,
        "Everything you shared stays on your device.", COL_TEXT, COL_PANEL, true);
    font_draw_string(76, 252,
        "CORVUS does not send your profile to any server.", COL_DIM, COL_PANEL, true);
    font_draw_string(76, 270,
        "Say 'CORVUS UPDATE MY PROFILE' at any time to change these settings.",
        COL_DIM, COL_PANEL, true);

    draw_centered_text((int32_t)(sh/2 + 80),
        "\"We are all human data collectors.\"", COL_GOLD, COL_BG);
    draw_centered_text((int32_t)(sh/2 + 100),
        "— Nathan Samuel, Builder of Raven AOS", COL_DIM, COL_BG);

    draw_button((int32_t)(sw/2 - 100), (int32_t)(sh-100), 200, 40,
                "Enter Raven AOS", true);
}

// ── UI tier calculation ───────────────────────────────────────────────────────
static ui_tier_t calculate_tier(void) {
    for (uint32_t i = 0; i < ONBOARD_Q_COUNT; i++)
        if (g_questions[i].answer) return TIER_SOVEREIGN;
    return TIER_STANDARD;
}

// ── Profile serialization ─────────────────────────────────────────────────────
static void profile_to_string(char* buf, uint32_t max) {
    // Simple key=value format
    uint32_t pos = 0;
    const char* name = g_profile.name;
    // name=...
    const char* k1 = "name=";
    for (uint32_t i=0;k1[i]&&pos<max-1;i++) buf[pos++]=k1[i];
    for (uint32_t i=0;name[i]&&pos<max-1;i++) buf[pos++]=name[i];
    buf[pos++]='\n';
    // tier=...
    const char* k2 = "tier=";
    for (uint32_t i=0;k2[i]&&pos<max-1;i++) buf[pos++]=k2[i];
    buf[pos++] = (g_profile.tier == TIER_SOVEREIGN) ? '1' : '0';
    buf[pos++] = '\n';
    // flags=...
    const char* k3 = "flags=";
    for (uint32_t i=0;k3[i]&&pos<max-1;i++) buf[pos++]=k3[i];
    buf[pos++] = '0' + (g_profile.voice_primary    ? 1:0);
    buf[pos++] = '0' + (g_profile.motor_difficulty ? 1:0);
    buf[pos++] = '0' + (g_profile.speech_diff      ? 1:0);
    buf[pos++] = '0' + (g_profile.cognitive_diff   ? 1:0);
    buf[pos++] = '0' + (g_profile.visual_diff      ? 1:0);
    buf[pos++] = '0' + (g_profile.hearing_diff     ? 1:0);
    buf[pos++] = '0' + (g_profile.mobility_diff    ? 1:0);
    buf[pos++] = '0' + (g_profile.mental_health    ? 1:0);
    buf[pos++] = '\n';
    buf[pos] = 0;
}

static void save_profile(void) {
    char buf[512];
    profile_to_string(buf, 512);
    vfs_write("/etc/raven_profile", buf, str_len(buf));
    terminal_write("[ONBOARD] Profile saved to /etc/raven_profile\n");
}

static bool load_profile_from_disk(void) {
    char buf[512];
    int32_t n = vfs_read("/etc/raven_profile", buf, 511);
    if (n <= 0) return false;
    buf[n] = 0;

    // Parse name=
    for (uint32_t i = 0; buf[i]; i++) {
        if (buf[i]=='n' && buf[i+1]=='a' && buf[i+2]=='m' &&
            buf[i+3]=='e' && buf[i+4]=='=') {
            uint32_t j=0, k=i+5;
            while (buf[k] && buf[k]!='\n' && j<PROFILE_NAME_LEN-1)
                g_profile.name[j++]=buf[k++];
            g_profile.name[j]=0;
        }
        if (buf[i]=='t' && buf[i+1]=='i' && buf[i+2]=='e' &&
            buf[i+3]=='r' && buf[i+4]=='=') {
            g_profile.tier = (buf[i+5]=='1') ? TIER_SOVEREIGN : TIER_STANDARD;
        }
        if (buf[i]=='f' && buf[i+1]=='l' && buf[i+2]=='a' &&
            buf[i+3]=='g' && buf[i+4]=='s' && buf[i+5]=='=') {
            uint32_t k=i+6;
            g_profile.voice_primary    = buf[k++]=='1';
            g_profile.motor_difficulty = buf[k++]=='1';
            g_profile.speech_diff      = buf[k++]=='1';
            g_profile.cognitive_diff   = buf[k++]=='1';
            g_profile.visual_diff      = buf[k++]=='1';
            g_profile.hearing_diff     = buf[k++]=='1';
            g_profile.mobility_diff    = buf[k++]=='1';
            g_profile.mental_health    = buf[k++]=='1';
        }
    }
    return true;
}

// ── Main onboarding flow ──────────────────────────────────────────────────────
void onboarding_run(void) {
    terminal_write("[ONBOARD] Starting first-boot onboarding...\n");

    char name_buf[PROFILE_NAME_LEN] = {0};
    uint32_t name_len = 0;
    uint32_t use_sel  = 3;  // Default: General
    uint32_t q_cursor = 0;
    uint32_t screen   = 0;  // 0=welcome, 1=basic, 2=access, 3=trust

    screen_welcome();

    while (1) {
        uint8_t key = keyboard_read_scancode();
        if (!key) continue;

        char ch = keyboard_scancode_to_char(key);

        if (screen == 0) {
            // Any key / Enter advances
            if (key == KEY_ENTER || ch) {
                screen = 1;
                screen_basic_profile(name_buf, use_sel);
            }
        }
        else if (screen == 1) {
            // Name input
            if (ch >= 0x20 && ch < 0x7F && name_len < PROFILE_NAME_LEN-1) {
                name_buf[name_len++] = ch;
                name_buf[name_len]   = 0;
            } else if (key == KEY_BACKSPACE && name_len > 0) {
                name_buf[--name_len] = 0;
            } else if (key == KEY_LEFT  && use_sel > 0) use_sel--;
            else if (key == KEY_RIGHT && use_sel < 4) use_sel++;
            else if (key == KEY_ENTER) {
                str_copy(g_profile.name, name_buf, PROFILE_NAME_LEN);
                g_profile.primary_use = (primary_use_t)use_sel;
                screen = 2;
                screen_accessibility(q_cursor);
                continue;
            }
            screen_basic_profile(name_buf, use_sel);
        }
        else if (screen == 2) {
            if (key == KEY_UP   && q_cursor > 0) q_cursor--;
            else if (key == KEY_DOWN && q_cursor < ONBOARD_Q_COUNT-1) q_cursor++;
            else if (ch == 'y' || ch == 'Y') g_questions[q_cursor].answer = true;
            else if (ch == 'n' || ch == 'N') g_questions[q_cursor].answer = false;
            else if (key == KEY_ENTER) {
                // Save answers to profile
                g_profile.voice_primary    = g_questions[0].answer;
                g_profile.motor_difficulty = g_questions[1].answer;
                g_profile.speech_diff      = g_questions[2].answer;
                g_profile.cognitive_diff   = g_questions[3].answer;
                g_profile.visual_diff      = g_questions[4].answer;
                g_profile.hearing_diff     = g_questions[5].answer;
                g_profile.mobility_diff    = g_questions[6].answer;
                g_profile.mental_health    = g_questions[7].answer;
                g_profile.tier = calculate_tier();
                screen = 3;
                screen_trust(g_profile.tier);
                continue;
            }
            screen_accessibility(q_cursor);
        }
        else if (screen == 3) {
            if (key == KEY_ENTER || ch) {
                // Save and exit
                save_profile();
                g_profile_loaded = true;
                terminal_write("[ONBOARD] Onboarding complete. Welcome, ");
                terminal_write(g_profile.name);
                terminal_write(".\n");
                if (g_profile.tier == TIER_SOVEREIGN)
                    terminal_write("[ONBOARD] Tier: SOVEREIGN — All accessibility features active.\n");
                else
                    terminal_write("[ONBOARD] Tier: STANDARD — Full desktop experience.\n");
                return;
            }
        }
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
bool onboarding_needed(void) {
    if (g_profile_loaded) return false;
    // Try loading from disk
    if (load_profile_from_disk()) {
        g_profile_loaded = true;
        return false;
    }
    return true;
}

raven_user_profile_t* onboarding_get_profile(void) {
    return &g_profile;
}

bool profile_is_sovereign(void) {
    return g_profile.tier == TIER_SOVEREIGN;
}

bool profile_needs_voice(void) {
    return g_profile.voice_primary || g_profile.motor_difficulty ||
           g_profile.mobility_diff;
}

bool profile_needs_dysarthria(void) {
    return g_profile.speech_diff;
}

bool profile_needs_simplified_ui(void) {
    return g_profile.cognitive_diff;
}

bool profile_needs_high_contrast(void) {
    return g_profile.visual_diff;
}

bool profile_needs_calm_mode(void) {
    return g_profile.mental_health;
}

void onboarding_update_profile(void) {
    // Reset and re-run onboarding
    g_profile_loaded = false;
    for (uint32_t i = 0; i < ONBOARD_Q_COUNT; i++)
        g_questions[i].answer = false;
    onboarding_run();
}
