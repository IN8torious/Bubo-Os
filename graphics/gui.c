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
// Raven OS — GUI System
// Full windowed desktop environment rendered to the framebuffer
// Inspired by: UECanvasGui, modio-ue-component-ui, UE-UI-Challenges
// CORVUS drives this UI — every window, panel, and button is agent-aware
// =============================================================================

#include "gui.h"
#include "../include/framebuffer.h"
#include "../include/font.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── GUI State ─────────────────────────────────────────────────────────────────
static raven_window_t  g_windows[GUI_MAX_WINDOWS];
static int             g_window_count  = 0;
static int             g_focused_win   = -1;
static bool            g_gui_ready     = false;

// ── Akatsuki color palette for GUI ───────────────────────────────────────────
#define GUI_COL_BG          0x0A0A0A   // Near-black desktop
#define GUI_COL_WIN_BG      0x111111   // Window background
#define GUI_COL_WIN_TITLE   0x1A0000   // Dark crimson title bar
#define GUI_COL_WIN_BORDER  0x8B0000   // Crimson border
#define GUI_COL_WIN_BORDER_FOCUS 0xCC0000  // Bright crimson when focused
#define GUI_COL_TITLE_TEXT  0xFF4444   // Red title text
#define GUI_COL_BODY_TEXT   0xDDDDDD   // Light grey body text
#define GUI_COL_DIM_TEXT    0x888888   // Dimmed text
#define GUI_COL_BTN_BG      0x1A0000   // Button background
#define GUI_COL_BTN_HOVER   0x330000   // Button hover
#define GUI_COL_BTN_PRESS   0x550000   // Button pressed
#define GUI_COL_BTN_TEXT    0xFF6666   // Button text
#define GUI_COL_PANEL_BG    0x0D0D0D   // Panel background
#define GUI_COL_PANEL_HDR   0x1A0000   // Panel header
#define GUI_COL_SCROLLBAR   0x330000   // Scrollbar track
#define GUI_COL_SCROLLTHUMB 0x880000   // Scrollbar thumb
#define GUI_COL_PROGRESS_BG 0x1A0000   // Progress bar background
#define GUI_COL_PROGRESS_FG 0xCC0000   // Progress bar fill
#define GUI_COL_AGENT_OK    0x00CC44   // Agent healthy (green)
#define GUI_COL_AGENT_WARN  0xFFAA00   // Agent warning (orange)
#define GUI_COL_AGENT_ERR   0xFF2222   // Agent error (red)
#define GUI_COL_SEPARATOR   0x330000   // Separator line

// ── String helpers ────────────────────────────────────────────────────────────
static int raven_strlen(const char* s) {
    int n = 0; while (s && s[n]) n++; return n;
}
static void raven_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src && src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

// ── Initialize GUI ────────────────────────────────────────────────────────────
void gui_init(void) {
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) {
        g_windows[i].active = false;
    }
    g_window_count = 0;
    g_focused_win  = -1;
    g_gui_ready    = true;

    // Paint the desktop
    fb_clear(GUI_COL_BG);

    // Draw the taskbar at the bottom
    gui_draw_taskbar();
}

// ── Draw the desktop taskbar ──────────────────────────────────────────────────
void gui_draw_taskbar(void) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t tb_y = (int32_t)fb->height - GUI_TASKBAR_HEIGHT;
    int32_t tb_w = (int32_t)fb->width;

    // Taskbar background
    fb_fill_rect(0, tb_y, tb_w, GUI_TASKBAR_HEIGHT, 0x0D0000);
    // Top border — crimson line
    fb_draw_hline(0, tb_y, tb_w, GUI_COL_WIN_BORDER_FOCUS);

    // CORVUS logo / start button
    fb_fill_rect(4, tb_y + 4, 80, GUI_TASKBAR_HEIGHT - 8, GUI_COL_WIN_TITLE);
    fb_draw_rect(4, tb_y + 4, 80, GUI_TASKBAR_HEIGHT - 8, 1, GUI_COL_WIN_BORDER);
    font_draw_string(12, tb_y + 10, "CORVUS", GUI_COL_WIN_BORDER_FOCUS, 0, true);

    // Clock area (right side)
    font_draw_string(tb_w - 80, tb_y + 10, "00:00:00", GUI_COL_DIM_TEXT, 0, true);

    // Agent status dots
    int32_t dot_x = tb_w - 180;
    for (int i = 0; i < 10; i++) {
        fb_fill_circle(dot_x + i * 14, tb_y + GUI_TASKBAR_HEIGHT/2, 4, GUI_COL_AGENT_OK);
    }
}

