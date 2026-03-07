// =============================================================================
// Instinct Engine — Native Game Engine for Instinct OS
// Pure C — no dependencies, no black boxes, no waiting for patches
// Runs directly on the Raven framebuffer
//
// Architecture inspired by:
//   - UECanvasGui  (rendering API design)
//   - GamePlay3D   (scene graph model)
//   - Ouzel        (component system)
//   - Needs-Based AI (entity behavior)
//
// CORVUS is the game director — it can spawn entities, run scripts,
// and modify the scene through the MCP tool layer.
// =============================================================================

#include "../include/instinct_engine.h"
#include "../include/framebuffer.h"
#include "../include/font.h"
#include "../include/corvus_brain.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── Engine state ──────────────────────────────────────────────────────────────
static raven_scene_t  g_scene;
static instinct_engine_t g_engine;
static bool           g_engine_ready = false;

// ── Fixed-point math helpers (no FPU needed in kernel) ───────────────────────
static int32_t fp_abs(int32_t x)  { return x < 0 ? -x : x; }
static int32_t fp_min(int32_t a, int32_t b) { return a < b ? a : b; }
static int32_t fp_max(int32_t a, int32_t b) { return a > b ? a : b; }
static int32_t fp_clamp(int32_t v, int32_t lo, int32_t hi) {
    return fp_max(lo, fp_min(hi, v));
}

// ── Simple sqrt approximation (Newton-Raphson, integer) ──────────────────────
static int32_t isqrt(int32_t n) {
    if (n <= 0) return 0;
    int32_t x = n, y = 1;
    while (x > y) { x = (x + y) / 2; y = n / x; }
    return x;
}

// ── String helpers ────────────────────────────────────────────────────────────
static int raven_strlen(const char* s) {
    int n = 0; while (s && s[n]) n++; return n;
}
static void raven_strcpy(char* d, const char* s, int max) {
    int i = 0;
    while (s && s[i] && i < max - 1) { d[i] = s[i]; i++; }
    d[i] = '\0';
}

// ── Initialize the engine ─────────────────────────────────────────────────────
void engine_init(void) {
    g_engine.entity_count  = 0;
    g_engine.camera_x      = 0;
    g_engine.camera_y      = 0;
    g_engine.frame_count   = 0;
    g_engine.paused        = false;
    g_engine.target_fps    = 60;
    g_engine.gravity_x     = 0;
    g_engine.gravity_y     = 2;    // Downward gravity (pixels/tick^2 * 100)

    for (int i = 0; i < ENGINE_MAX_ENTITIES; i++) {
        g_engine.entities[i].active = false;
    }

    // Initialize scene
    g_scene.entity_count = 0;
    g_scene.bg_color     = 0x050505;
    raven_strcpy(g_scene.name, "RavenScene", 32);

    g_engine_ready = true;
}

// ── Spawn an entity ───────────────────────────────────────────────────────────
int engine_spawn(const char* name, int32_t x, int32_t y,
                 int32_t w, int32_t h, entity_type_t type) {
    if (!g_engine_ready) return -1;

    int slot = -1;
    for (int i = 0; i < ENGINE_MAX_ENTITIES; i++) {
        if (!g_engine.entities[i].active) { slot = i; break; }
    }
    if (slot < 0) return -1;

    raven_entity_t* e = &g_engine.entities[slot];
    e->active    = true;
    e->visible   = true;
    e->type      = type;
    e->x         = x;
    e->y         = y;
    e->w         = w;
    e->h         = h;
    e->vx        = 0;
    e->vy        = 0;
    e->color     = 0xCC0000;   // Default: Akatsuki crimson
    e->hp        = 100;
    e->max_hp    = 100;
    e->grounded  = false;
    e->collidable = (type != ENTITY_PARTICLE);
    e->gravity   = (type == ENTITY_PLAYER || type == ENTITY_ENEMY ||
                    type == ENTITY_PROJECTILE);
    e->anim_frame = 0;
    e->anim_timer = 0;
    raven_strcpy(e->name, name, ENTITY_NAME_LEN);

    g_engine.entity_count++;
    return slot;
}

// ── Destroy an entity ─────────────────────────────────────────────────────────
void engine_destroy(int entity_id) {
    if (entity_id < 0 || entity_id >= ENGINE_MAX_ENTITIES) return;
    g_engine.entities[entity_id].active = false;
    g_engine.entity_count--;
}

// ── Set entity velocity ───────────────────────────────────────────────────────
void engine_set_velocity(int entity_id, int32_t vx, int32_t vy) {
    if (entity_id < 0 || entity_id >= ENGINE_MAX_ENTITIES) return;
    if (!g_engine.entities[entity_id].active) return;
    g_engine.entities[entity_id].vx = vx;
    g_engine.entities[entity_id].vy = vy;
}

// ── Set entity color ──────────────────────────────────────────────────────────
void engine_set_color(int entity_id, uint32_t color) {
    if (entity_id < 0 || entity_id >= ENGINE_MAX_ENTITIES) return;
    g_engine.entities[entity_id].color = color;
}

