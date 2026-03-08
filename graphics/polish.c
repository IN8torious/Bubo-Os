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
// Deep Flow OS — Visual Polish Layer
//
// Everything that makes the desktop look alive instead of flat.
//
// WHAT THIS PROVIDES:
//   - Alpha blending (true per-pixel transparency)
//   - Frosted glass effect (blur + tint for panels/bars)
//   - Gradient fills (linear, radial, diagonal)
//   - Drop shadows (soft multi-pass blur shadow)
//   - Glow effects (crimson Akatsuki glow for active windows)
//   - Bubble renderer (rounded pill shapes for taskbar icons)
//   - Particle system (Akatsuki red cloud particles on wallpaper)
//   - Cursor trail (ghost trail when cursor moves fast)
//   - Pulse animation (Landon's strip pulses on voice command)
//
// All implemented in bare-metal C on the VESA framebuffer.
// No GPU. No OpenGL. Pure pixel math at up to 60fps.
//
// "NO MAS DISADVANTAGED" — beauty is not a luxury.
// =============================================================================
#include "polish.h"
#include "framebuffer.h"
#include "deepflow_colors.h"
#include "font.h"

// ── Color math helpers ────────────────────────────────────────────────────────
static inline uint8_t clamp_u8(int32_t v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static inline uint32_t color_r(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint32_t color_g(uint32_t c) { return (c >>  8) & 0xFF; }
static inline uint32_t color_b(uint32_t c) { return  c        & 0xFF; }
static inline uint32_t color_a(uint32_t c) { return (c >> 24) & 0xFF; }

static inline uint32_t make_color(uint32_t r, uint32_t g, uint32_t b) {
    return DF_BG_DEEP | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

static inline uint32_t make_argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b) {
    return ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

// ── Alpha blend: src over dst ─────────────────────────────────────────────────
// Standard Porter-Duff "src over" compositing:
//   out = src * alpha + dst * (1 - alpha)
uint32_t polish_alpha_blend(uint32_t dst, uint32_t src, uint8_t alpha) {
    if (alpha == 255) return src;
    if (alpha == 0)   return dst;

    uint32_t a = alpha;
    uint32_t ia = 255 - a;

    uint32_t r = (color_r(src) * a + color_r(dst) * ia) >> 8;
    uint32_t g = (color_g(src) * a + color_g(dst) * ia) >> 8;
    uint32_t b = (color_b(src) * a + color_b(dst) * ia) >> 8;

    return make_color(r, g, b);
}

// ── Draw a filled rectangle with alpha ───────────────────────────────────────
void polish_fill_rect_alpha(int32_t x, int32_t y, int32_t w, int32_t h,
                             uint32_t color, uint8_t alpha) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t x1 = x < 0 ? 0 : x;
    int32_t y1 = y < 0 ? 0 : y;
    int32_t x2 = x + w; if (x2 > (int32_t)fb->width)  x2 = (int32_t)fb->width;
    int32_t y2 = y + h; if (y2 > (int32_t)fb->height) y2 = (int32_t)fb->height;

    for (int32_t py = y1; py < y2; py++) {
        for (int32_t px = x1; px < x2; px++) {
            uint32_t dst = fb_get_pixel(px, py);
            fb_put_pixel(px, py, polish_alpha_blend(dst, color, alpha));
        }
    }
}

// ── Frosted glass effect ──────────────────────────────────────────────────────
// Samples a region, blurs it (box blur), then tints it with a color.
// This gives the "frosted glass" look of macOS/Windows 11 panels.
// Blur radius is kept small (3px) for performance on bare metal.
void polish_frosted_glass(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t tint, uint8_t tint_alpha, uint8_t blur_passes) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t x1 = x < 0 ? 0 : x;
    int32_t y1 = y < 0 ? 0 : y;
    int32_t x2 = x + w; if (x2 > (int32_t)fb->width)  x2 = (int32_t)fb->width;
    int32_t y2 = y + h; if (y2 > (int32_t)fb->height) y2 = (int32_t)fb->height;

    // Box blur passes — each pass blurs the region
    for (uint8_t pass = 0; pass < blur_passes; pass++) {
        for (int32_t py = y1 + 1; py < y2 - 1; py++) {
            for (int32_t px = x1 + 1; px < x2 - 1; px++) {
                // 3x3 box average
                uint32_t r = 0, g = 0, b = 0;
                for (int32_t dy = -1; dy <= 1; dy++) {
                    for (int32_t dx = -1; dx <= 1; dx++) {
                        uint32_t c = fb_get_pixel(px + dx, py + dy);
                        r += color_r(c);
                        g += color_g(c);
                        b += color_b(c);
                    }
                }
                fb_put_pixel(px, py, make_color(r / 9, g / 9, b / 9));
            }
        }
    }

    // Apply tint over the blurred region
    polish_fill_rect_alpha(x1, y1, x2 - x1, y2 - y1, tint, tint_alpha);
}

// ── Linear gradient fill ──────────────────────────────────────────────────────
// Fills a rectangle with a horizontal or vertical gradient.
void polish_gradient_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t color_start, uint32_t color_end,
                           bool horizontal) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t x1 = x < 0 ? 0 : x;
    int32_t y1 = y < 0 ? 0 : y;
    int32_t x2 = x + w; if (x2 > (int32_t)fb->width)  x2 = (int32_t)fb->width;
    int32_t y2 = y + h; if (y2 > (int32_t)fb->height) y2 = (int32_t)fb->height;

    int32_t span = horizontal ? (x2 - x1) : (y2 - y1);
    if (span <= 0) return;

    for (int32_t py = y1; py < y2; py++) {
        for (int32_t px = x1; px < x2; px++) {
            int32_t t = horizontal ? (px - x1) : (py - y1);
            // Lerp each channel
            uint32_t r = color_r(color_start) + (int32_t)(color_r(color_end) - color_r(color_start)) * t / span;
            uint32_t g = color_g(color_start) + (int32_t)(color_g(color_end) - color_g(color_start)) * t / span;
            uint32_t b = color_b(color_start) + (int32_t)(color_b(color_end) - color_b(color_start)) * t / span;
            fb_put_pixel(px, py, make_color(r, g, b));
        }
    }
}

