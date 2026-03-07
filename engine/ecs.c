// =============================================================================
// INSTINCT ENGINE — Entity Component System
// Instinct OS v1.1 | Built by IN8torious (Nathan Samuel — "Gift of God")
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/ecs.c — The heart of the INSTINCT ENGINE
// Every entity in every world — Ragnar, CORVUS, Landon, Raphael,
// Huginn the Raven, the Demon 170, the Road Runner, every building
// on the Nürburgring — is an ECS entity managed here.
//
// Pioneer & Inventor: IN8torious
// First constitutionally governed, accessibility-first sovereign OS
// built from bare metal with an embedded game engine and MAS.
// =============================================================================

#include "ecs.h"
#include "vmm.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── World state ───────────────────────────────────────────────────────────────
static raven_entity_t   g_entities[ECS_MAX_ENTITIES];
static uint32_t         g_entity_count = 0;
static uint32_t         g_tick         = 0;
static bool             g_initialized  = false;

// Component pools — flat arrays indexed by entity id
static transform_t      g_transforms[ECS_MAX_ENTITIES];
static render_comp_t    g_renders[ECS_MAX_ENTITIES];
static physics_comp_t   g_physics[ECS_MAX_ENTITIES];
static ai_comp_t        g_ai[ECS_MAX_ENTITIES];
static character_comp_t g_characters[ECS_MAX_ENTITIES];
static vehicle_comp_t   g_vehicles[ECS_MAX_ENTITIES];
static animal_comp_t    g_animals[ECS_MAX_ENTITIES];

// ── Math helpers ──────────────────────────────────────────────────────────────
static float fabs_f(float x)   { return x < 0.0f ? -x : x; }
static float clamp_f(float x, float mn, float mx) {
    return x < mn ? mn : (x > mx ? mx : x);
}
static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x+b.x, a.y+b.y, a.z+b.z};
}
static vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x*s, v.y*s, v.z*s};
}
static float vec3_dot(vec3_t a, vec3_t b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static float vec3_len(vec3_t v) {
    float d = vec3_dot(v,v);
    // Fast inverse sqrt approximation (Quake III)
    float x2 = d * 0.5f;
    float y  = d;
    uint32_t i;
    __builtin_memcpy(&i, &y, 4);
    i = 0x5f3759df - (i >> 1);
    __builtin_memcpy(&y, &i, 4);
    y = y * (1.5f - (x2 * y * y));
    return (d > 0.0f) ? (1.0f / y) : 0.0f;
}
static vec3_t vec3_norm(vec3_t v) {
    float l = vec3_len(v);
    if (l < 0.0001f) return (vec3_t){0,0,0};
    return vec3_scale(v, 1.0f/l);
}

// ── Entity management ─────────────────────────────────────────────────────────
void ecs_init(void) {
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
        g_entities[i].id      = i;
        g_entities[i].alive   = false;
        g_entities[i].mask    = 0;
        g_entities[i].name[0] = 0;
        g_entities[i].scene   = SCENE_NONE;
    }
    g_entity_count = 0;
    g_tick         = 0;
    g_initialized  = true;
    terminal_write("[ECS] INSTINCT ENGINE entity system initialized.\n");
    terminal_write("[ECS] Max entities: " ECS_MAX_STR "\n");
}

entity_id_t ecs_create(const char* name, scene_id_t scene) {
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (!g_entities[i].alive) {
            g_entities[i].alive   = true;
            g_entities[i].mask    = 0;
            g_entities[i].scene   = scene;
            // Copy name
            uint32_t j = 0;
            while (name[j] && j < ECS_NAME_LEN-1) {
                g_entities[i].name[j] = name[j]; j++;
            }
            g_entities[i].name[j] = 0;
            // Zero transform
            g_transforms[i] = (transform_t){
                .pos   = {0,0,0},
                .rot   = {0,0,0},
                .scale = {1,1,1}
            };
            g_entity_count++;
            return (entity_id_t)i;
        }
    }
    terminal_write("[ECS] ERROR: Entity pool exhausted!\n");
    return ECS_INVALID_ID;
}

void ecs_destroy(entity_id_t id) {
    if (id >= ECS_MAX_ENTITIES) return;
    g_entities[id].alive = false;
    g_entities[id].mask  = 0;
    if (g_entity_count > 0) g_entity_count--;
}

bool ecs_alive(entity_id_t id) {
    return id < ECS_MAX_ENTITIES && g_entities[id].alive;
}