// ── AABB collision detection ──────────────────────────────────────────────────
static bool aabb_overlap(raven_entity_t* a, raven_entity_t* b) {
    return !(a->x + a->w <= b->x ||
             b->x + b->w <= a->x ||
             a->y + a->h <= b->y ||
             b->y + b->h <= a->y);
}

// ── Physics update ────────────────────────────────────────────────────────────
static void engine_update_physics(void) {
    fb_info_t* fb = fb_get_info();
    int32_t floor_y = fb ? (int32_t)fb->height - 80 : 500;

    for (int i = 0; i < ENGINE_MAX_ENTITIES; i++) {
        raven_entity_t* e = &g_engine.entities[i];
        if (!e->active || !e->gravity) continue;

        // Apply gravity
        if (!e->grounded) {
            e->vy += g_engine.gravity_y;
        }

        // Apply velocity
        e->x += e->vx / 10;
        e->y += e->vy / 10;

        // Floor collision
        if (e->y + e->h >= floor_y) {
            e->y      = floor_y - e->h;
            e->vy     = 0;
            e->grounded = true;
        } else {
            e->grounded = false;
        }

        // Screen bounds (horizontal)
        fb_info_t* fbi = fb_get_info();
        if (fbi) {
            if (e->x < 0) { e->x = 0; e->vx = fp_abs(e->vx); }
            if (e->x + e->w > (int32_t)fbi->width) {
                e->x = (int32_t)fbi->width - e->w;
                e->vx = -fp_abs(e->vx);
            }
        }

        // Friction
        e->vx = e->vx * 9 / 10;
    }

    // Entity-entity collision
    for (int i = 0; i < ENGINE_MAX_ENTITIES; i++) {
        raven_entity_t* a = &g_engine.entities[i];
        if (!a->active || !a->collidable) continue;

        for (int j = i + 1; j < ENGINE_MAX_ENTITIES; j++) {
            raven_entity_t* b = &g_engine.entities[j];
            if (!b->active || !b->collidable) continue;

            if (aabb_overlap(a, b)) {
                // Simple bounce response
                int32_t tmp_vx = a->vx;
                int32_t tmp_vy = a->vy;
                a->vx = b->vx / 2;
                a->vy = b->vy / 2;
                b->vx = tmp_vx / 2;
                b->vy = tmp_vy / 2;

                // Projectile damages enemies
                if (a->type == ENTITY_PROJECTILE && b->type == ENTITY_ENEMY) {
                    b->hp -= 25;
                    if (b->hp <= 0) engine_destroy(j);
                    engine_destroy(i);
                }
                if (b->type == ENTITY_PROJECTILE && a->type == ENTITY_ENEMY) {
                    a->hp -= 25;
                    if (a->hp <= 0) engine_destroy(i);
                    engine_destroy(j);
                }
            }
        }
    }
}

// ── Render all entities ───────────────────────────────────────────────────────
static void engine_render_entities(void) {
    // Clear scene (draw background)
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    // Draw floor line
    fb_draw_hline(0, fb->height - 80, fb->width, 0x330000);
    fb_draw_hline(0, fb->height - 79, fb->width, 0x1A0000);

    for (int i = 0; i < ENGINE_MAX_ENTITIES; i++) {
        raven_entity_t* e = &g_engine.entities[i];
        if (!e->active || !e->visible) continue;

        int32_t rx = e->x - g_engine.camera_x;
        int32_t ry = e->y - g_engine.camera_y;

        switch (e->type) {
            case ENTITY_PLAYER:
                // Draw player as a colored rectangle with outline
                fb_fill_rect(rx, ry, e->w, e->h, e->color);
                fb_draw_rect(rx, ry, e->w, e->h, 2, 0xFF4444);
                // Eyes
                fb_fill_rect(rx + 4,  ry + 4, 4, 4, 0xFF0000);
                fb_fill_rect(rx + e->w - 8, ry + 4, 4, 4, 0xFF0000);
                // HP bar above
                engine_draw_hp_bar(rx, ry - 8, e->w, e->hp, e->max_hp);
                // Name
                font_draw_string(rx, ry - 18, e->name, 0xFF6666, 0, true);
                break;

            case ENTITY_ENEMY:
                fb_fill_rect(rx, ry, e->w, e->h, 0x440000);
                fb_draw_rect(rx, ry, e->w, e->h, 1, 0x880000);
                // Menacing eyes
                fb_fill_rect(rx + 3,  ry + 5, 5, 5, 0xFF2200);
                fb_fill_rect(rx + e->w - 8, ry + 5, 5, 5, 0xFF2200);
                engine_draw_hp_bar(rx, ry - 8, e->w, e->hp, e->max_hp);
                break;

            case ENTITY_PROJECTILE:
                fb_fill_circle(rx + e->w/2, ry + e->h/2, e->w/2, 0xFF6600);
                break;

            case ENTITY_PLATFORM:
                fb_fill_rect(rx, ry, e->w, e->h, 0x1A0000);
                fb_draw_rect(rx, ry, e->w, e->h, 1, 0x440000);
                break;

            case ENTITY_PARTICLE: {
                // Fade particle based on HP (used as lifetime)
                uint32_t alpha = (uint32_t)(e->hp * 255 / e->max_hp);
                uint32_t col = (alpha << 16) | (alpha / 4);
                fb_fill_rect(rx, ry, e->w, e->h, col);
                e->hp--;
                if (e->hp <= 0) engine_destroy(i);
                break;
            }

            case ENTITY_TRIGGER:
                // Invisible — just a collision zone
                break;

            case ENTITY_UI:
                fb_fill_rect(rx, ry, e->w, e->h, e->color);
                font_draw_centered(rx + e->w/2, ry + (e->h - FONT_HEIGHT)/2,
                                   e->name, 0xFFFFFF, 0, true);
                break;
        }
    }
}

