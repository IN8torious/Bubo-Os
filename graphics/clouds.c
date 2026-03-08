// =============================================================================
// Deep Flow OS — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI.
//
// clouds.c — Akatsuki Particle Cloud Engine + Rinnegan Glow System
//
// Real particle-based clouds using bezier curve shapes, layered depth,
// wind drift, and alpha compositing — all in bare-metal C on the framebuffer.
//
// The Rinnegan glows when CORVUS is active. Glow intensity maps to task load:
//   IDLE     → faint purple pulse
//   THINKING → medium purple glow, slow rotation
//   DRIVING  → deep crimson blaze, fast pulse
//   NITRO    → full white-crimson explosion
//   VOICE    → blue-purple ripple
// =============================================================================

#include "clouds.h"

typedef enum {
    RINNEGAN_IDLE     = 0,
    RINNEGAN_THINKING = 1,
    RINNEGAN_DRIVING  = 2,
    RINNEGAN_NITRO    = 3,
    RINNEGAN_VOICE    = 4,
} rinnegan_state_t;

#include "framebuffer.h"
#include "polish.h"
#include <stdint.h>
#include <stdbool.h>

// ── Fixed-point math (16.16) ──────────────────────────────────────────────────
#define FP_SHIFT    16
#define FP_ONE      (1 << FP_SHIFT)
#define FP_MUL(a,b) (((int64_t)(a) * (b)) >> FP_SHIFT)
#define INT_TO_FP(x) ((x) << FP_SHIFT)
#define FP_TO_INT(x) ((x) >> FP_SHIFT)

// ── Pseudo-random number generator (xorshift32) ───────────────────────────────
static uint32_t rng_state = 0xDEADBEEF;
static uint32_t rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
static int32_t rng_range(int32_t lo, int32_t hi) {
    return lo + (int32_t)(rng_next() % (uint32_t)(hi - lo + 1));
}

// ── Cloud particle definition ─────────────────────────────────────────────────
// Each cloud is a cluster of overlapping ellipses arranged in an Akatsuki
// cloud shape — large bottom lobe, smaller top lobes, organic edges.
typedef struct {
    int32_t  x, y;          // center position (pixels)
    int32_t  w, h;          // bounding width/height
    uint32_t color;          // base color (ARGB)
    uint8_t  alpha;          // base alpha (0-255)
    int32_t  vx;             // horizontal drift velocity (fp 16.16, pixels/tick)
    int32_t  depth;          // 0=far (small,slow,dark), 2=near (large,fast,bright)
    bool     active;
    // Bezier lobe offsets for organic shape (relative to center)
    int32_t  lobe_x[6];
    int32_t  lobe_y[6];
    int32_t  lobe_rx[6];     // lobe x-radius
    int32_t  lobe_ry[6];     // lobe y-radius
    int32_t  lobe_count;
} cloud_particle_t;

#define MAX_CLOUDS 24
static cloud_particle_t clouds[MAX_CLOUDS];
static int32_t screen_w = 1024;
static int32_t screen_h = 768;

// ── Rinnegan state ────────────────────────────────────────────────────────────
static rinnegan_state_t rinnegan_state = RINNEGAN_IDLE;
static uint32_t rinnegan_tick = 0;
static int32_t  rinnegan_cx = 0;
static int32_t  rinnegan_cy = 0;
static uint8_t  rinnegan_glow_alpha = 0;
static int32_t  rinnegan_glow_radius = 0;
static bool     rinnegan_pulse_up = true;

// ── Color helpers ─────────────────────────────────────────────────────────────
static inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
static inline uint8_t r_of(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t g_of(uint32_t c) { return (c >>  8) & 0xFF; }
static inline uint8_t b_of(uint32_t c) { return  c        & 0xFF; }

// ── Alpha-blend a pixel onto the framebuffer ──────────────────────────────────
static inline void blend_pixel(int32_t x, int32_t y, uint32_t color, uint8_t alpha) {
    if (x < 0 || y < 0 || x >= screen_w || y >= screen_h) return;
    if (alpha == 0) return;
    uint32_t dst = fb_get_pixel((uint32_t)x, (uint32_t)y);
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >>  8) & 0xFF;
    uint32_t db =  dst        & 0xFF;
    uint32_t sr = r_of(color);
    uint32_t sg = g_of(color);
    uint32_t sb = b_of(color);
    uint32_t a = alpha;
    uint32_t ia = 255 - a;
    uint32_t nr = (sr * a + dr * ia) >> 8;
    uint32_t ng = (sg * a + dg * ia) >> 8;
    uint32_t nb = (sb * a + db * ia) >> 8;
    fb_put_pixel((uint32_t)x, (uint32_t)y, (nr << 16) | (ng << 8) | nb);
}

