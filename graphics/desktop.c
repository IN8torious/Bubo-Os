// =============================================================================
// Raven AOS v0.7 — Desktop Shell
// Full window manager, taskbar, app launcher, wallpaper, cursor
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
// =============================================================================
#include "desktop.h"
#include "framebuffer.h"
#include "font.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Akatsuki color palette ────────────────────────────────────────────────────
#define COL_VOID        0xFF0A0A0A
#define COL_CRIMSON     0xFFCC0000
#define COL_BLOOD       0xFF8B0000
#define COL_ASH         0xFF1A1A1A
#define COL_STEEL       0xFF2A2A2A
#define COL_CLOUD       0xFFE0E0E0
#define COL_GOLD        0xFFFFD700
#define COL_CORVUS      0xFF4A0080
#define COL_LANDON      0xFF0066CC
#define COL_GREEN       0xFF00CC44
#define COL_TRANSPARENT 0x00000000

// ── Desktop state ─────────────────────────────────────────────────────────────
static desktop_state_t g_desktop;
static desktop_win_t   g_windows[DESKTOP_MAX_WINDOWS];
static app_icon_t      g_icons[DESKTOP_MAX_ICONS];
static uint32_t        g_cursor_x = 400;
static uint32_t        g_cursor_y = 300;
static bool            g_desktop_ready = false;

// ── Forward declarations ──────────────────────────────────────────────────────
static void desktop_draw_wallpaper(void);
static void desktop_draw_taskbar(void);
static void desktop_draw_icons(void);
static void desktop_draw_windows(void);
static void desktop_draw_cursor(void);
static void desktop_draw_corvus_bar(void);
static void desktop_draw_accessibility_bar(void);
static void desktop_draw_clock(void);

// ── String helpers ────────────────────────────────────────────────────────────
static void ds_strcpy(char* d, const char* s, int m) {
    int i=0; while(s[i]&&i<m-1){d[i]=s[i];i++;} d[i]=0;
}
static void ds_itoa(uint32_t n, char* buf) {
    if (n == 0) { buf[0]='0'; buf[1]=0; return; }
    char tmp[12]; int i=0;
    while (n) { tmp[i++]='0'+(n%10); n/=10; }
    int j=0; while(i>0) buf[j++]=tmp[--i]; buf[j]=0;
}

// ── Initialize the desktop ────────────────────────────────────────────────────
void desktop_init(void) {
    terminal_write("[DESKTOP] Initializing Raven Desktop Shell v0.7...\n");

    for (int i = 0; i < DESKTOP_MAX_WINDOWS; i++) {
        g_windows[i].active    = false;
        g_windows[i].visible   = false;
        g_windows[i].minimized = false;
    }
    for (int i = 0; i < DESKTOP_MAX_ICONS; i++) {
        g_icons[i].active = false;
    }

    g_desktop.window_count   = 0;
    g_desktop.icon_count     = 0;
    g_desktop.focused_window = -1;
    g_desktop.taskbar_height = TASKBAR_HEIGHT;
    g_desktop.corvus_bar_h   = CORVUS_BAR_HEIGHT;
    g_desktop.accessibility  = false;
    g_desktop.tick           = 0;

    desktop_register_icon("Terminal", 0x01, 20,  30);
    desktop_register_icon("Files",    0x02, 20, 100);
    desktop_register_icon("Settings", 0x03, 20, 170);
    desktop_register_icon("CORVUS",   0x04, 20, 240);
    desktop_register_icon("Tasks",    0x05, 20, 310);
    desktop_register_icon("Race",     0x06, 20, 380);
    desktop_register_icon("Landon",   0x07, 20, 450);

    g_desktop_ready = true;
    terminal_write("[DESKTOP] Desktop shell ready — NO MAS DISADVANTAGED\n");
}