// ── Create a window ───────────────────────────────────────────────────────────
int gui_create_window(const char* title, int32_t x, int32_t y,
                      int32_t w, int32_t h, uint32_t flags) {
    if (!g_gui_ready || g_window_count >= GUI_MAX_WINDOWS) return -1;

    // Find free slot
    int slot = -1;
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) {
        if (!g_windows[i].active) { slot = i; break; }
    }
    if (slot < 0) return -1;

    raven_window_t* win = &g_windows[slot];
    win->active     = true;
    win->visible    = true;
    win->minimized  = false;
    win->x = x; win->y = y; win->w = w; win->h = h;
    win->flags      = flags;
    win->scroll_y   = 0;
    win->btn_count  = 0;
    raven_strcpy(win->title, title, GUI_MAX_TITLE);

    g_window_count++;
    g_focused_win = slot;

    gui_draw_window(slot);
    return slot;
}

// ── Draw a single window ──────────────────────────────────────────────────────
void gui_draw_window(int win_id) {
    if (win_id < 0 || win_id >= GUI_MAX_WINDOWS) return;
    raven_window_t* win = &g_windows[win_id];
    if (!win->active || !win->visible || win->minimized) return;

    bool focused = (win_id == g_focused_win);
    uint32_t border_col = focused ? GUI_COL_WIN_BORDER_FOCUS : GUI_COL_WIN_BORDER;

    // ── Window shadow ─────────────────────────────────────────────────────────
    fb_fill_rect(win->x + 4, win->y + 4, win->w, win->h, 0x050505);

    // ── Window body ───────────────────────────────────────────────────────────
    fb_fill_rect(win->x, win->y, win->w, win->h, GUI_COL_WIN_BG);

    // ── Title bar ─────────────────────────────────────────────────────────────
    fb_fill_rect(win->x, win->y, win->w, GUI_TITLEBAR_HEIGHT, GUI_COL_WIN_TITLE);

    // Title bar gradient effect (3 lines)
    fb_draw_hline(win->x, win->y,     win->w, 0x2A0000);
    fb_draw_hline(win->x, win->y + 1, win->w, 0x200000);

    // Title text
    font_draw_string(win->x + 10, win->y + (GUI_TITLEBAR_HEIGHT - FONT_HEIGHT) / 2,
                     win->title, GUI_COL_TITLE_TEXT, 0, true);

    // ── Window controls (close, minimize) ─────────────────────────────────────
    // Close button [X]
    int32_t btn_x = win->x + win->w - 24;
    int32_t btn_y = win->y + 4;
    fb_fill_rect(btn_x, btn_y, 18, 18, 0x440000);
    fb_draw_rect(btn_x, btn_y, 18, 18, 1, border_col);
    font_draw_char(btn_x + 5, btn_y + 1, 'X', 0xFF4444, 0, true);

    // Minimize button [_]
    btn_x -= 22;
    fb_fill_rect(btn_x, btn_y, 18, 18, 0x1A1A00);
    fb_draw_rect(btn_x, btn_y, 18, 18, 1, 0x666600);
    font_draw_char(btn_x + 5, btn_y + 1, '_', 0xFFFF44, 0, true);

    // ── Border ────────────────────────────────────────────────────────────────
    fb_draw_rect(win->x, win->y, win->w, win->h, 1, border_col);

    // Focused: double border
    if (focused) {
        fb_draw_rect(win->x - 1, win->y - 1, win->w + 2, win->h + 2, 1, 0x440000);
    }

    // ── Scrollbar (if content overflows) ─────────────────────────────────────
    if (win->flags & GUI_FLAG_SCROLLABLE) {
        int32_t sb_x = win->x + win->w - GUI_SCROLLBAR_W - 1;
        int32_t sb_y = win->y + GUI_TITLEBAR_HEIGHT;
        int32_t sb_h = win->h - GUI_TITLEBAR_HEIGHT - 1;
        fb_fill_rect(sb_x, sb_y, GUI_SCROLLBAR_W, sb_h, GUI_COL_SCROLLBAR);
        // Thumb (simplified — fixed 30% height)
        fb_fill_rect(sb_x + 1, sb_y + 2, GUI_SCROLLBAR_W - 2, sb_h / 3, GUI_COL_SCROLLTHUMB);
    }

    // ── Draw buttons ──────────────────────────────────────────────────────────
    for (int i = 0; i < win->btn_count; i++) {
        gui_draw_button(win_id, i);
    }
}