// ── Draw a filled ellipse with alpha ─────────────────────────────────────────
static void draw_ellipse_alpha(int32_t cx, int32_t cy, int32_t rx, int32_t ry,
                                uint32_t color, uint8_t alpha) {
    if (rx <= 0 || ry <= 0 || alpha == 0) return;
    int32_t y1 = cy - ry, y2 = cy + ry;
    for (int32_t py = y1; py <= y2; py++) {
        int32_t dy = py - cy;
        // x range: (dx/rx)^2 + (dy/ry)^2 <= 1
        // dx^2 <= rx^2 * (1 - (dy/ry)^2) = rx^2 * (ry^2 - dy^2) / ry^2
        int64_t rhs = (int64_t)rx * rx * (int64_t)(ry * ry - dy * dy) / (ry * ry);
        if (rhs < 0) continue;
        int32_t dx_max = 0;
        // integer sqrt approximation
        int64_t s = rhs;
        int64_t sq = 0;
        for (int bit = 1 << 15; bit > 0; bit >>= 1) {
            if ((sq + bit) * (sq + bit) <= s) sq += bit;
        }
        dx_max = (int32_t)sq;
        for (int32_t px = cx - dx_max; px <= cx + dx_max; px++) {
            // soft edge: fade outer 2 pixels
            int32_t edge_dx = (px < cx) ? (cx - dx_max - px) : (px - (cx + dx_max));
            uint8_t edge_alpha = alpha;
            if (edge_dx >= -2) {
                int32_t fade = edge_dx + 3; // 1..3
                edge_alpha = (uint8_t)((uint32_t)alpha * (uint32_t)(3 - (fade < 3 ? fade : 3)) / 3);
            }
            blend_pixel(px, py, color, edge_alpha);
        }
    }
}

// ── Spawn a single cloud ──────────────────────────────────────────────────────
static void spawn_cloud(int32_t idx, bool offscreen_left) {
    cloud_particle_t* c = &clouds[idx];
    c->active = true;
    c->depth  = rng_range(0, 2);

    // Depth determines size, speed, brightness
    int32_t base_w, base_h;
    uint8_t  base_alpha;
    int32_t  base_speed;  // fp pixels per tick
    uint32_t base_color;

    switch (c->depth) {
        case 0: // far — small, slow, dark
            base_w     = rng_range(80, 140);
            base_h     = rng_range(40, 70);
            base_alpha = rng_range(30, 55);
            base_speed = rng_range(20, 50);   // fp/256 per tick → ~0.08–0.2 px/tick
            base_color = rgba(80, 0, 0, 255);
            break;
        case 1: // mid
            base_w     = rng_range(140, 220);
            base_h     = rng_range(60, 100);
            base_alpha = rng_range(55, 90);
            base_speed = rng_range(50, 100);
            base_color = rgba(120, 0, 0, 255);
            break;
        default: // near — large, fast, bright
            base_w     = rng_range(200, 320);
            base_h     = rng_range(90, 140);
            base_alpha = rng_range(90, 140);
            base_speed = rng_range(100, 180);
            base_color = rgba(160, 10, 10, 255);
            break;
    }

    c->w     = base_w;
    c->h     = base_h;
    c->alpha = (uint8_t)base_alpha;
    c->color = base_color;
    c->vx    = (base_speed << 8);  // store as fp*256 for sub-pixel motion
    c->y     = rng_range(screen_h / 6, screen_h * 5 / 6);

    if (offscreen_left) {
        c->x = -base_w - rng_range(0, 100);
    } else {
        c->x = rng_range(-base_w, screen_w);
    }

    // Build organic Akatsuki cloud shape:
    // Main bottom lobe (large), 2-3 top lobes (smaller, offset up)
    c->lobe_count = rng_range(4, 6);

    // Lobe 0: main body — wide, centered
    c->lobe_x[0] = 0;
    c->lobe_y[0] = base_h / 6;
    c->lobe_rx[0] = base_w / 2;
    c->lobe_ry[0] = base_h * 2 / 3;

    // Lobe 1: left top bump
    c->lobe_x[1] = -base_w / 4;
    c->lobe_y[1] = -base_h / 4;
    c->lobe_rx[1] = base_w / 4;
    c->lobe_ry[1] = base_h / 2;

    // Lobe 2: right top bump
    c->lobe_x[2] = base_w / 4;
    c->lobe_y[2] = -base_h / 5;
    c->lobe_rx[2] = base_w / 4;
    c->lobe_ry[2] = base_h * 2 / 5;

    // Lobe 3: center top peak
    c->lobe_x[3] = rng_range(-base_w/8, base_w/8);
    c->lobe_y[3] = -base_h / 3;
    c->lobe_rx[3] = base_w / 5;
    c->lobe_ry[3] = base_h / 3;

    if (c->lobe_count >= 5) {
        // Extra left bump
        c->lobe_x[4] = -base_w * 3 / 8;
        c->lobe_y[4] = -base_h / 8;
        c->lobe_rx[4] = base_w / 6;
        c->lobe_ry[4] = base_h / 4;
    }
    if (c->lobe_count >= 6) {
        // Extra right bump
        c->lobe_x[5] = base_w * 3 / 8;
        c->lobe_y[5] = -base_h / 10;
        c->lobe_rx[5] = base_w / 7;
        c->lobe_ry[5] = base_h / 5;
    }
}