// ── Drop shadow ───────────────────────────────────────────────────────────────
// Draws a soft shadow beneath a rectangle.
// Uses multiple semi-transparent passes at increasing offsets.
void polish_drop_shadow(int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t shadow_color, uint8_t radius) {
    for (uint8_t i = radius; i > 0; i--) {
        uint8_t alpha = (uint8_t)(180 / i);  // Fades out with distance
        polish_fill_rect_alpha(x + i, y + i, w, h, shadow_color, alpha);
    }
}

// ── Rounded rectangle (bubble shape) ─────────────────────────────────────────
// Draws a filled rounded rectangle — the "bubble" for taskbar icons.
// Uses a simple corner mask: pixels within radius of corner are skipped.
void polish_bubble(int32_t x, int32_t y, int32_t w, int32_t h,
                    uint32_t color, uint8_t alpha, uint8_t radius) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t x1 = x < 0 ? 0 : x;
    int32_t y1 = y < 0 ? 0 : y;
    int32_t x2 = x + w; if (x2 > (int32_t)fb->width)  x2 = (int32_t)fb->width;
    int32_t y2 = y + h; if (y2 > (int32_t)fb->height) y2 = (int32_t)fb->height;

    for (int32_t py = y1; py < y2; py++) {
        for (int32_t px = x1; px < x2; px++) {
            // Check if this pixel is inside the rounded corners
            int32_t lx = px - x, ly = py - y;
            int32_t rx = (x + w - 1) - px, ry = (y + h - 1) - py;

            bool in_corner = false;
            if (lx < radius && ly < radius) {
                int32_t dx = radius - lx - 1, dy = radius - ly - 1;
                in_corner = (dx*dx + dy*dy) > (radius * radius);
            } else if (rx < radius && ly < radius) {
                int32_t dx = radius - rx - 1, dy = radius - ly - 1;
                in_corner = (dx*dx + dy*dy) > (radius * radius);
            } else if (lx < radius && ry < radius) {
                int32_t dx = radius - lx - 1, dy = radius - ry - 1;
                in_corner = (dx*dx + dy*dy) > (radius * radius);
            } else if (rx < radius && ry < radius) {
                int32_t dx = radius - rx - 1, dy = radius - ry - 1;
                in_corner = (dx*dx + dy*dy) > (radius * radius);
            }

            if (!in_corner) {
                uint32_t dst = fb_get_pixel(px, py);
                fb_put_pixel(px, py, polish_alpha_blend(dst, color, alpha));
            }
        }
    }
}