// ── Component attachment ──────────────────────────────────────────────────────
transform_t* ecs_add_transform(entity_id_t id) {
    if (!ecs_alive(id)) return (transform_t*)0;
    g_entities[id].mask |= COMP_TRANSFORM;
    return &g_transforms[id];
}
render_comp_t* ecs_add_render(entity_id_t id) {
    if (!ecs_alive(id)) return (render_comp_t*)0;
    g_entities[id].mask |= COMP_RENDER;
    return &g_renders[id];
}
physics_comp_t* ecs_add_physics(entity_id_t id) {
    if (!ecs_alive(id)) return (physics_comp_t*)0;
    g_entities[id].mask |= COMP_PHYSICS;
    g_physics[id] = (physics_comp_t){
        .vel={0,0,0}, .accel={0,0,0},
        .mass=1.0f, .friction=0.95f,
        .grounded=false, .kinematic=false
    };
    return &g_physics[id];
}
ai_comp_t* ecs_add_ai(entity_id_t id) {
    if (!ecs_alive(id)) return (ai_comp_t*)0;
    g_entities[id].mask |= COMP_AI;
    g_ai[id].state      = AI_IDLE;
    g_ai[id].target_id  = ECS_INVALID_ID;
    g_ai[id].timer      = 0;
    return &g_ai[id];
}
character_comp_t* ecs_add_character(entity_id_t id) {
    if (!ecs_alive(id)) return (character_comp_t*)0;
    g_entities[id].mask |= COMP_CHARACTER;
    return &g_characters[id];
}
vehicle_comp_t* ecs_add_vehicle(entity_id_t id) {
    if (!ecs_alive(id)) return (vehicle_comp_t*)0;
    g_entities[id].mask |= COMP_VEHICLE;
    g_vehicles[id] = (vehicle_comp_t){
        .speed=0, .throttle=0, .brake=0, .steer=0,
        .gear=1, .rpm=800, .nitro=false, .nitro_fuel=100.0f
    };
    return &g_vehicles[id];
}
animal_comp_t* ecs_add_animal(entity_id_t id) {
    if (!ecs_alive(id)) return (animal_comp_t*)0;
    g_entities[id].mask |= COMP_ANIMAL;
    g_animals[id].state      = ANIMAL_IDLE;
    g_animals[id].owner_id   = ECS_INVALID_ID;
    g_animals[id].wander_timer = 0;
    return &g_animals[id];
}

// ── Component getters ─────────────────────────────────────────────────────────
transform_t*     ecs_get_transform(entity_id_t id)  { return ecs_alive(id) ? &g_transforms[id]  : (transform_t*)0; }
render_comp_t*   ecs_get_render(entity_id_t id)     { return ecs_alive(id) ? &g_renders[id]     : (render_comp_t*)0; }
physics_comp_t*  ecs_get_physics(entity_id_t id)    { return ecs_alive(id) ? &g_physics[id]     : (physics_comp_t*)0; }
ai_comp_t*       ecs_get_ai(entity_id_t id)         { return ecs_alive(id) ? &g_ai[id]          : (ai_comp_t*)0; }
character_comp_t*ecs_get_character(entity_id_t id)  { return ecs_alive(id) ? &g_characters[id]  : (character_comp_t*)0; }
vehicle_comp_t*  ecs_get_vehicle(entity_id_t id)    { return ecs_alive(id) ? &g_vehicles[id]    : (vehicle_comp_t*)0; }
animal_comp_t*   ecs_get_animal(entity_id_t id)     { return ecs_alive(id) ? &g_animals[id]     : (animal_comp_t*)0; }
raven_entity_t*  ecs_get_entity(entity_id_t id)     { return ecs_alive(id) ? &g_entities[id]    : (raven_entity_t*)0; }

// ── Physics system tick ───────────────────────────────────────────────────────
static void physics_tick(float dt) {
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (!g_entities[i].alive) continue;
        if (!(g_entities[i].mask & COMP_PHYSICS)) continue;
        if (!(g_entities[i].mask & COMP_TRANSFORM)) continue;

        physics_comp_t* p = &g_physics[i];
        transform_t*    t = &g_transforms[i];

        if (p->kinematic) continue;

        // Apply gravity
        if (!p->grounded) p->vel.y -= 9.8f * dt;

        // Apply acceleration
        p->vel = vec3_add(p->vel, vec3_scale(p->accel, dt));

        // Apply friction
        p->vel.x *= p->friction;
        p->vel.z *= p->friction;

        // Integrate position
        t->pos = vec3_add(t->pos, vec3_scale(p->vel, dt));

        // Ground clamp
        if (t->pos.y < 0.0f) {
            t->pos.y  = 0.0f;
            p->vel.y  = 0.0f;
            p->grounded = true;
        } else {
            p->grounded = (t->pos.y < 0.01f);
        }
    }
}

