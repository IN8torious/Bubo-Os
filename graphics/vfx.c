// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// graphics/vfx.c — Visual Effects Engine
//
// Depth of Field: multi-layer Gaussian blur composited by depth
// Head Bobbing:   sinusoidal camera sway driven by mouse/gyro input
// Particle Mouse Tracking: small particles attracted/repelled by cursor
// =============================================================================

#include "vfx.h"
#include "framebuffer.h"
#include <stdint.h>
#include <stdbool.h>

// ── Fixed-point sin LUT (64 entries = quarter wave, scaled * 32768) ───────────
static const int32_t fp_sin_lut[64] = {
       0,  804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512,10278,11039,11793,
   12539,13279,14010,14732,15446,16151,16846,17530,
   18204,18868,19519,20159,20787,21403,22005,22594,
   23170,23731,24279,24811,25329,25832,26319,26790,
   27245,27683,28105,28510,28898,29268,29621,29956,
   30273,30571,30852,31113,31356,31580,31785,31971,
   32137,32285,32412,32521,32609,32678,32728,32757
};

static int32_t vfx_sin(int32_t angle_256) {
    int32_t a = ((angle_256 % 256) + 256) % 256;
    int32_t q = a >> 6;
    int32_t i = a & 63;
    switch (q) {
        case 0: return  fp_sin_lut[i];
        case 1: return  fp_sin_lut[63 - i];
        case 2: return -fp_sin_lut[i];
        default:return -fp_sin_lut[63 - i];
    }
}

static int32_t vfx_cos(int32_t angle_256) {
    return vfx_sin(angle_256 + 64);
}

// ── State ─────────────────────────────────────────────────────────────────────
static int32_t  screen_w   = 1920;
static int32_t  screen_h   = 1080;
static uint32_t tick       = 0;
static int32_t  bob_angle  = 0;
static int32_t  bob_amp    = 3;
static bool     bob_on     = true;
static int32_t  mouse_x    = 960;
static int32_t  mouse_y    = 540;

// ── Particle system ───────────────────────────────────────────────────────────
#define MAX_PARTICLES 512

typedef struct {
    int32_t  x, y;      // fp*16
    int32_t  vx, vy;    // fp*16 per tick
    uint8_t  r, g, b;
    uint8_t  alpha;
    uint8_t  life;
    uint8_t  max_life;
    uint8_t  size;      // 1..3 px
    bool     active;
} particle_t;

static particle_t particles[MAX_PARTICLES];
static uint32_t   rng_s = 0xCAFEBABE;

static uint32_t prng(void) {
    rng_s ^= rng_s << 13;
    rng_s ^= rng_s >> 17;
    rng_s ^= rng_s << 5;
    return rng_s;
}

static int32_t prng_range(int32_t lo, int32_t hi) {
    if (hi <= lo) return lo;
    return lo + (int32_t)(prng() % (uint32_t)(hi - lo + 1));
}

static void spawn_particle(int32_t x, int32_t y) {
    for (int32_t i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particle_t* p = &particles[i];
            p->active   = true;
            p->x        = x << 4;
            p->y        = y << 4;
            p->vx       = prng_range(-6, 6);
            p->vy       = prng_range(-10, -1);
            uint8_t t   = (uint8_t)(prng() % 3);
            if      (t == 0) { p->r = 200; p->g = 0;   p->b = 0;   }
            else if (t == 1) { p->r = 120; p->g = 0;   p->b = 180; }
            else             { p->r = 220; p->g = 180; p->b = 255; }
            p->max_life = (uint8_t)prng_range(60, 180);
            p->life     = p->max_life;
            p->alpha    = (uint8_t)prng_range(80, 180);
            p->size     = (uint8_t)prng_range(1, 3);
            return;
        }
    }
}

// ── Depth of field — box blur on a scanline ───────────────────────────────────
#define DOF_SCRATCH_W 1920
static uint32_t dof_scratch[DOF_SCRATCH_W];

static void dof_blur_row(int32_t y, int32_t radius) {
    if (radius <= 0 || y < 0 || y >= screen_h) return;
    int32_t w = screen_w;
    for (int32_t x = 0; x < w; x++)
        dof_scratch[x] = fb_get_pixel((uint32_t)x, (uint32_t)y);
    for (int32_t x = 0; x < w; x++) {
        int32_t lo = x - radius; if (lo < 0) lo = 0;
        int32_t hi = x + radius; if (hi >= w) hi = w - 1;
        int32_t cnt = hi - lo + 1;
        uint32_t sr = 0, sg = 0, sb = 0;
        for (int32_t k = lo; k <= hi; k++) {
            uint32_t c = dof_scratch[k];
            sr += (c >> 16) & 0xFF;
            sg += (c >>  8) & 0xFF;
            sb +=  c        & 0xFF;
        }
        fb_put_pixel((uint32_t)x, (uint32_t)y,
            ((sr/(uint32_t)cnt) << 16) |
            ((sg/(uint32_t)cnt) <<  8) |
             (sb/(uint32_t)cnt));
    }
}