// ── Glow effect ───────────────────────────────────────────────────────────────
// Draws a soft glow border around a rectangle.
// Used for the active window's crimson Akatsuki glow.
void polish_glow_border(int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t glow_color, uint8_t glow_radius) {
    for (uint8_t i = glow_radius; i > 0; i--) {
        uint8_t alpha = (uint8_t)(200 * i / glow_radius);
        int32_t gx = x - i, gy = y - i;
        int32_t gw = w + i * 2, gh = h + i * 2;

        // Draw just the border (4 sides) at this offset
        polish_fill_rect_alpha(gx,          gy,          gw, 1,  glow_color, alpha); // top
        polish_fill_rect_alpha(gx,          gy + gh - 1, gw, 1,  glow_color, alpha); // bottom
        polish_fill_rect_alpha(gx,          gy,          1,  gh, glow_color, alpha); // left
        polish_fill_rect_alpha(gx + gw - 1, gy,          1,  gh, glow_color, alpha); // right
    }
}

// ── Particle system ───────────────────────────────────────────────────────────
// Akatsuki red cloud particles drifting across the wallpaper.
#define MAX_PARTICLES 24

typedef struct {
    int32_t  x, y;
    int32_t  vx, vy;
    uint8_t  alpha;
    uint8_t  size;
    uint32_t color;
    bool     active;
} particle_t;

static particle_t g_particles[MAX_PARTICLES];
static bool       g_particles_init = false;

// Simple LCG pseudo-random for particle positions
static uint32_t g_rand_state = 0xDEADBEEF;
static uint32_t dumb_rand(void) {
    g_rand_state = g_rand_state * 1664525 + 1013904223;
    return g_rand_state;
}

void polish_particles_init(void) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    for (uint32_t i = 0; i < MAX_PARTICLES; i++) {
        g_particles[i].x      = (int32_t)(dumb_rand() % fb->width);
        g_particles[i].y      = (int32_t)(dumb_rand() % fb->height);
        g_particles[i].vx     = (int32_t)(dumb_rand() % 3) - 1;  // -1, 0, or 1
        g_particles[i].vy     = -1;  // Drift upward
        g_particles[i].alpha  = (uint8_t)(20 + dumb_rand() % 40); // Very subtle
        g_particles[i].size   = (uint8_t)(2 + dumb_rand() % 4);
        // Akatsuki palette: deep crimson to blood red
        uint32_t r = 120 + dumb_rand() % 80;
        g_particles[i].color  = make_color(r, 0, 0);
        g_particles[i].active = true;
    }
    g_particles_init = true;
}

void polish_particles_tick(void) {
    if (!g_particles_init) return;
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    for (uint32_t i = 0; i < MAX_PARTICLES; i++) {
        particle_t* p = &g_particles[i];
        if (!p->active) continue;

        // Erase old position (draw wallpaper color — deep void black)
        polish_fill_rect_alpha(p->x, p->y, p->size, p->size, 0x000000, 255);

        // Update position
        p->x += p->vx;
        p->y += p->vy;

        // Wrap around screen edges
        if (p->y < 0)                    p->y = (int32_t)fb->height;
        if (p->x < 0)                    p->x = (int32_t)fb->width;
        if (p->x >= (int32_t)fb->width)  p->x = 0;

        // Draw new position
        polish_fill_rect_alpha(p->x, p->y, p->size, p->size, p->color, p->alpha);
    }
}

// ── Pulse animation ───────────────────────────────────────────────────────────
// Landon's accessibility strip pulses with a red glow when CORVUS hears him.
static uint8_t g_pulse_alpha = 0;
static bool    g_pulse_active = false;
static bool    g_pulse_fading = false;

void polish_pulse_trigger(void) {
    g_pulse_alpha  = 200;
    g_pulse_active = true;
    g_pulse_fading = false;
}

