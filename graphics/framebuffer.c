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

// =============================================================================
// Raven OS — VESA Framebuffer Driver
// Layer 3.5: Graphics Foundation
// Provides pixel-level rendering at 1024x768 32-bit color
// This is the foundation for ImGui, the Raven Renderer, and eventually UE5
// =============================================================================

#include "framebuffer.h"
#include "../include/vga.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ── Framebuffer state ─────────────────────────────────────────────────────────
static fb_info_t fb;
static bool fb_initialized = false;

// ── Multiboot2 tag types ──────────────────────────────────────────────────────
#define MB2_TAG_FRAMEBUFFER  8
#define MB2_TAG_END          0

typedef struct {
    uint32_t type;
    uint32_t size;
} mb2_tag_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
} mb2_tag_framebuffer_t;

// ── Parse multiboot2 info for framebuffer tag ─────────────────────────────────
bool fb_init_from_multiboot(uint32_t mb2_info_addr) {
    if (!mb2_info_addr) return false;

    // Skip total_size and reserved fields (8 bytes)
    uint8_t* ptr = (uint8_t*)(mb2_info_addr + 8);
    uint32_t total_size = *(uint32_t*)mb2_info_addr;
    uint8_t* end = (uint8_t*)mb2_info_addr + total_size;

    while (ptr < end) {
        mb2_tag_t* tag = (mb2_tag_t*)ptr;

        if (tag->type == MB2_TAG_END) break;

        if (tag->type == MB2_TAG_FRAMEBUFFER) {
            mb2_tag_framebuffer_t* fbtag = (mb2_tag_framebuffer_t*)ptr;

            fb.addr   = fbtag->framebuffer_addr;  // full 64-bit address
            fb.width  = fbtag->framebuffer_width;
            fb.height = fbtag->framebuffer_height;
            fb.pitch  = fbtag->framebuffer_pitch;
            fb.bpp    = fbtag->framebuffer_bpp;
            fb.type   = fbtag->framebuffer_type;
            fb.size   = fb.pitch * fb.height;

            fb_initialized = true;

            // Report via VGA text (before we switch to framebuffer)
            uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
            uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
            terminal_setcolor(white);
            terminal_write("  [ FB   ] Framebuffer: ");
            // Print resolution
            char buf[32];
            uint32_t w = fb.width, h = fb.height;
            int i = 0;
            if (!w) buf[i++] = '0';
            else { char t[8]; int j=0; while(w){t[j++]='0'+(w%10);w/=10;} while(j>0)buf[i++]=t[--j]; }
            buf[i++] = 'x';
            if (!h) buf[i++] = '0';
            else { char t[8]; int j=0; while(h){t[j++]='0'+(h%10);h/=10;} while(j>0)buf[i++]=t[--j]; }
            buf[i++] = '@';
            buf[i++] = '0' + (fb.bpp / 10);
            buf[i++] = '0' + (fb.bpp % 10);
            buf[i++] = 'b'; buf[i++] = 'p'; buf[i++] = 'p'; buf[i] = '\0';
            terminal_write(buf);
            terminal_write(" addr=0x");
            const char* hex = "0123456789ABCDEF";
            char hbuf[9]; hbuf[8] = '\0';
            for (int k = 7; k >= 0; k--) hbuf[7-k] = hex[(fb.addr >> (k*4)) & 0xF];
            terminal_write(hbuf);
            terminal_setcolor(green);
            terminal_writeline("  ✓");

            return true;
        }

        // Align to 8-byte boundary
        uint32_t next = ((tag->size + 7) & ~7);
        if (next == 0) break;
        ptr += next;
    }

    return false;
}

// ── Fallback: use a fixed VESA address if multiboot didn't give us one ────────
void fb_init_fallback(void) {
    // QEMU default VBE framebuffer address
    fb.addr   = 0xFD000000ULL;
    fb.width  = 1024;
    fb.height = 768;
    fb.pitch  = 1024 * 4;
    fb.bpp    = 32;
    fb.type   = 1;
    fb.size   = fb.pitch * fb.height;
    fb_initialized = true;

    uint8_t white = vga_entry_color(VGA_WHITE, VGA_BLACK);
    terminal_setcolor(white);
    terminal_writeline("  [ FB   ] Framebuffer: fallback 1024x768@32bpp at 0xFD000000");
}

// ── Get framebuffer info ──────────────────────────────────────────────────────
fb_info_t* fb_get_info(void) {
    return fb_initialized ? &fb : NULL;
}

bool fb_is_ready(void) {
    return fb_initialized;
}

// ── Core pixel operations ─────────────────────────────────────────────────────

static inline uint32_t* fb_pixel_ptr(int32_t x, int32_t y) {
    return (uint32_t*)(fb.addr + (uint64_t)y * fb.pitch + (uint64_t)x * 4);
}

void fb_put_pixel(int32_t x, int32_t y, uint32_t color) {
    if (!fb_initialized) return;
    if (x < 0 || y < 0 || (uint32_t)x >= fb.width || (uint32_t)y >= fb.height) return;
    *fb_pixel_ptr(x, y) = color;
}

uint32_t fb_get_pixel(int32_t x, int32_t y) {
    if (!fb_initialized) return 0;
    if (x < 0 || y < 0 || (uint32_t)x >= fb.width || (uint32_t)y >= fb.height) return 0;
    return *fb_pixel_ptr(x, y);
}