// ── Register an app icon ──────────────────────────────────────────────────────
int32_t desktop_register_icon(const char* name, uint32_t app_id, uint32_t x, uint32_t y) {
    for (int i = 0; i < DESKTOP_MAX_ICONS; i++) {
        if (!g_icons[i].active) {
            ds_strcpy(g_icons[i].name, name, 32);
            g_icons[i].app_id = app_id;
            g_icons[i].x      = x;
            g_icons[i].y      = y;
            g_icons[i].w      = ICON_SIZE;
            g_icons[i].h      = ICON_SIZE;
            g_icons[i].active = true;
            g_desktop.icon_count++;
            return i;
        }
    }
    return -1;
}

// ── Open a new window ─────────────────────────────────────────────────────────
int32_t desktop_open_window(const char* title, uint32_t x, uint32_t y,
                             uint32_t w, uint32_t h, uint32_t app_id) {
    for (int i = 0; i < DESKTOP_MAX_WINDOWS; i++) {
        if (!g_windows[i].active) {
            ds_strcpy(g_windows[i].title, title, 64);
            g_windows[i].x         = x;
            g_windows[i].y         = y;
            g_windows[i].w         = w;
            g_windows[i].h         = h;
            g_windows[i].app_id    = app_id;
            g_windows[i].active    = true;
            g_windows[i].visible   = true;
            g_windows[i].minimized = false;
            g_windows[i].focused   = true;
            g_desktop.focused_window = i;
            g_desktop.window_count++;
            terminal_write("[DESKTOP] Window opened: ");
            terminal_write(title);
            terminal_write("\n");
            return i;
        }
    }
    return -1;
}

// ── Close a window ────────────────────────────────────────────────────────────
void desktop_close_window(int32_t wid) {
    if (wid < 0 || wid >= DESKTOP_MAX_WINDOWS) return;
    g_windows[wid].active  = false;
    g_windows[wid].visible = false;
    if (g_desktop.window_count > 0) g_desktop.window_count--;
    if (g_desktop.focused_window == wid) g_desktop.focused_window = -1;
}

// ── Draw the full desktop frame ───────────────────────────────────────────────
void desktop_render(void) {
    if (!g_desktop_ready) return;
    desktop_draw_wallpaper();
    desktop_draw_icons();
    desktop_draw_windows();
    desktop_draw_taskbar();
    desktop_draw_corvus_bar();
    if (g_desktop.accessibility) desktop_draw_accessibility_bar();
    desktop_draw_cursor();
    g_desktop.tick++;
}

// ── Wallpaper ─────────────────────────────────────────────────────────────────
static void desktop_draw_wallpaper(void) {
    fb_info_t* info = fb_get_info();
    int32_t sw = (int32_t)info->width;
    int32_t sh = (int32_t)info->height;

    fb_fill_rect(0, 0, sw, sh, COL_VOID);

    // Akatsuki red cloud bands
    for (int32_t y = 0; y < sh; y += 120) {
        fb_fill_rect(0, y + 40, sw, 3, COL_BLOOD);
        fb_fill_rect(0, y + 44, sw, 1, COL_CRIMSON);
    }

    // Rinnegan symbol — top right corner
    int32_t cx = sw - 80, cy = 80;
    fb_draw_circle(cx, cy, 50, COL_BLOOD);
    fb_draw_circle(cx, cy, 35, COL_CRIMSON);
    fb_draw_circle(cx, cy, 20, COL_BLOOD);
    fb_draw_circle(cx, cy,  8, COL_CRIMSON);
    fb_fill_rect(cx + 24, cy - 4, 8, 8, COL_GOLD);
    fb_fill_rect(cx - 18, cy + 20, 8, 8, COL_GOLD);
    fb_fill_rect(cx - 18, cy - 28, 8, 8, COL_GOLD);

    // Watermarks
    font_draw_string(sw - 280, sh - TASKBAR_HEIGHT - 20,
                     "NO MAS DISADVANTAGED", COL_BLOOD, COL_TRANSPARENT, true);
    font_draw_string(4, sh - TASKBAR_HEIGHT - 18,
                     "Raven AOS v0.7 | CORVUS MAS", COL_BLOOD, COL_TRANSPARENT, true);
}

