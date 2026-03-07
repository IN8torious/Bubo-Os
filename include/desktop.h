#pragma once
// =============================================================================
// Raven AOS — Desktop Shell Header
// "NO MAS DISADVANTAGED" — MAS = Multi-Agentic Systems
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

#define DESKTOP_MAX_WINDOWS  16
#define DESKTOP_MAX_ICONS    32
#define TASKBAR_HEIGHT       32
#define CORVUS_BAR_HEIGHT    18
#define ACCESS_BAR_HEIGHT    28
#define WINDOW_TITLE_H       24
#define ICON_SIZE            56

typedef struct {
    char     title[64];
    uint32_t x, y, w, h;
    uint32_t app_id;
    bool     active;
    bool     visible;
    bool     minimized;
    bool     focused;
} desktop_win_t;

typedef struct {
    char     name[32];
    uint32_t app_id;
    uint32_t x, y, w, h;
    bool     active;
} app_icon_t;

typedef struct {
    uint32_t window_count;
    uint32_t icon_count;
    int32_t  focused_window;
    uint32_t taskbar_height;
    uint32_t corvus_bar_h;
    bool     accessibility;
    uint32_t tick;
} desktop_state_t;

// Public API
void             desktop_init(void);
void             desktop_render(void);
void             desktop_handle_key(char key);
void             desktop_handle_click(uint32_t x, uint32_t y);
void             desktop_move_cursor(int32_t dx, int32_t dy);
void             desktop_launch_app(uint32_t app_id);
void             desktop_toggle_accessibility(void);
int32_t          desktop_open_window(const char* title, uint32_t x, uint32_t y,
                                      uint32_t w, uint32_t h, uint32_t app_id);
void             desktop_close_window(int32_t wid);
int32_t          desktop_register_icon(const char* name, uint32_t app_id,
                                        uint32_t x, uint32_t y);
desktop_state_t* desktop_get_state(void);
