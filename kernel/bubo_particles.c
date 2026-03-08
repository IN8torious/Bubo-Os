/*
 * bubo_particles.c — BUBO OS Desktop Particle System Implementation
 * ─────────────────────────────────────────────────────────────────────────────
 * Rain, lightning, snow, mist — the desktop is alive.
 *
 * NO MAS DISADVANTAGED
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../include/bubo_particles.h"
#include <stddef.h>   /* must precede stdlib.h — defines wchar_t */
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ── Global instance ───────────────────────────────────────────────────────── */
bubo_particle_system_t g_bubo_particles = {0};

/* ── Simple LCG pseudo-random (no stdlib rand dependency in kernel) ────────── */
static uint32_t s_seed = 0xDEADBEEF;
static float ps_rand(void)
{
    s_seed = s_seed * 1664525u + 1013904223u;
    return (float)(s_seed & 0x7FFF) / (float)0x7FFF;
}
static float ps_rand_range(float lo, float hi)
{
    return lo + ps_rand() * (hi - lo);
}

/* ── Init ──────────────────────────────────────────────────────────────────── */
void bubo_particles_init(bubo_particle_system_t *ps)
{
    memset(ps, 0, sizeof(*ps));
}

/* ── Spawn a rain particle ─────────────────────────────────────────────────── */
static void spawn_rain(bubo_particle_t *p, const bubo_visual_intensity_t *v)
{
    p->type   = PARTICLE_RAIN;
    p->x      = ps_rand_range(0.0f, BUBO_SCREEN_W);
    p->y      = ps_rand_range(-60.0f, -5.0f);
    p->vx     = ps_rand_range(-1.5f, -0.5f) * v->wind_intensity * 3.0f;
    p->vy     = ps_rand_range(8.0f, 18.0f);
    p->alpha  = ps_rand_range(0.3f, 0.7f);
    p->length = ps_rand_range(8.0f, 22.0f);
    p->r      = 140; p->g = 160; p->b = 200;
    p->active = 1;
}

/* ── Spawn a snow particle ─────────────────────────────────────────────────── */
static void spawn_snow(bubo_particle_t *p, const bubo_visual_intensity_t *v)
{
    p->type   = PARTICLE_SNOW;
    p->x      = ps_rand_range(0.0f, BUBO_SCREEN_W);
    p->y      = ps_rand_range(-20.0f, -5.0f);
    p->vx     = ps_rand_range(-1.0f, 1.0f) + v->wind_intensity * 2.0f;
    p->vy     = ps_rand_range(1.5f, 4.0f);
    p->alpha  = ps_rand_range(0.5f, 0.9f);
    p->size   = ps_rand_range(2.0f, 5.0f);
    p->r      = 220; p->g = 230; p->b = 255;
    p->active = 1;
}

/* ── Spawn a mist particle ─────────────────────────────────────────────────── */
static void spawn_mist(bubo_particle_t *p)
{
    p->type   = PARTICLE_MIST;
    p->x      = ps_rand_range(0.0f, BUBO_SCREEN_W);
    p->y      = ps_rand_range(BUBO_SCREEN_H * 0.4f, BUBO_SCREEN_H * 0.7f);
    p->vx     = ps_rand_range(-0.3f, 0.3f);
    p->vy     = ps_rand_range(-0.1f, 0.1f);
    p->alpha  = ps_rand_range(0.05f, 0.15f);
    p->size   = ps_rand_range(80.0f, 200.0f);
    p->r      = 180; p->g = 180; p->b = 200;
    p->active = 1;
}

/* ── Trigger a lightning bolt ──────────────────────────────────────────────── */
void bubo_particles_trigger_lightning(bubo_particle_system_t *ps)
{
    for (int i = 0; i < BUBO_MAX_LIGHTNING; i++) {
        if (!ps->lightning[i].active) {
            bubo_lightning_t *bolt = &ps->lightning[i];
            bolt->x1 = ps_rand_range(BUBO_SCREEN_W * 0.1f, BUBO_SCREEN_W * 0.9f);
            bolt->y1 = ps_rand_range(0.0f, BUBO_SCREEN_H * 0.3f);
            bolt->x2 = bolt->x1 + ps_rand_range(-150.0f, 150.0f);
            bolt->y2 = ps_rand_range(BUBO_SCREEN_H * 0.4f, BUBO_SCREEN_H * 0.7f);
            bolt->max_lifetime = ps_rand_range(4.0f, 12.0f);
            bolt->lifetime     = bolt->max_lifetime;
            bolt->alpha        = 1.0f;
            bolt->active       = 1;
            ps->lightning_count++;
            return;
        }
    }
}

/* ── Set intensity ─────────────────────────────────────────────────────────── */
void bubo_particles_set_intensity(bubo_particle_system_t *ps,
                                   const bubo_visual_intensity_t *intensity)
{
    ps->intensity = *intensity;
}