// ── Taskbar ───────────────────────────────────────────────────────────────────
static void desktop_draw_taskbar(void) {
    fb_info_t* info = fb_get_info();
    int32_t sw = (int32_t)info->width;
    int32_t sh = (int32_t)info->height;
    int32_t ty = sh - TASKBAR_HEIGHT;

    fb_fill_rect(0, ty, sw, TASKBAR_HEIGHT, COL_STEEL);
    fb_fill_rect(0, ty, sw, 2, COL_CRIMSON);

    // App launcher
    fb_fill_rect(4, ty + 4, 48, TASKBAR_HEIGHT - 8, COL_BLOOD);
    font_draw_string(10, ty + 12, "RAVEN", COL_CLOUD, COL_TRANSPARENT, true);

    // Open window buttons
    int32_t btn_x = 60;
    for (int i = 0; i < DESKTOP_MAX_WINDOWS; i++) {
        if (!g_windows[i].active) continue;
        uint32_t col = (g_desktop.focused_window == i) ? COL_CRIMSON : COL_ASH;
        fb_fill_rect(btn_x, ty + 4, 120, TASKBAR_HEIGHT - 8, col);
        font_draw_string(btn_x + 4, ty + 12, g_windows[i].title,
                         COL_CLOUD, COL_TRANSPARENT, true);
        btn_x += 128;
    }

    // CORVUS status dot
    fb_fill_rect(sw - 160, ty + 6, 12, 12, COL_GREEN);
    font_draw_string(sw - 144, ty + 8, "CORVUS", COL_GREEN, COL_TRANSPARENT, true);

    desktop_draw_clock();
}

// ── Clock ─────────────────────────────────────────────────────────────────────
static void desktop_draw_clock(void) {
    fb_info_t* info = fb_get_info();
    int32_t sw = (int32_t)info->width;
    int32_t sh = (int32_t)info->height;
    int32_t ty = sh - TASKBAR_HEIGHT;

    uint32_t secs  = g_desktop.tick / 100;
    uint32_t mins  = secs / 60;
    uint32_t hours = mins / 60;
    secs %= 60; mins %= 60; hours %= 24;

    char time_str[16];
    char h[4], m[4], s[4];
    ds_itoa(hours, h); ds_itoa(mins, m); ds_itoa(secs, s);
    int i=0;
    if (hours < 10) time_str[i++]='0';
    for (int j=0; h[j]; j++) time_str[i++]=h[j];
    time_str[i++]=':';
    if (mins < 10) time_str[i++]='0';
    for (int j=0; m[j]; j++) time_str[i++]=m[j];
    time_str[i++]=':';
    if (secs < 10) time_str[i++]='0';
    for (int j=0; s[j]; j++) time_str[i++]=s[j];
    time_str[i]=0;

    font_draw_string(sw - 72, ty + 8, time_str, COL_CLOUD, COL_TRANSPARENT, true);
}

// ── Desktop icons (left sidebar) ─────────────────────────────────────────────
static void desktop_draw_icons(void) {
    for (int i = 0; i < DESKTOP_MAX_ICONS; i++) {
        if (!g_icons[i].active) continue;
        int32_t x = (int32_t)g_icons[i].x;
        int32_t y = (int32_t)g_icons[i].y;

        uint32_t col = COL_ASH;
        if (g_icons[i].app_id == 0x06) col = COL_BLOOD;
        if (g_icons[i].app_id == 0x07) col = COL_LANDON;
        if (g_icons[i].app_id == 0x04) col = COL_CORVUS;

        fb_fill_rect(x, y, ICON_SIZE, ICON_SIZE, col);
        fb_draw_rect(x, y, ICON_SIZE, ICON_SIZE, 1, COL_CRIMSON);
        font_draw_string(x + 2, y + ICON_SIZE + 2, g_icons[i].name,
                         COL_CLOUD, COL_TRANSPARENT, true);
    }
}

