// BUBO OS — LVGL Display Port
// Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon. Built for everyone.
//
// This file bridges LVGL v9 to the BUBO OS bare-metal framebuffer.
// LVGL renders into a back-buffer; we blit it to the VESA framebuffer
// using the existing fb_* API. No OS, no malloc, no libc.
// =============================================================================

#include "lv_port_bubo.h"
#include "framebuffer.h"
#include "../lvgl/lvgl.h"

// ── Screen dimensions ────────────────────────────────────────────────────────
#define BUBO_SCREEN_W   1024
#define BUBO_SCREEN_H   768
#define BUBO_BPP        4       // 32-bit ARGB

// ── LVGL draw buffer — 1/10th of screen, double-buffered ─────────────────────
#define BUF_ROWS        (BUBO_SCREEN_H / 10)
static lv_color_t lv_buf1[BUBO_SCREEN_W * BUF_ROWS];
static lv_color_t lv_buf2[BUBO_SCREEN_W * BUF_ROWS];

static lv_display_t *g_disp = NULL;

// ── Flush callback — LVGL calls this when a region is ready to display ───────
static void bubo_lv_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // px_map is LVGL's rendered pixels in ARGB8888 format
    // Blit each row to the VESA framebuffer
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    uint32_t *src = (uint32_t *)px_map;
    for (int32_t y = 0; y < h; y++) {
        for (int32_t x = 0; x < w; x++) {
            fb_put_pixel(area->x1 + x, area->y1 + y, src[y * w + x]);
        }
    }

    // Tell LVGL the flush is done
    lv_display_flush_ready(disp);
}

// ── Tick callback — called from the PIT interrupt at 1ms intervals ───────────
void bubo_lv_tick(void)
{
    lv_tick_inc(1);  // Tell LVGL 1ms has passed
}

// ── Input device — mouse/cursor from RINNEGAN gaze or PS/2 mouse ─────────────
static int32_t g_cursor_x = BUBO_SCREEN_W / 2;
static int32_t g_cursor_y = BUBO_SCREEN_H / 2;
static bool    g_cursor_pressed = false;

void bubo_lv_set_cursor(int32_t x, int32_t y, bool pressed)
{
    g_cursor_x = x;
    g_cursor_y = y;
    g_cursor_pressed = pressed;
}

static void bubo_lv_indev_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = (lv_coord_t)g_cursor_x;
    data->point.y = (lv_coord_t)g_cursor_y;
    data->state = g_cursor_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// ── Main init ─────────────────────────────────────────────────────────────────
void lv_port_bubo_init(void)
{
    // Initialize LVGL
    lv_init();

    // Create display
    g_disp = lv_display_create(BUBO_SCREEN_W, BUBO_SCREEN_H);
    lv_display_set_flush_cb(g_disp, bubo_lv_flush);
    lv_display_set_buffers(g_disp, lv_buf1, lv_buf2,
                           sizeof(lv_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_ARGB8888);

    // Register pointer input device (RINNEGAN eye-gaze or mouse)
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, bubo_lv_indev_read);
}

// ── Called every frame from the kernel main loop ─────────────────────────────
void lv_port_bubo_tick(void)
{
    lv_task_handler();
}
