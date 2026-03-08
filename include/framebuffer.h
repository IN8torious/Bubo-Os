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

#ifndef RAVEN_FRAMEBUFFER_H
#define RAVEN_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

// Framebuffer info struct
typedef struct {
    uint64_t addr;      // Physical address of framebuffer (64-bit)
    uint32_t width;
    uint32_t height;
    uint32_t pitch;     // Bytes per row
    uint8_t  bpp;       // Bits per pixel (32)
    uint8_t  type;      // 1 = RGB
    uint32_t size;      // Total size in bytes
} fb_info_t;

// Initialization
bool      fb_init_from_multiboot(uint32_t mb2_info_addr);
void      fb_init_fallback(void);
#ifdef __cplusplus
extern "C" {
#endif

fb_info_t* fb_get_info(void);
bool      fb_is_ready(void);

// Pixel operations
void     fb_put_pixel(int32_t x, int32_t y, uint32_t color);
uint32_t fb_get_pixel(int32_t x, int32_t y);
void     fb_clear(uint32_t color);

// Shapes
void fb_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void fb_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t thickness, uint32_t color);
void fb_draw_hline(int32_t x, int32_t y, int32_t len, uint32_t color);
void fb_draw_vline(int32_t x, int32_t y, int32_t len, uint32_t color);
void fb_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
void fb_draw_circle(int32_t cx, int32_t cy, int32_t r, uint32_t color);
void fb_fill_circle(int32_t cx, int32_t cy, int32_t r, uint32_t color);

// Bitmap
void fb_blit(int32_t x, int32_t y, int32_t w, int32_t h,
             const uint32_t* pixels, bool alpha_blend);

// Color helpers
uint32_t fb_rgb(uint8_t r, uint8_t g, uint8_t b);
uint32_t fb_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// Akatsuki color palette
extern const uint32_t FB_BLACK;
extern const uint32_t FB_WHITE;
extern const uint32_t FB_RED;
extern const uint32_t FB_LIGHT_RED;
extern const uint32_t FB_DARK_RED;
extern const uint32_t FB_GREY;
extern const uint32_t FB_LIGHT_GREY;
extern const uint32_t FB_DARK_GREY;
extern const uint32_t FB_ORANGE;

// Convenience macro: pack RGB
#define RGB(r,g,b) (((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(b))

#ifdef __cplusplus
}
#endif

#endif // RAVEN_FRAMEBUFFER_H