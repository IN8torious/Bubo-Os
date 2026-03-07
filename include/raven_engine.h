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

#ifndef RAVEN_ENGINE_H
#define RAVEN_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define ENGINE_MAX_ENTITIES  128
#define ENTITY_NAME_LEN       32

// ── Entity types ──────────────────────────────────────────────────────────────
typedef enum {
    ENTITY_PLAYER = 0,
    ENTITY_ENEMY,
    ENTITY_PROJECTILE,
    ENTITY_PLATFORM,
    ENTITY_PARTICLE,
    ENTITY_TRIGGER,
    ENTITY_UI
} entity_type_t;

// ── Entity ────────────────────────────────────────────────────────────────────
typedef struct {
    bool          active;
    bool          visible;
    bool          collidable;
    bool          gravity;
    bool          grounded;
    entity_type_t type;
    char          name[ENTITY_NAME_LEN];
    int32_t       x, y;
    int32_t       w, h;
    int32_t       vx, vy;
    uint32_t      color;
    int32_t       hp, max_hp;
    int32_t       anim_frame;
    int32_t       anim_timer;
} raven_entity_t;

// ── Scene ─────────────────────────────────────────────────────────────────────
typedef struct {
    char     name[32];
    int      entity_count;
    uint32_t bg_color;
} raven_scene_t;

// ── Engine ────────────────────────────────────────────────────────────────────
typedef struct {
    raven_entity_t entities[ENGINE_MAX_ENTITIES];
    int            entity_count;
    int32_t        camera_x, camera_y;
    uint64_t       frame_count;
    bool           paused;
    int            target_fps;
    int32_t        gravity_x, gravity_y;
} raven_engine_t;

// ── API ───────────────────────────────────────────────────────────────────────
void            engine_init(void);
int             engine_spawn(const char* name, int32_t x, int32_t y,
                             int32_t w, int32_t h, entity_type_t type);
void            engine_destroy(int entity_id);
void            engine_set_velocity(int entity_id, int32_t vx, int32_t vy);
void            engine_set_color(int entity_id, uint32_t color);
void            engine_tick(void);
void            engine_demo_scene(void);
void            engine_draw_hp_bar(int32_t x, int32_t y, int32_t w,
                                   int32_t hp, int32_t max_hp);
void            engine_spawn_particles(int32_t x, int32_t y,
                                       int count, uint32_t color);
void            engine_set_camera(int32_t x, int32_t y);
void            engine_pause(bool paused);
raven_entity_t* engine_get_entity(int entity_id);
int             engine_get_entity_count(void);
uint64_t        engine_get_frame(void);
bool            engine_is_ready(void);

#endif // RAVEN_ENGINE_H