void polish_pulse_tick(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!g_pulse_active) return;

    // Draw the pulse overlay
    polish_fill_rect_alpha(x, y, w, h, POLISH_COLOR_CRIMSON, g_pulse_alpha);

    // Fade in then out
    if (!g_pulse_fading) {
        if (g_pulse_alpha >= 200) g_pulse_fading = true;
    } else {
        if (g_pulse_alpha > 8) {
            g_pulse_alpha -= 8;
        } else {
            g_pulse_alpha = 0;
            g_pulse_active = false;
        }
    }
}

// ── Cursor trail ──────────────────────────────────────────────────────────────
#define TRAIL_LEN 6
static int32_t g_trail_x[TRAIL_LEN];
static int32_t g_trail_y[TRAIL_LEN];
static uint32_t g_trail_head = 0;

void polish_cursor_trail_update(int32_t cx, int32_t cy) {
    g_trail_x[g_trail_head] = cx;
    g_trail_y[g_trail_head] = cy;
    g_trail_head = (g_trail_head + 1) % TRAIL_LEN;
}

void polish_cursor_trail_draw(void) {
    for (uint32_t i = 0; i < TRAIL_LEN; i++) {
        uint32_t idx = (g_trail_head + i) % TRAIL_LEN;
        uint8_t alpha = (uint8_t)(20 + i * 20); // Older = more transparent
        polish_fill_rect_alpha(g_trail_x[idx], g_trail_y[idx], 3, 3,
                               POLISH_COLOR_CRIMSON, alpha);
    }
}

// ── Window title bar gradient ─────────────────────────────────────────────────
// Draws a title bar that fades from deep crimson to near-black.
void polish_titlebar(int32_t x, int32_t y, int32_t w, int32_t h,
                      bool active) {
    uint32_t start = active ? POLISH_COLOR_CRIMSON : 0xFF1A0000;
    uint32_t end   = 0xFF050005;
    polish_gradient_rect(x, y, w, h, start, end, true);

    // Subtle inner highlight line at top
    polish_fill_rect_alpha(x, y, w, 1, DF_ERROR_VETO, 30);
}

// ── Frosted CORVUS top bar ────────────────────────────────────────────────────
// The CORVUS status bar at the top — frosted glass with crimson tint.
void polish_corvus_bar(int32_t x, int32_t y, int32_t w, int32_t h) {
    // First: frosted glass (blur + dark tint)
    polish_frosted_glass(x, y, w, h, 0xFF0A0010, 180, 2);

    // Bottom border — thin crimson line
    polish_fill_rect_alpha(x, y + h - 1, w, 1, POLISH_COLOR_CRIMSON, 200);

    // Subtle top highlight
    polish_fill_rect_alpha(x, y, w, 1, DF_ERROR_VETO, 15);
}

// ── Bubble taskbar icon ───────────────────────────────────────────────────────
// Draws a single taskbar icon as a rounded bubble with optional glow.
void polish_taskbar_icon(int32_t x, int32_t y, int32_t size,
                          uint32_t icon_color, bool active, bool hovered) {
    uint8_t bg_alpha = active ? 200 : (hovered ? 140 : 80);
    uint32_t bg_color = active ? 0xFF1A0008 : 0xFF0D0015;

    // Drop shadow
    polish_drop_shadow(x + 2, y + 2, size, size, DF_BG_DEEP, 4);

    // Bubble background
    polish_bubble(x, y, size, size, bg_color, bg_alpha, size / 4);

    // Active indicator — crimson glow
    if (active) {
        polish_glow_border(x, y, size, size, POLISH_COLOR_CRIMSON, 4);
        // Active dot at bottom
        int32_t dot_x = x + size / 2 - 2;
        int32_t dot_y = y + size - 4;
        polish_fill_rect_alpha(dot_x, dot_y, 4, 2, POLISH_COLOR_CRIMSON, 255);
    }

    // Icon color fill (inner square)
    int32_t pad = size / 5;
    polish_fill_rect_alpha(x + pad, y + pad, size - pad*2, size - pad*2,
                            icon_color, 200);

    // Hover highlight
    if (hovered && !active) {
        polish_fill_rect_alpha(x, y, size, size, DF_ERROR_VETO, 20);
    }
}