// ── Windows ───────────────────────────────────────────────────────────────────
static void desktop_draw_windows(void) {
    for (int i = 0; i < DESKTOP_MAX_WINDOWS; i++) {
        if (!g_windows[i].active || !g_windows[i].visible ||
             g_windows[i].minimized) continue;

        int32_t x = (int32_t)g_windows[i].x;
        int32_t y = (int32_t)g_windows[i].y;
        int32_t w = (int32_t)g_windows[i].w;
        int32_t h = (int32_t)g_windows[i].h;

        fb_fill_rect(x + 4, y + 4, w, h, 0xFF050505);   // shadow
        fb_fill_rect(x, y, w, h, COL_ASH);               // body

        uint32_t title_col = (g_desktop.focused_window == i) ? COL_CRIMSON : COL_BLOOD;
        fb_fill_rect(x, y, w, WINDOW_TITLE_H, title_col);
        font_draw_string(x + 8, y + 6, g_windows[i].title,
                         COL_CLOUD, COL_TRANSPARENT, true);

        fb_fill_rect(x + w - 20, y + 4, 16, 16, 0xFFAA0000);
        font_draw_string(x + w - 16, y + 5, "X", COL_CLOUD, COL_TRANSPARENT, true);
        fb_fill_rect(x + w - 40, y + 4, 16, 16, COL_STEEL);
        font_draw_string(x + w - 36, y + 5, "_", COL_CLOUD, COL_TRANSPARENT, true);

        fb_draw_rect(x, y, w, h, 1, COL_CRIMSON);
        fb_fill_rect(x + 1, y + WINDOW_TITLE_H, w - 2, 1, COL_BLOOD);
    }
}

// ── CORVUS status bar (top of screen) ────────────────────────────────────────
static void desktop_draw_corvus_bar(void) {
    fb_info_t* info = fb_get_info();
    int32_t sw = (int32_t)info->width;

    fb_fill_rect(0, 0, sw, CORVUS_BAR_HEIGHT, 0xFF0D0D0D);
    fb_fill_rect(0, CORVUS_BAR_HEIGHT - 1, sw, 1, COL_BLOOD);

    font_draw_string(8, 4, "CORVUS MAS", COL_CRIMSON, COL_TRANSPARENT, true);
    font_draw_string(90, 4, "|", COL_BLOOD, COL_TRANSPARENT, true);

    const char* agents[] = {"CROW","HEAL","SEC","MEM","NET","PLAN","LEARN","PHYS","VOICE","DRIVE"};
    int32_t ax = 100;
    for (int i = 0; i < 10; i++) {
        fb_fill_rect(ax, 3, 6, 6, COL_GREEN);
        font_draw_string(ax + 8, 4, agents[i], COL_CLOUD, COL_TRANSPARENT, true);
        ax += 52;
    }

    font_draw_string(sw - 200, 4, "NO MAS DISADVANTAGED", COL_GOLD, COL_TRANSPARENT, true);
}

// ── Accessibility bar ─────────────────────────────────────────────────────────
static void desktop_draw_accessibility_bar(void) {
    fb_info_t* info = fb_get_info();
    int32_t sw = (int32_t)info->width;
    int32_t sh = (int32_t)info->height;
    int32_t ay = sh - TASKBAR_HEIGHT - ACCESS_BAR_HEIGHT;

    fb_fill_rect(0, ay, sw, ACCESS_BAR_HEIGHT, 0xFF001133);
    fb_fill_rect(0, ay, sw, 2, COL_LANDON);

    font_draw_string(8, ay + 8, "LANDON PANKUCH", COL_LANDON, COL_TRANSPARENT, true);
    font_draw_string(160, ay + 8, "|", COL_STEEL, COL_TRANSPARENT, true);
    font_draw_string(172, ay + 8, "Say: FASTER  BRAKE  DRIFT  NITRO  OVERTAKE  STATUS",
                     COL_CLOUD, COL_TRANSPARENT, true);
    font_draw_string(sw - 180, ay + 8, "CORVUS IS DRIVING FOR YOU",
                     COL_GOLD, COL_TRANSPARENT, true);
}