/* ── Update one frame ──────────────────────────────────────────────────────── */
void bubo_particles_update(bubo_particle_system_t *ps,
                            const bubo_visual_intensity_t *intensity)
{
    ps->intensity = *intensity;
    ps->frame++;

    /* ── Determine spawn rate from intensity ─────────────────────────────── */
    float rain_rate  = intensity->rain_density  * 12.0f;  /* particles/frame  */
    float snow_rate  = 0.0f;
    float mist_rate  = intensity->fog_density   * 0.5f;

    /* Snow only when conditions are right (handled by weather state upstream) */
    /* For now rain_rate drives both rain and snow based on weather state      */

    /* ── Spawn new particles ─────────────────────────────────────────────── */
    ps->spawn_accumulator += rain_rate;
    int to_spawn = (int)ps->spawn_accumulator;
    ps->spawn_accumulator -= (float)to_spawn;

    int spawned = 0;
    for (int i = 0; i < BUBO_MAX_PARTICLES && spawned < to_spawn; i++) {
        if (!ps->particles[i].active) {
            spawn_rain(&ps->particles[i], intensity);
            spawned++;
            ps->active_count++;
        }
    }

    /* Mist */
    if (ps_rand() < mist_rate) {
        for (int i = 0; i < BUBO_MAX_PARTICLES; i++) {
            if (!ps->particles[i].active) {
                spawn_mist(&ps->particles[i]);
                ps->active_count++;
                break;
            }
        }
    }

    /* ── Update existing particles ───────────────────────────────────────── */
    for (int i = 0; i < BUBO_MAX_PARTICLES; i++) {
        bubo_particle_t *p = &ps->particles[i];
        if (!p->active) continue;

        p->x += p->vx;
        p->y += p->vy;

        /* Mist drifts and fades */
        if (p->type == PARTICLE_MIST) {
            p->alpha -= 0.0005f;
            if (p->alpha <= 0.0f) { p->active = 0; ps->active_count--; }
            continue;
        }

        /* Kill particles that leave the screen */
        if (p->y > BUBO_SCREEN_H + 30.0f ||
            p->x < -50.0f || p->x > BUBO_SCREEN_W + 50.0f) {
            p->active = 0;
            ps->active_count--;
        }
    }

    /* ── Update lightning ────────────────────────────────────────────────── */
    for (int i = 0; i < BUBO_MAX_LIGHTNING; i++) {
        bubo_lightning_t *bolt = &ps->lightning[i];
        if (!bolt->active) continue;

        bolt->lifetime -= 1.0f;
        bolt->alpha = bolt->lifetime / bolt->max_lifetime;

        if (bolt->lifetime <= 0.0f) {
            bolt->active = 0;
            ps->lightning_count--;
        }
    }

    /* ── Randomly trigger new lightning ─────────────────────────────────── */
    if (intensity->lightning_freq > 0.0f) {
        /* lightning_freq of 1.0 = ~1 bolt every 30 frames */
        float chance = intensity->lightning_freq * 0.033f;
        if (ps_rand() < chance) {
            bubo_particles_trigger_lightning(ps);
        }
    }
}

/* ── Render to framebuffer ─────────────────────────────────────────────────── */
/*
 * In the full kernel build this writes directly to the framebuffer.
 * In the Qt layer this is replaced by QPainter calls on a QImage overlay.
 * The function signature is kept identical so the Qt wrapper is a drop-in.
 */
void bubo_particles_render(const bubo_particle_system_t *ps,
                            uint32_t *fb,
                            int width, int height)
{
    if (!fb) return;

    /* ── Render rain streaks ─────────────────────────────────────────────── */
    for (int i = 0; i < BUBO_MAX_PARTICLES; i++) {
        const bubo_particle_t *p = &ps->particles[i];
        if (!p->active || p->type != PARTICLE_RAIN) continue;

        /* Draw a short diagonal line from (x,y) to (x+vx*len, y+vy*len) */
        float nx = -p->vy; float ny = p->vx;
        float mag = sqrtf(nx*nx + ny*ny);
        if (mag < 0.001f) continue;
        nx /= mag; ny /= mag;

        float steps = p->length;
        float dx = (p->vx / (p->vy + 0.001f));
        uint8_t a = (uint8_t)(p->alpha * 255.0f);

        for (int s = 0; s < (int)steps; s++) {
            int px = (int)(p->x + dx * s);
            int py = (int)(p->y + s);
            if (px < 0 || px >= width || py < 0 || py >= height) continue;
            uint32_t *pixel = &fb[py * width + px];
            /* Alpha blend */
            uint8_t sr = (*pixel >> 16) & 0xFF;
            uint8_t sg = (*pixel >>  8) & 0xFF;
            uint8_t sb = (*pixel      ) & 0xFF;
            uint8_t fa = a;
            sr = (uint8_t)((p->r * fa + sr * (255 - fa)) / 255);
            sg = (uint8_t)((p->g * fa + sg * (255 - fa)) / 255);
            sb = (uint8_t)((p->b * fa + sb * (255 - fa)) / 255);
            *pixel = (0xFF << 24) | (sr << 16) | (sg << 8) | sb;
        }
    }

    /* ── Render lightning ────────────────────────────────────────────────── */
    for (int i = 0; i < BUBO_MAX_LIGHTNING; i++) {
        const bubo_lightning_t *bolt = &ps->lightning[i];
        if (!bolt->active) continue;

        /* Bresenham line from (x1,y1) to (x2,y2) */
        int x0 = (int)bolt->x1, y0 = (int)bolt->y1;
        int x1 = (int)bolt->x2, y1 = (int)bolt->y2;
        int dx = abs(x1 - x0), dy = abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        uint8_t la = (uint8_t)(bolt->alpha * 255.0f);

        while (1) {
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
                /* Lightning is white-yellow, bright */
                uint32_t *pixel = &fb[y0 * width + x0];
                uint8_t lr = 255, lg = 255, lb = 200;
                uint8_t sr = (*pixel >> 16) & 0xFF;
                uint8_t sg = (*pixel >>  8) & 0xFF;
                uint8_t sb = (*pixel      ) & 0xFF;
                sr = (uint8_t)((lr * la + sr * (255 - la)) / 255);
                sg = (uint8_t)((lg * la + sg * (255 - la)) / 255);
                sb = (uint8_t)((lb * la + sb * (255 - la)) / 255);
                *pixel = (0xFF << 24) | (sr << 16) | (sg << 8) | sb;
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 <  dx) { err += dx; y0 += sy; }
        }
    }
}
