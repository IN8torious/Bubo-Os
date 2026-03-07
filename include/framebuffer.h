#ifndef RAVEN_FRAMEBUFFER_H
#define RAVEN_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

// Framebuffer info struct
typedef struct {
    uint32_t addr;      // Physical address of framebuffer
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

#endif // RAVEN_FRAMEBUFFER_H