// ── Draw HP bar ───────────────────────────────────────────────────────────────
void engine_draw_hp_bar(int32_t x, int32_t y, int32_t w,
                         int32_t hp, int32_t max_hp) {
    if (max_hp <= 0) return;
    fb_fill_rect(x, y, w, 5, 0x1A0000);
    int32_t fill = w * hp / max_hp;
    uint32_t col = (hp > max_hp * 6/10) ? 0x00CC44 :
                   (hp > max_hp * 3/10) ? 0xFFAA00 : 0xFF2222;
    if (fill > 0) fb_fill_rect(x, y, fill, 5, col);
    fb_draw_rect(x, y, w, 5, 1, 0x330000);
}

// ── Spawn a particle burst ────────────────────────────────────────────────────
void engine_spawn_particles(int32_t x, int32_t y, int count, uint32_t color) {
    (void)color;
    for (int i = 0; i < count && i < 8; i++) {
        int id = engine_spawn("particle", x + (i * 3 - 12), y, 3, 3, ENTITY_PARTICLE);
        if (id >= 0) {
            g_engine.entities[id].vx = (i - 4) * 15;
            g_engine.entities[id].vy = -(20 + i * 5);
            g_engine.entities[id].hp = 30 + i * 5;
            g_engine.entities[id].max_hp = 60;
            g_engine.entities[id].gravity = true;
        }
    }
}

// ── Main engine tick — call this every frame ──────────────────────────────────
void engine_tick(void) {
    if (!g_engine_ready || g_engine.paused) return;

    engine_update_physics();
    engine_render_entities();
    g_engine.frame_count++;

    // CORVUS game director tick — every 60 frames
    if (g_engine.frame_count % 60 == 0) {
        corvus_brain_tick();
    }
}

// ── Demo scene: Instinct OS splash game ────────────────────────────────────────
// Spawns a player, some enemies, and platforms to show the engine working
void engine_demo_scene(void) {
    fb_info_t* fb = fb_get_info();
    if (!fb) return;

    int32_t W = (int32_t)fb->width;
    int32_t H = (int32_t)fb->height;

    // Player
    int player = engine_spawn("CORVUS", W/2 - 16, H/2, 32, 48, ENTITY_PLAYER);
    engine_set_color(player, 0x8B0000);
    engine_set_velocity(player, 0, 0);

    // Enemies
    for (int i = 0; i < 3; i++) {
        int e = engine_spawn("Akatsuki", 100 + i * 200, H - 200, 28, 40, ENTITY_ENEMY);
        engine_set_color(e, 0x440000);
        engine_set_velocity(e, (i % 2 == 0) ? 8 : -8, 0);
    }

    // Platforms
    engine_spawn("platform_1", 100,       H - 200, 200, 16, ENTITY_PLATFORM);
    engine_spawn("platform_2", 400,       H - 300, 160, 16, ENTITY_PLATFORM);
    engine_spawn("platform_3", W - 300,   H - 250, 200, 16, ENTITY_PLATFORM);
    engine_spawn("platform_4", W/2 - 80,  H - 380, 160, 16, ENTITY_PLATFORM);

    // UI elements
    int title = engine_spawn("Instinct OS", 10, 10, 200, 24, ENTITY_UI);
    engine_set_color(title, 0x1A0000);

    // Particle burst at center
    engine_spawn_particles(W/2, H/2, 8, 0xCC0000);
}

// ── Getters ───────────────────────────────────────────────────────────────────
raven_entity_t* engine_get_entity(int entity_id) {
    if (entity_id < 0 || entity_id >= ENGINE_MAX_ENTITIES) return NULL;
    return g_engine.entities[entity_id].active ? &g_engine.entities[entity_id] : NULL;
}

int engine_get_entity_count(void) { return g_engine.entity_count; }
uint64_t engine_get_frame(void)   { return g_engine.frame_count; }
bool engine_is_ready(void)        { return g_engine_ready; }

void engine_set_camera(int32_t x, int32_t y) {
    g_engine.camera_x = x;
    g_engine.camera_y = y;
}

void engine_pause(bool paused) { g_engine.paused = paused; }