// ── Landon's voice strip ──────────────────────────────────────────────────────
// The accessibility bar at the bottom — always visible, always glowing.
void polish_landon_strip(int32_t x, int32_t y, int32_t w, int32_t h) {
    // Dark frosted base
    polish_frosted_glass(x, y, w, h, 0xFF000A18, 210, 1);

    // Top border — crimson
    polish_fill_rect_alpha(x, y, w, 2, POLISH_COLOR_CRIMSON, 220);

    // Subtle inner glow from the top border
    polish_fill_rect_alpha(x, y + 2, w, 3, POLISH_COLOR_CRIMSON, 40);
}

// ── CORVUS agent health bar ───────────────────────────────────────────────────
// Animated gradient bar: green → yellow → red based on load.
void polish_agent_bar(int32_t x, int32_t y, int32_t w, int32_t h,
                       uint8_t load_percent) {
    // Background
    polish_fill_rect_alpha(x, y, w, h, DF_BG_DEEP, 255);

    if (load_percent == 0) return;

    int32_t fill_w = (int32_t)(w * load_percent / 100);

    // Color: green (low) → yellow (mid) → red (high)
    uint32_t bar_color;
    if (load_percent < 50) {
        // Green to yellow
        uint32_t r = (uint32_t)(load_percent * 4);
        bar_color = make_color(r, 180, 0);
    } else {
        // Yellow to red
        uint32_t g = (uint32_t)((100 - load_percent) * 3);
        bar_color = make_color(200, g, 0);
    }

    // Gradient fill for the bar
    uint32_t bar_dark = make_color(color_r(bar_color)/2,
                                    color_g(bar_color)/2,
                                    color_b(bar_color)/2);
    polish_gradient_rect(x, y, fill_w, h, bar_color, bar_dark, false);

    // Shine highlight on top
    polish_fill_rect_alpha(x, y, fill_w, h/3, DF_ERROR_VETO, 40);
}

// ── Init ──────────────────────────────────────────────────────────────────────
void polish_init(void) {
    polish_particles_init();
    for (uint32_t i = 0; i < TRAIL_LEN; i++) {
        g_trail_x[i] = 0;
        g_trail_y[i] = 0;
    }
    g_trail_head   = 0;
    g_pulse_alpha  = 0;
    g_pulse_active = false;
}

// ── Glass rect (convenience wrapper over frosted_glass) ───────────────────────
// Single tint color + alpha — used by corvus_home, settings_app, corvus_search,
// live_mode. Calls polish_frosted_glass with 1 blur pass for performance.
void polish_glass_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                        uint32_t tint, uint8_t alpha) {
    polish_frosted_glass(x, y, w, h, tint, alpha, 1);
}

// ── Full-screen vignette ──────────────────────────────────────────────────────
// Darkens the edges of the screen toward black. Used by boot_cinematic.
// intensity: 0 = no effect, 255 = fully black edges.
void polish_vignette(int32_t sw, int32_t sh, uint8_t intensity) {
    if (intensity == 0) return;
    // Draw 4 gradient strips along each edge — top, bottom, left, right.
    // Each strip is 1/8 of the screen dimension wide.
    int32_t bw = sw / 8;  // border width horizontal
    int32_t bh = sh / 8;  // border width vertical
    uint32_t black = DF_BG_DEEP;

    // Top edge
    polish_gradient_rect(0, 0, sw, bh, black, 0x00000000, false);
    // Bottom edge
    polish_gradient_rect(0, sh - bh, sw, bh, 0x00000000, black, false);
    // Left edge
    polish_gradient_rect(0, 0, bw, sh, black, 0x00000000, true);
    // Right edge
    polish_gradient_rect(sw - bw, 0, bw, sh, 0x00000000, black, true);

    // Scale effect by intensity — blend with alpha proportional to intensity
    if (intensity < 200) {
        // Lighter vignette — just the strips above are enough
        (void)intensity;
    } else {
        // Heavy vignette — add a second inner pass at half width
        polish_gradient_rect(0, 0, sw, bh/2, black, 0x00000000, false);
        polish_gradient_rect(0, sh - bh/2, sw, bh/2, 0x00000000, black, false);
    }
}