// ── Fill entire screen with color ─────────────────────────────────────────────
void fb_clear(uint32_t color) {
    if (!fb_initialized) return;
    uint32_t* p = (uint32_t*)(uintptr_t)fb.addr;
    uint32_t  n = fb.pitch * fb.height / 4;
    for (uint32_t i = 0; i < n; i++) p[i] = color;
}

// ── Draw filled rectangle ─────────────────────────────────────────────────────
void fb_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (!fb_initialized) return;
    // Clip
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int32_t)fb.width)  w = (int32_t)fb.width  - x;
    if (y + h > (int32_t)fb.height) h = (int32_t)fb.height - y;
    if (w <= 0 || h <= 0) return;

    for (int32_t row = y; row < y + h; row++) {
        uint32_t* p = fb_pixel_ptr(x, row);
        for (int32_t col = 0; col < w; col++) p[col] = color;
    }
}

// ── Draw rectangle outline ────────────────────────────────────────────────────
void fb_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                  int32_t thickness, uint32_t color) {
    fb_fill_rect(x,           y,           w,         thickness, color); // top
    fb_fill_rect(x,           y+h-thickness, w,       thickness, color); // bottom
    fb_fill_rect(x,           y,           thickness, h,         color); // left
    fb_fill_rect(x+w-thickness, y,         thickness, h,         color); // right
}

// ── Draw horizontal line ──────────────────────────────────────────────────────
void fb_draw_hline(int32_t x, int32_t y, int32_t len, uint32_t color) {
    fb_fill_rect(x, y, len, 1, color);
}

// ── Draw vertical line ────────────────────────────────────────────────────────
void fb_draw_vline(int32_t x, int32_t y, int32_t len, uint32_t color) {
    fb_fill_rect(x, y, 1, len, color);
}

// ── Bresenham line ────────────────────────────────────────────────────────────
void fb_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    if (!fb_initialized) return;
    int32_t dx = x1 - x0; if (dx < 0) dx = -dx;
    int32_t dy = y1 - y0; if (dy < 0) dy = -dy;
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx - dy;

    while (1) {
        fb_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// ── Draw circle (Midpoint algorithm) ─────────────────────────────────────────
void fb_draw_circle(int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    if (!fb_initialized) return;
    int32_t x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
        fb_put_pixel(cx+x, cy+y, color); fb_put_pixel(cx-x, cy+y, color);
        fb_put_pixel(cx+x, cy-y, color); fb_put_pixel(cx-x, cy-y, color);
        fb_put_pixel(cx+y, cy+x, color); fb_put_pixel(cx-y, cy+x, color);
        fb_put_pixel(cx+y, cy-x, color); fb_put_pixel(cx-y, cy-x, color);
        if (d < 0) d += 4*x + 6;
        else { d += 4*(x-y) + 10; y--; }
        x++;
    }
}

// ── Fill circle ───────────────────────────────────────────────────────────────
void fb_fill_circle(int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    for (int32_t y = -r; y <= r; y++) {
        int32_t w = 0;
        int32_t yy = y * y;
        int32_t rr = r * r;
        while ((w+1)*(w+1) + yy <= rr) w++;
        fb_fill_rect(cx - w, cy + y, 2*w+1, 1, color);
    }
}

// ── Blit a 32-bit ARGB bitmap ─────────────────────────────────────────────────
void fb_blit(int32_t x, int32_t y, int32_t w, int32_t h,
             const uint32_t* pixels, bool alpha_blend) {
    if (!fb_initialized || !pixels) return;
    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            uint32_t src = pixels[row * w + col];
            if (alpha_blend) {
                uint8_t a = (src >> 24) & 0xFF;
                if (a == 0) continue;
                if (a == 255) {
                    fb_put_pixel(x + col, y + row, src & 0x00FFFFFF);
                } else {
                    uint32_t dst = fb_get_pixel(x + col, y + row);
                    uint8_t sr = (src >> 16) & 0xFF;
                    uint8_t sg = (src >>  8) & 0xFF;
                    uint8_t sb = (src >>  0) & 0xFF;
                    uint8_t dr = (dst >> 16) & 0xFF;
                    uint8_t dg = (dst >>  8) & 0xFF;
                    uint8_t db = (dst >>  0) & 0xFF;
                    uint8_t or2 = (uint8_t)((sr * a + dr * (255 - a)) / 255);
                    uint8_t og = (uint8_t)((sg * a + dg * (255 - a)) / 255);
                    uint8_t ob = (uint8_t)((sb * a + db * (255 - a)) / 255);
                    fb_put_pixel(x + col, y + row,
                                 ((uint32_t)or2 << 16) | ((uint32_t)og << 8) | ob);
                }
            } else {
                fb_put_pixel(x + col, y + row, src & 0x00FFFFFF);
            }
        }
    }
}

// ── Color helpers ─────────────────────────────────────────────────────────────
uint32_t fb_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t fb_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// ── Raven OS color palette (Akatsuki theme) ───────────────────────────────────
const uint32_t FB_BLACK       = 0x000000;
const uint32_t FB_WHITE       = 0xFFFFFF;
const uint32_t FB_RED         = 0xCC0000;   // Akatsuki crimson
const uint32_t FB_LIGHT_RED   = 0xFF3333;
const uint32_t FB_DARK_RED    = 0x880000;
const uint32_t FB_GREY        = 0x333333;
const uint32_t FB_LIGHT_GREY  = 0x888888;
const uint32_t FB_DARK_GREY   = 0x111111;
const uint32_t FB_ORANGE      = 0xFF6600;   // Naruto orange accent
