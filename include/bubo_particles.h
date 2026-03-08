/*
 * bubo_particles.h — BUBO OS Desktop Particle System
 * ─────────────────────────────────────────────────────────────────────────────
 * Rain, lightning, snow, mist — driven by real weather + Vera Workflow tasks.
 *
 * Inspired by Epic Games / Unreal Engine's real-time particle philosophy:
 * the world should feel alive. Every particle is a data point.
 * The storm is not decoration. The storm is information.
 *
 * When Boo is idle       → calm Akatsuki dawn, still ocean
 * When Boo is working    → clouds darken, rain begins
 * When Boo works hard    → full storm, lightning, heavy rain
 * When Boo finishes      → storm fades, dawn returns
 *
 * Credits (they all earned it):
 *   Epic Games      — particle system philosophy
 *   OpenWeatherMap  — real weather data
 *   NASA + Google   — the world behind the window
 *
 * NO MAS DISADVANTAGED
 * ─────────────────────────────────────────────────────────────────────────────
 */

#ifndef BUBO_PARTICLES_H
#define BUBO_PARTICLES_H

#include <stdint.h>
#include "bubo_weather.h"

/* ── Particle types ────────────────────────────────────────────────────────── */
typedef enum {
    PARTICLE_RAIN       = 0,
    PARTICLE_SNOW       = 1,
    PARTICLE_LIGHTNING  = 2,
    PARTICLE_MIST       = 3,
    PARTICLE_SPLASH     = 4    /* Ocean surface splash when rain hits */
} bubo_particle_type_t;

/* ── Single particle ───────────────────────────────────────────────────────── */
typedef struct {
    float               x, y;           /* Screen position (pixels)            */
    float               vx, vy;         /* Velocity (pixels/frame)             */
    float               alpha;          /* Opacity 0.0 - 1.0                   */
    float               length;         /* Rain streak length                  */
    float               size;           /* Snow flake / splash size            */
    bubo_particle_type_t type;
    uint8_t             r, g, b;        /* Color                               */
    int                 active;         /* 1 = alive, 0 = dead/recycled        */
} bubo_particle_t;

/* ── Lightning bolt ────────────────────────────────────────────────────────── */
typedef struct {
    float   x1, y1;        /* Start point (in the clouds)                     */
    float   x2, y2;        /* End point (ocean surface or mid-air)            */
    float   alpha;         /* Fade out over lifetime                          */
    float   lifetime;      /* Frames remaining                                */
    float   max_lifetime;  /* Total frames for this bolt                      */
    int     active;
} bubo_lightning_t;

/* ── Particle system config ────────────────────────────────────────────────── */
#define BUBO_MAX_PARTICLES      2048
#define BUBO_MAX_LIGHTNING      8
#define BUBO_SCREEN_W           1920
#define BUBO_SCREEN_H           1080
#define BUBO_PARTICLE_FPS       60

/* ── Particle system state ─────────────────────────────────────────────────── */
typedef struct {
    bubo_particle_t     particles[BUBO_MAX_PARTICLES];
    bubo_lightning_t    lightning[BUBO_MAX_LIGHTNING];
    int                 active_count;
    int                 lightning_count;
    float               spawn_accumulator;  /* Fractional particle spawn       */
    uint32_t            frame;              /* Frame counter                   */
    bubo_visual_intensity_t intensity;      /* Current visual params           */
} bubo_particle_system_t;

/* ── Function declarations ─────────────────────────────────────────────────── */

/**
 * bubo_particles_init — Initialize the particle system
 */
void bubo_particles_init(bubo_particle_system_t *ps);

/**
 * bubo_particles_update — Update all particles for one frame
 * Call at BUBO_PARTICLE_FPS (60Hz) from the desktop render loop.
 * @intensity: current visual intensity from bubo_weather_get_visual()
 */
void bubo_particles_update(bubo_particle_system_t *ps,
                           const bubo_visual_intensity_t *intensity);

/**
 * bubo_particles_render — Render all particles to the framebuffer
 * In Qt: renders to a QImage overlay on top of the wallpaper.
 */
void bubo_particles_render(const bubo_particle_system_t *ps,
                           uint32_t *framebuffer,
                           int width, int height);

/**
 * bubo_particles_trigger_lightning — Manually trigger a lightning bolt
 * Called automatically when intensity.lightning_freq is high enough.
 */
void bubo_particles_trigger_lightning(bubo_particle_system_t *ps);

/**
 * bubo_particles_set_intensity — Update visual intensity from weather system
 * Called when weather updates or Vera task intensity changes.
 */
void bubo_particles_set_intensity(bubo_particle_system_t *ps,
                                  const bubo_visual_intensity_t *intensity);

/* ── Global particle system instance ──────────────────────────────────────── */
extern bubo_particle_system_t g_bubo_particles;

#endif /* BUBO_PARTICLES_H */