// ── Initialize the cloud system ───────────────────────────────────────────────
void clouds_init(int32_t sw, int32_t sh) {
    screen_w = sw;
    screen_h = sh;
    rinnegan_cx = sw / 2;
    rinnegan_cy = sh / 2;

    for (int32_t i = 0; i < MAX_CLOUDS; i++) {
        spawn_cloud(i, false);
    }
}

// ── Draw all clouds for this frame ────────────────────────────────────────────
void clouds_draw(void) {
    for (int32_t i = 0; i < MAX_CLOUDS; i++) {
        cloud_particle_t* c = &clouds[i];
        if (!c->active) continue;

        // Draw each lobe back-to-front (lobe 0 first = bottom)
        for (int32_t l = 0; l < c->lobe_count; l++) {
            int32_t lx = c->x + c->lobe_x[l];
            int32_t ly = c->y + c->lobe_y[l];
            int32_t rx = c->lobe_rx[l];
            int32_t ry = c->lobe_ry[l];

            // Darker inner shadow for depth
            draw_ellipse_alpha(lx + 3, ly + 4, rx, ry,
                               rgba(20, 0, 0, 255), (uint8_t)(c->alpha / 3));
            // Main lobe
            draw_ellipse_alpha(lx, ly, rx, ry, c->color, c->alpha);
            // Highlight — lighter top edge
            draw_ellipse_alpha(lx, ly - ry/4, rx * 3/4, ry/4,
                               rgba(200, 20, 20, 255), (uint8_t)(c->alpha / 4));
        }
    }
}

// ── Tick: advance cloud positions ─────────────────────────────────────────────
void clouds_tick(void) {
    for (int32_t i = 0; i < MAX_CLOUDS; i++) {
        cloud_particle_t* c = &clouds[i];
        if (!c->active) continue;

        // Sub-pixel motion: vx is fp*256
        c->x += c->vx >> 8;

        // Wrap around: when cloud drifts off right edge, respawn on left
        if (c->x > screen_w + c->w) {
            spawn_cloud(i, true);
        }
    }
}

// ── Rinnegan: set state ───────────────────────────────────────────────────────
void rinnegan_set_state(rinnegan_state_t state) {
    rinnegan_state = state;
    rinnegan_tick  = 0;
}

void rinnegan_set_position(int32_t cx, int32_t cy) {
    rinnegan_cx = cx;
    rinnegan_cy = cy;
}