// ── Vehicle system tick ───────────────────────────────────────────────────────
static void vehicle_tick(float dt) {
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (!g_entities[i].alive) continue;
        if (!(g_entities[i].mask & COMP_VEHICLE)) continue;
        if (!(g_entities[i].mask & COMP_TRANSFORM)) continue;

        vehicle_comp_t* v = &g_vehicles[i];
        transform_t*    t = &g_transforms[i];

        // Engine RPM simulation
        float target_rpm = 800.0f + v->throttle * (v->spec.redline - 800.0f);
        v->rpm += (target_rpm - v->rpm) * 8.0f * dt;
        v->rpm  = clamp_f(v->rpm, 800.0f, v->spec.redline);

        // Torque → force
        float torque = v->spec.torque_nm * (v->throttle * 0.8f + 0.2f);
        float drive_force = torque / (v->spec.wheel_radius > 0 ? v->spec.wheel_radius : 0.35f);

        // Nitro boost
        if (v->nitro && v->nitro_fuel > 0.0f) {
            drive_force *= 2.5f;
            v->nitro_fuel -= 15.0f * dt;
            if (v->nitro_fuel < 0.0f) { v->nitro_fuel = 0.0f; v->nitro = false; }
        }

        // Drag
        float drag = 0.5f * 1.225f * v->spec.drag_coeff * v->spec.frontal_area
                     * v->speed * v->speed;

        // Net acceleration
        float mass = v->spec.mass_kg > 0 ? v->spec.mass_kg : 1500.0f;
        float net_force = drive_force - drag - v->brake * v->spec.brake_force;
        float accel = net_force / mass;

        v->speed += accel * dt;
        if (v->speed < 0.0f) v->speed = 0.0f;

        // Max speed cap
        float max_spd = v->spec.top_speed_ms;
        if (v->speed > max_spd) v->speed = max_spd;

        // Steering — yaw rotation
        if (fabs_f(v->speed) > 0.5f) {
            float steer_rate = v->steer * 1.8f * dt * (1.0f - v->speed / (max_spd * 2.0f));
            t->rot.y += steer_rate;
        }

        // Move forward in facing direction
        float yaw = t->rot.y;
        // Approximate sin/cos with Taylor series (no libm)
        float s = yaw - (yaw*yaw*yaw)/6.0f + (yaw*yaw*yaw*yaw*yaw)/120.0f;
        float c = 1.0f - (yaw*yaw)/2.0f + (yaw*yaw*yaw*yaw)/24.0f;
        t->pos.x += s * v->speed * dt;
        t->pos.z += c * v->speed * dt;
    }
}

// ── Animal AI tick ────────────────────────────────────────────────────────────
static void animal_tick(float dt) {
    (void)dt;
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (!g_entities[i].alive) continue;
        if (!(g_entities[i].mask & COMP_ANIMAL)) continue;

        animal_comp_t* a = &g_animals[i];
        a->wander_timer++;

        switch (a->state) {
        case ANIMAL_IDLE:
            if (a->wander_timer > 120) {
                a->state = ANIMAL_WANDER;
                a->wander_timer = 0;
            }
            break;
        case ANIMAL_WANDER:
            if (a->wander_timer > 80) {
                a->state = ANIMAL_IDLE;
                a->wander_timer = 0;
            }
            break;
        case ANIMAL_FOLLOW:
            // Move toward owner
            if (a->owner_id != ECS_INVALID_ID && ecs_alive(a->owner_id)) {
                transform_t* at = &g_transforms[i];
                transform_t* ot = &g_transforms[a->owner_id];
                vec3_t dir = vec3_norm((vec3_t){
                    ot->pos.x - at->pos.x, 0,
                    ot->pos.z - at->pos.z
                });
                at->pos.x += dir.x * 2.0f * (1.0f/60.0f);
                at->pos.z += dir.z * 2.0f * (1.0f/60.0f);
            }
            break;
        case ANIMAL_PERCH:
            // Stay on owner's shoulder
            if (a->owner_id != ECS_INVALID_ID && ecs_alive(a->owner_id)) {
                transform_t* at = &g_transforms[i];
                transform_t* ot = &g_transforms[a->owner_id];
                at->pos.x = ot->pos.x - 0.3f;
                at->pos.y = ot->pos.y + 1.7f;
                at->pos.z = ot->pos.z;
            }
            break;
        default: break;
        }
    }
}

// ── Main engine tick ──────────────────────────────────────────────────────────
void ecs_tick(float dt) {
    if (!g_initialized) return;
    g_tick++;
    physics_tick(dt);
    vehicle_tick(dt);
    animal_tick(dt);
}

uint32_t ecs_entity_count(void) { return g_entity_count; }
uint32_t ecs_tick_count(void)   { return g_tick; }

// ── Scene query ───────────────────────────────────────────────────────────────
uint32_t ecs_get_scene_entities(scene_id_t scene,
                                entity_id_t* out, uint32_t max) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES && n < max; i++) {
        if (g_entities[i].alive && g_entities[i].scene == scene)
            out[n++] = (entity_id_t)i;
    }
    return n;
}

// ── Voice command dispatch ────────────────────────────────────────────────────
void ecs_voice_command(const char* cmd, entity_id_t target) {
    // "COME HERE" — target follows caller
    if (target != ECS_INVALID_ID && ecs_alive(target)) {
        ai_comp_t* ai = ecs_get_ai(target);
        if (ai) {
            if (cmd[0]=='C' && cmd[1]=='O') { // COME
                ai->state = AI_MOVE_TO;
            } else if (cmd[0]=='S' && cmd[1]=='T') { // STOP
                ai->state = AI_IDLE;
            }
        }
        animal_comp_t* an = ecs_get_animal(target);
        if (an) {
            if (cmd[0]=='C') an->state = ANIMAL_FOLLOW;
            if (cmd[0]=='S') an->state = ANIMAL_IDLE;
            if (cmd[0]=='P') an->state = ANIMAL_PERCH;
        }
    }
}
