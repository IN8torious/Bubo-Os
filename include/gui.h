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

#ifndef RAVEN_GUI_H
#define RAVEN_GUI_H

#include <stdint.h>
#include <stdbool.h>

// ── Constants ─────────────────────────────────────────────────────────────────
#define GUI_MAX_WINDOWS    16
#define GUI_MAX_BUTTONS    32
#define GUI_MAX_TITLE      64
#define GUI_TITLEBAR_HEIGHT 26
#define GUI_TASKBAR_HEIGHT  32
#define GUI_SCROLLBAR_W     10

// ── Window flags ──────────────────────────────────────────────────────────────
#define GUI_FLAG_SCROLLABLE  (1 << 0)
#define GUI_FLAG_PINNED      (1 << 1)
#define GUI_FLAG_BORDERLESS  (1 << 2)
#define GUI_FLAG_MODAL       (1 << 3)

// ── Button states ─────────────────────────────────────────────────────────────
typedef enum {
    BTN_NORMAL = 0,
    BTN_HOVER,
    BTN_PRESSED,
    BTN_DISABLED
} btn_state_t;

// ── Button ────────────────────────────────────────────────────────────────────
typedef struct {
    int32_t     x, y, w, h;
    char        label[GUI_MAX_TITLE];
    btn_state_t state;
    bool        visible;
    void        (*on_click)(void);
} raven_button_t;

// ── Window ────────────────────────────────────────────────────────────────────
typedef struct {
    bool            active;
    bool            visible;
    bool            minimized;
    int32_t         x, y, w, h;
    char            title[GUI_MAX_TITLE];
    uint32_t        flags;
    int32_t         scroll_y;
    raven_button_t  buttons[GUI_MAX_BUTTONS];
    int             btn_count;
} raven_window_t;

// ── API ───────────────────────────────────────────────────────────────────────
void            gui_init(void);
void            gui_draw_taskbar(void);
int             gui_create_window(const char* title, int32_t x, int32_t y,
                                  int32_t w, int32_t h, uint32_t flags);
void            gui_draw_window(int win_id);
int             gui_add_button(int win_id, const char* label,
                               int32_t x, int32_t y, int32_t w, int32_t h);
void            gui_draw_button(int win_id, int btn_id);
void            gui_draw_panel(int32_t x, int32_t y, int32_t w, int32_t h,
                               const char* header);
void            gui_draw_progress(int32_t x, int32_t y, int32_t w, int32_t h,
                                  float pct, uint32_t color);
void            gui_draw_separator(int32_t x, int32_t y, int32_t w);
void            gui_draw_corvus_dashboard(void);
void            gui_render_all(void);
void            gui_focus_window(int win_id);
void            gui_close_window(int win_id);
raven_window_t* gui_get_window(int win_id);

#endif // RAVEN_GUI_H