// ── Rinnegan: draw the eye with glow ─────────────────────────────────────────
// The Rinnegan is concentric circles with 6 tomoe (comma shapes) arranged
// around the pupil. Glow color and intensity depend on CORVUS state.
static void draw_rinnegan(int32_t cx, int32_t cy, int32_t radius,
                           uint32_t glow_color, uint8_t glow_alpha,
                           int32_t glow_radius) {
    // ── Outer glow rings (largest first, most transparent) ────────────────────
    for (int32_t r = glow_radius; r > radius; r -= 4) {
        uint8_t ga = (uint8_t)((uint32_t)glow_alpha *
                               (uint32_t)(glow_radius - r) /
                               (uint32_t)(glow_radius - radius + 1));
        draw_ellipse_alpha(cx, cy, r, r, glow_color, ga);
    }

    // ── Main eye body ─────────────────────────────────────────────────────────
    // Outer ring — dark purple/grey
    draw_ellipse_alpha(cx, cy, radius, radius, rgba(40, 20, 60, 255), 200);
    // Inner iris — concentric rings
    int32_t ring_colors[5][3] = {
        {60,  30,  90},   // ring 1
        {50,  20,  75},   // ring 2
        {40,  15,  60},   // ring 3
        {30,  10,  50},   // ring 4
        {15,   5,  30},   // pupil
    };
    for (int32_t i = 0; i < 5; i++) {
        int32_t r = radius - (i * radius / 5);
        draw_ellipse_alpha(cx, cy, r, r,
            rgba((uint8_t)ring_colors[i][0],
                 (uint8_t)ring_colors[i][1],
                 (uint8_t)ring_colors[i][2], 255), 220);
    }

    // ── Tomoe (6 comma shapes around the iris) ────────────────────────────────
    // Each tomoe: small filled circle + teardrop tail pointing inward
    int32_t tomoe_r = radius * 2 / 3;  // orbit radius
    for (int32_t t = 0; t < 6; t++) {
        // Angle: 60 degrees apart, offset by rinnegan_tick for rotation
        // Use lookup table for sin/cos (fixed point, scaled *256)
        static const int32_t sin_lut[6] = { 0, 222, 222, 0, -222, -222 };
        static const int32_t cos_lut[6] = { 256, 128, -128, -256, -128, 128 };
        int32_t tx = cx + (tomoe_r * cos_lut[t]) / 256;
        int32_t ty = cy + (tomoe_r * sin_lut[t]) / 256;
        int32_t tr = radius / 8;
        // Tomoe head
        draw_ellipse_alpha(tx, ty, tr, tr, rgba(180, 100, 220, 255), 200);
        // Tomoe tail (smaller ellipse toward center)
        int32_t tail_x = cx + (tomoe_r * 3 / 4 * cos_lut[t]) / 256;
        int32_t tail_y = cy + (tomoe_r * 3 / 4 * sin_lut[t]) / 256;
        draw_ellipse_alpha(tail_x, tail_y, tr * 2 / 3, tr / 2,
                           rgba(160, 80, 200, 255), 160);
    }

    // ── Pupil ─────────────────────────────────────────────────────────────────
    draw_ellipse_alpha(cx, cy, radius / 8, radius / 8, rgba(5, 0, 10, 255), 255);

    // ── State-specific inner glow ─────────────────────────────────────────────
    if (glow_alpha > 20) {
        draw_ellipse_alpha(cx, cy, radius / 3, radius / 3,
                           glow_color, (uint8_t)(glow_alpha / 3));
    }
}

// ── Rinnegan: tick and draw ───────────────────────────────────────────────────
void rinnegan_tick_and_draw(void) {
    rinnegan_tick++;

    uint32_t glow_color;
    uint8_t  target_alpha;
    int32_t  target_radius;
    int32_t  base_radius = 60;  // base eye radius in pixels

    switch (rinnegan_state) {
        case RINNEGAN_IDLE:
            // Slow faint purple pulse
            glow_color    = rgba(80, 0, 120, 255);
            target_alpha  = 30;
            target_radius = base_radius + 15;
            break;

        case RINNEGAN_THINKING:
            // Medium purple glow, moderate pulse
            glow_color    = rgba(120, 0, 180, 255);
            target_alpha  = 80;
            target_radius = base_radius + 30;
            break;

        case RINNEGAN_DRIVING:
            // Deep crimson blaze, fast pulse
            glow_color    = rgba(200, 10, 10, 255);
            target_alpha  = 120;
            target_radius = base_radius + 45;
            break;

        case RINNEGAN_NITRO:
            // Full white-crimson explosion — maximum glow
            glow_color    = rgba(255, 80, 0, 255);
            target_alpha  = 200;
            target_radius = base_radius + 80;
            break;

        case RINNEGAN_VOICE:
            // Blue-purple ripple
            glow_color    = rgba(40, 80, 220, 255);
            target_alpha  = 100;
            target_radius = base_radius + 35;
            break;

        default:
            glow_color    = rgba(80, 0, 120, 255);
            target_alpha  = 30;
            target_radius = base_radius + 15;
            break;
    }

    // Pulse: smoothly oscillate glow_alpha between 50% and 100% of target
    uint32_t pulse_period = (rinnegan_state == RINNEGAN_NITRO) ? 8 :
                            (rinnegan_state == RINNEGAN_DRIVING) ? 15 :
                            (rinnegan_state == RINNEGAN_VOICE) ? 12 : 30;

    uint32_t phase = rinnegan_tick % (pulse_period * 2);
    uint8_t pulse_alpha;
    if (phase < pulse_period) {
        pulse_alpha = (uint8_t)(target_alpha / 2 +
                      (uint32_t)target_alpha * phase / (pulse_period * 2));
    } else {
        pulse_alpha = (uint8_t)(target_alpha -
                      (uint32_t)target_alpha * (phase - pulse_period) / (pulse_period * 2));
    }

    // Smooth interpolation toward target radius
    if (rinnegan_glow_radius < target_radius) rinnegan_glow_radius += 2;
    if (rinnegan_glow_radius > target_radius) rinnegan_glow_radius -= 2;

    draw_rinnegan(rinnegan_cx, rinnegan_cy, base_radius,
                  glow_color, pulse_alpha, rinnegan_glow_radius);
}