// ── Add a button to a window ──────────────────────────────────────────────────
int gui_add_button(int win_id, const char* label,
                   int32_t x, int32_t y, int32_t w, int32_t h) {
    if (win_id < 0 || win_id >= GUI_MAX_WINDOWS) return -1;
    raven_window_t* win = &g_windows[win_id];
    if (!win->active || win->btn_count >= GUI_MAX_BUTTONS) return -1;

    int idx = win->btn_count++;
    raven_button_t* btn = &win->buttons[idx];
    btn->x = win->x + x;
    btn->y = win->y + y;
    btn->w = w; btn->h = h;
    btn->state = BTN_NORMAL;
    btn->visible = true;
    raven_strcpy(btn->label, label, GUI_MAX_TITLE);
    btn->on_click = NULL;

    gui_draw_button(win_id, idx);
    return idx;
}

// ── Draw a button ─────────────────────────────────────────────────────────────
void gui_draw_button(int win_id, int btn_id) {
    if (win_id < 0 || win_id >= GUI_MAX_WINDOWS) return;
    raven_window_t* win = &g_windows[win_id];
    if (btn_id < 0 || btn_id >= win->btn_count) return;
    raven_button_t* btn = &win->buttons[btn_id];
    if (!btn->visible) return;

    uint32_t bg = GUI_COL_BTN_BG;
    if (btn->state == BTN_HOVER)   bg = GUI_COL_BTN_HOVER;
    if (btn->state == BTN_PRESSED) bg = GUI_COL_BTN_PRESS;

    fb_fill_rect(btn->x, btn->y, btn->w, btn->h, bg);
    fb_draw_rect(btn->x, btn->y, btn->w, btn->h, 1, GUI_COL_WIN_BORDER);

    // Highlight top edge
    fb_draw_hline(btn->x + 1, btn->y + 1, btn->w - 2, 0x330000);

    // Center label
    int32_t lw = font_measure_width(btn->label);
    int32_t lx = btn->x + (btn->w - lw) / 2;
    int32_t ly = btn->y + (btn->h - FONT_HEIGHT) / 2;
    font_draw_string(lx, ly, btn->label, GUI_COL_BTN_TEXT, 0, true);
}

// ── Draw a panel inside a window ──────────────────────────────────────────────
void gui_draw_panel(int32_t x, int32_t y, int32_t w, int32_t h,
                    const char* header) {
    fb_fill_rect(x, y, w, h, GUI_COL_PANEL_BG);
    if (header && raven_strlen(header) > 0) {
        fb_fill_rect(x, y, w, GUI_TITLEBAR_HEIGHT - 4, GUI_COL_PANEL_HDR);
        font_draw_string(x + 6, y + 3, header, GUI_COL_TITLE_TEXT, 0, true);
        fb_draw_hline(x, y + GUI_TITLEBAR_HEIGHT - 4, w, GUI_COL_SEPARATOR);
    }
    fb_draw_rect(x, y, w, h, 1, GUI_COL_WIN_BORDER);
}

// ── Draw a progress bar ───────────────────────────────────────────────────────
void gui_draw_progress(int32_t x, int32_t y, int32_t w, int32_t h,
                       float pct, uint32_t color) {
    fb_fill_rect(x, y, w, h, GUI_COL_PROGRESS_BG);
    fb_draw_rect(x, y, w, h, 1, GUI_COL_WIN_BORDER);
    int32_t fill = (int32_t)(w * pct);
    if (fill > 2) fb_fill_rect(x + 1, y + 1, fill - 2, h - 2, color);
}