// ── Mouse cursor ──────────────────────────────────────────────────────────────
static void desktop_draw_cursor(void) {
    int32_t x = (int32_t)g_cursor_x;
    int32_t y = (int32_t)g_cursor_y;

    fb_fill_rect(x,     y,     2, 14, COL_CLOUD);
    fb_fill_rect(x,     y,     10, 2, COL_CLOUD);
    fb_fill_rect(x + 1, y + 1, 1, 12, COL_CRIMSON);
    fb_fill_rect(x + 1, y + 1, 8,  1, COL_CRIMSON);
    for (int i = 2; i < 10; i++) {
        fb_fill_rect(x + i, y + i, 2, 2, COL_CLOUD);
        fb_fill_rect(x + i + 1, y + i + 1, 1, 1, COL_CRIMSON);
    }
}

// ── Update cursor position ────────────────────────────────────────────────────
void desktop_move_cursor(int32_t dx, int32_t dy) {
    fb_info_t* info = fb_get_info();
    int32_t nx = (int32_t)g_cursor_x + dx;
    int32_t ny = (int32_t)g_cursor_y + dy;
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if ((uint32_t)nx >= info->width)  nx = (int32_t)info->width  - 1;
    if ((uint32_t)ny >= info->height) ny = (int32_t)info->height - 1;
    g_cursor_x = (uint32_t)nx;
    g_cursor_y = (uint32_t)ny;
}

// ── Handle keyboard input ─────────────────────────────────────────────────────
void desktop_handle_key(char key) {
    (void)key;
}

// ── Toggle Landon accessibility mode ─────────────────────────────────────────
void desktop_toggle_accessibility(void) {
    g_desktop.accessibility = !g_desktop.accessibility;
    if (g_desktop.accessibility)
        terminal_write("[DESKTOP] Accessibility mode ON — Landon Pankuch\n");
    else
        terminal_write("[DESKTOP] Accessibility mode OFF\n");
}

// ── Get desktop state ─────────────────────────────────────────────────────────
desktop_state_t* desktop_get_state(void) {
    return &g_desktop;
}

// ── Handle click at (x, y) ────────────────────────────────────────────────────
void desktop_handle_click(uint32_t x, uint32_t y) {
    for (int i = 0; i < DESKTOP_MAX_ICONS; i++) {
        if (!g_icons[i].active) continue;
        if (x >= g_icons[i].x && x < g_icons[i].x + ICON_SIZE &&
            y >= g_icons[i].y && y < g_icons[i].y + ICON_SIZE) {
            desktop_launch_app(g_icons[i].app_id);
            return;
        }
    }
    for (int i = 0; i < DESKTOP_MAX_WINDOWS; i++) {
        if (!g_windows[i].active || !g_windows[i].visible) continue;
        uint32_t wx = g_windows[i].x, wy = g_windows[i].y;
        uint32_t ww = g_windows[i].w;
        if (x >= wx && x < wx + ww && y >= wy && y < wy + WINDOW_TITLE_H) {
            if (x >= wx + ww - 20 && x < wx + ww - 4) {
                desktop_close_window(i);
                return;
            }
            g_desktop.focused_window = i;
            return;
        }
    }
}

// ── Launch an app by ID ───────────────────────────────────────────────────────
void desktop_launch_app(uint32_t app_id) {
    switch (app_id) {
    case 0x01: desktop_open_window("Terminal",                100, 60, 500, 350, app_id); break;
    case 0x02: desktop_open_window("File Manager",            150, 80, 480, 320, app_id); break;
    case 0x03: desktop_open_window("Settings",                200,100, 400, 300, app_id); break;
    case 0x04: desktop_open_window("CORVUS Dashboard",         80, 50, 560, 400, app_id); break;
    case 0x05: desktop_open_window("Task Manager",            180, 90, 440, 280, app_id); break;
    case 0x06: desktop_open_window("Landon's Race — Demon 170", 60, 50, 600, 420, app_id); break;
    case 0x07:
        desktop_toggle_accessibility();
        desktop_open_window("Landon — Voice Control", 50, 60, 580, 380, app_id);
        break;
    default:
        terminal_write("[DESKTOP] Unknown app ID\n");
        break;
    }
}