void vfx_apply_dof(int32_t y_start, int32_t y_end, int32_t radius) {
    for (int32_t y = y_start; y < y_end; y++)
        dof_blur_row(y, radius);
}

// ── Public API ────────────────────────────────────────────────────────────────
void vfx_init(int32_t sw, int32_t sh) {
    screen_w = sw;
    screen_h = sh;
    for (int32_t i = 0; i < MAX_PARTICLES; i++)
        particles[i].active = false;
    for (int32_t i = 0; i < 80; i++)
        spawn_particle(prng_range(0, sw), prng_range(0, sh));
}

void vfx_set_mouse(int32_t x, int32_t y) {
    mouse_x = x;
    mouse_y = y;
}

void vfx_bob_enable(bool en)         { bob_on  = en; }
void vfx_bob_set_amplitude(int32_t a){ bob_amp  = a; }

int32_t vfx_get_bob_x(void) {
    if (!bob_on) return 0;
    return (vfx_cos(bob_angle) * bob_amp) / 32768;
}

int32_t vfx_get_bob_y(void) {
    if (!bob_on) return 0;
    return (vfx_sin((bob_angle * 2) & 255) * (bob_amp / 2)) / 32768;
}

void vfx_tick(void) {
    tick++;
    if (bob_on) bob_angle = (bob_angle + 1) & 255;

    // Spawn particles near mouse
    if (tick % 3 == 0)
        spawn_particle(mouse_x + prng_range(-25, 25),
                       mouse_y + prng_range(-25, 25));
    // Ambient particles
    if (tick % 8 == 0)
        spawn_particle(prng_range(0, screen_w),
                       prng_range(screen_h / 3, screen_h));

    // Update particles
    for (int32_t i = 0; i < MAX_PARTICLES; i++) {
        particle_t* p = &particles[i];
        if (!p->active) continue;

        int32_t px = p->x >> 4;
        int32_t py = p->y >> 4;
        int32_t mdx = mouse_x - px;
        int32_t mdy = mouse_y - py;
        int32_t d2  = mdx*mdx + mdy*mdy;

        if (d2 > 0 && d2 < 80*80) {
            // Repel within 80px
            p->vx -= (mdx << 4) / (d2 / 16 + 1);
            p->vy -= (mdy << 4) / (d2 / 16 + 1);
        } else if (d2 < 200*200) {
            // Gentle attract 80-200px
            p->vx += (mdx << 2) / (d2 / 64 + 1);
            p->vy += (mdy << 2) / (d2 / 64 + 1);
        }

        p->vx = p->vx * 15 / 16;
        p->vy = p->vy * 15 / 16;
        p->vy -= 1;  // upward float

        p->x += p->vx;
        p->y += p->vy;

        if (p->life > 0) p->life--;
        else { p->active = false; continue; }

        // Wrap horizontal
        if ((p->x >> 4) < -10) p->x = (screen_w + 10) << 4;
        if ((p->x >> 4) > screen_w + 10) p->x = -10 << 4;
        if ((p->y >> 4) < -10 || (p->y >> 4) > screen_h + 10)
            p->active = false;
    }
}

void vfx_draw_particles(void) {
    for (int32_t i = 0; i < MAX_PARTICLES; i++) {
        particle_t* p = &particles[i];
        if (!p->active) continue;
        int32_t px = p->x >> 4;
        int32_t py = p->y >> 4;
        uint8_t a  = (uint8_t)((uint32_t)p->alpha * p->life / p->max_life);
        if (a == 0) continue;
        uint32_t color = ((uint32_t)p->r << 16) | ((uint32_t)p->g << 8) | p->b;
        int32_t  sz    = p->size;
        for (int32_t dy = -sz; dy <= sz; dy++) {
            for (int32_t dx = -sz; dx <= sz; dx++) {
                if (dx*dx + dy*dy > sz*sz) continue;
                int32_t fx = px + dx;
                int32_t fy = py + dy;
                if (fx < 0 || fy < 0 || fx >= screen_w || fy >= screen_h) continue;
                int32_t d2 = dx*dx + dy*dy;
                uint8_t fa = (sz > 0) ?
                    (uint8_t)((uint32_t)a * (uint32_t)(sz*sz - d2) / (uint32_t)(sz*sz + 1)) : a;
                if (fa == 0) continue;
                uint32_t dst = fb_get_pixel((uint32_t)fx, (uint32_t)fy);
                uint32_t dr = (dst >> 16) & 0xFF;
                uint32_t dg = (dst >>  8) & 0xFF;
                uint32_t db =  dst        & 0xFF;
                uint32_t ia = 255 - fa;
                fb_put_pixel((uint32_t)fx, (uint32_t)fy,
                    (((p->r * fa + dr * ia) >> 8) << 16) |
                    (((p->g * fa + dg * ia) >> 8) <<  8) |
                     ((p->b * fa + db * ia) >> 8));
                (void)color;
            }
        }
    }
}