// ── Draw a separator line ─────────────────────────────────────────────────────
void gui_draw_separator(int32_t x, int32_t y, int32_t w) {
    fb_draw_hline(x, y, w, GUI_COL_SEPARATOR);
    fb_draw_hline(x, y + 1, w, 0x050505);
}

// ── CORVUS Dashboard Window ───────────────────────────────────────────────────
// The main CORVUS control panel — shows all 10 agents, system stats, shell
void gui_draw_corvus_dashboard(void) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t dash_w = 420;
    int32_t dash_h = (int32_t)fb->height - GUI_TASKBAR_HEIGHT - 20;
    int32_t dash_x = (int32_t)fb->width - dash_w - 10;
    int32_t dash_y = 10;

    // Create or redraw dashboard window
    static int dash_win = -1;
    if (dash_win < 0) {
        dash_win = gui_create_window("CORVUS — Agentic OS Brain",
                                    dash_x, dash_y, dash_w, dash_h,
                                    GUI_FLAG_SCROLLABLE | GUI_FLAG_PINNED);
    }

    raven_window_t* win = &g_windows[dash_win];
    int32_t cx = win->x + 8;
    int32_t cy = win->y + GUI_TITLEBAR_HEIGHT + 8;

    // ── Section: System Status ────────────────────────────────────────────────
    gui_draw_panel(cx, cy, dash_w - 16, 70, "System Status");
    cy += GUI_TITLEBAR_HEIGHT;

    // CPU bar
    font_draw_string(cx + 4, cy + 2, "CPU", GUI_COL_DIM_TEXT, 0, true);
    gui_draw_progress(cx + 40, cy + 2, dash_w - 70, 12, 0.23f, GUI_COL_AGENT_OK);
    cy += 18;

    // Memory bar
    font_draw_string(cx + 4, cy + 2, "MEM", GUI_COL_DIM_TEXT, 0, true);
    gui_draw_progress(cx + 40, cy + 2, dash_w - 70, 12, 0.61f, GUI_COL_AGENT_WARN);
    cy += 18;

    // Kernel ticks
    font_draw_string(cx + 4, cy + 2, "TICK", GUI_COL_DIM_TEXT, 0, true);
    font_draw_string(cx + 40, cy + 2, "100Hz  uptime: 00:00:42", GUI_COL_BODY_TEXT, 0, true);
    cy += 24;

    gui_draw_separator(cx, cy, dash_w - 16);
    cy += 8;

    // ── Section: CORVUS Agents ────────────────────────────────────────────────
    gui_draw_panel(cx, cy, dash_w - 16, 240, "Active Agents");
    cy += GUI_TITLEBAR_HEIGHT;

    static const char* agent_names[10] = {
        "Crow",     "Healer",   "Security", "Memory",
        "Scheduler","Network",  "FileSystem","Display",
        "Input",    "Entropy"
    };
    static const char* agent_states[10] = {
        "WATCHING", "IDLE",     "SCANNING", "LEARNING",
        "RUNNING",  "STANDBY",  "MOUNTED",  "RENDERING",
        "LISTENING","SAMPLING"
    };
    static const float agent_load[10] = {
        0.12f, 0.05f, 0.34f, 0.78f,
        0.91f, 0.08f, 0.45f, 0.67f,
        0.22f, 0.15f
    };
    static const uint32_t agent_colors[10] = {
        0x00CC44, 0x00CC44, 0xFFAA00, 0x00AAFF,
        0x00CC44, 0x888888, 0x00CC44, 0x00AAFF,
        0x00CC44, 0x888888
    };

    for (int i = 0; i < 10; i++) {
        // Status dot
        fb_fill_circle(cx + 8, cy + 8, 5, agent_colors[i]);

        // Agent name
        font_draw_string(cx + 18, cy, agent_names[i], GUI_COL_BODY_TEXT, 0, true);

        // State
        font_draw_string(cx + 90, cy, agent_states[i], GUI_COL_DIM_TEXT, 0, true);

        // Load bar
        gui_draw_progress(cx + 180, cy + 2, dash_w - 200, 10, agent_load[i], agent_colors[i]);

        cy += 20;

        // Separator every 5 agents
        if (i == 4) {
            gui_draw_separator(cx, cy, dash_w - 16);
            cy += 4;
        }
    }

    cy += 8;
    gui_draw_separator(cx, cy, dash_w - 16);
    cy += 8;

    // ── Section: CORVUS Shell Input ───────────────────────────────────────────
    gui_draw_panel(cx, cy, dash_w - 16, 80, "CORVUS Shell");
    cy += GUI_TITLEBAR_HEIGHT;

    // Recent output
    font_draw_string(cx + 4, cy,
        "> CORVUS initialized. 10 agents online.",
        GUI_COL_AGENT_OK, 0, true);
    cy += 16;
    font_draw_string(cx + 4, cy,
        "> Deep Flow OS v0.3 — framebuffer active.",
        GUI_COL_DIM_TEXT, 0, true);
    cy += 16;

    // Input box
    fb_fill_rect(cx + 4, cy + 2, dash_w - 24, 18, 0x050505);
    fb_draw_rect(cx + 4, cy + 2, dash_w - 24, 18, 1, GUI_COL_WIN_BORDER_FOCUS);
    font_draw_string(cx + 8, cy + 5, "> _", GUI_COL_WIN_BORDER_FOCUS, 0, true);
    cy += 28;

    gui_draw_separator(cx, cy, dash_w - 16);
    cy += 8;

    // ── Section: CORVUS Needs Model ───────────────────────────────────────────
    gui_draw_panel(cx, cy, dash_w - 16, 100, "CORVUS Needs (Maslow)");
    cy += GUI_TITLEBAR_HEIGHT;

    static const char* need_names[5] = {
        "Survival", "Safety", "Social", "Esteem", "Growth"
    };
    static const float need_vals[5] = {
        0.95f, 0.82f, 0.40f, 0.65f, 0.30f
    };
    static const uint32_t need_cols[5] = {
        0x00CC44, 0x00AAFF, 0xFFAA00, 0xCC44FF, 0xFF4444
    };

    for (int i = 0; i < 5; i++) {
        font_draw_string(cx + 4, cy + 2, need_names[i], GUI_COL_DIM_TEXT, 0, true);
        gui_draw_progress(cx + 68, cy + 2, dash_w - 88, 10, need_vals[i], need_cols[i]);
        cy += 16;
    }

    cy += 8;
    gui_draw_separator(cx, cy, dash_w - 16);
    cy += 8;

    // ── Section: GOAP Active Goal ─────────────────────────────────────────────
    gui_draw_panel(cx, cy, dash_w - 16, 50, "Active Goal (GOAP)");
    cy += GUI_TITLEBAR_HEIGHT;
    font_draw_string(cx + 4, cy,
        "Goal: MAINTAIN_SYSTEM_HEALTH", GUI_COL_WIN_BORDER_FOCUS, 0, true);
    cy += 16;
    font_draw_string(cx + 4, cy,
        "Plan: [Monitor] -> [Heal] -> [Report]", GUI_COL_DIM_TEXT, 0, true);
}

// ── Draw all windows ──────────────────────────────────────────────────────────
void gui_render_all(void) {
    if (!g_gui_ready) return;
    for (int i = 0; i < GUI_MAX_WINDOWS; i++) {
        if (g_windows[i].active && g_windows[i].visible && !g_windows[i].minimized) {
            gui_draw_window(i);
        }
    }
    gui_draw_taskbar();
}

// ── Focus a window ────────────────────────────────────────────────────────────
void gui_focus_window(int win_id) {
    if (win_id >= 0 && win_id < GUI_MAX_WINDOWS && g_windows[win_id].active) {
        g_focused_win = win_id;
        gui_draw_window(win_id);
    }
}

// ── Close a window ────────────────────────────────────────────────────────────
void gui_close_window(int win_id) {
    if (win_id >= 0 && win_id < GUI_MAX_WINDOWS) {
        g_windows[win_id].active = false;
        g_window_count--;
        if (g_focused_win == win_id) g_focused_win = -1;
    }
}

// ── Get window pointer ────────────────────────────────────────────────────────
raven_window_t* gui_get_window(int win_id) {
    if (win_id < 0 || win_id >= GUI_MAX_WINDOWS) return NULL;
    return g_windows[win_id].active ? &g_windows[win_id] : NULL;
}
