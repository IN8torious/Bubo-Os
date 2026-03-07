// =============================================================================
// INSTINCT ENGINE — Physics Engine
// Instinct OS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/physics.c
// Rigid body dynamics, AABB collision detection, vehicle physics.
// Demon 170 and Road Runner modeled from real specs.
// =============================================================================

#include "physics.h"
#include "ecs.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Constants ─────────────────────────────────────────────────────────────────
#define GRAVITY        9.81f
#define AIR_DENSITY    1.225f   // kg/m³
#define FIXED_DT       0.01667f // 60Hz

// ── Math helpers ──────────────────────────────────────────────────────────────
static float fabs_f(float x)  { return x < 0.0f ? -x : x; }
static float fmin_f(float a, float b) { return a < b ? a : b; }
static float fmax_f(float a, float b) { return a > b ? a : b; }
static float fclamp(float x, float mn, float mx) {
    return x < mn ? mn : (x > mx ? mx : x);
}
static float fsign(float x) { return x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f); }

// Fast sin approximation (Bhaskara I)
static float fsin(float x) {
    // Normalize to [-pi, pi]
    while (x >  3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x2 = x * x;
    return x * (1.0f - x2/6.0f + x2*x2/120.0f);
}
static float fcos(float x) { return fsin(x + 1.5708f); }

// ── AABB collision ────────────────────────────────────────────────────────────
typedef struct {
    vec3_t min, max;
} aabb_t;

static aabb_t aabb_from_transform(transform_t* t) {
    return (aabb_t){
        .min = {t->pos.x - t->scale.x*0.5f,
                t->pos.y,
                t->pos.z - t->scale.z*0.5f},
        .max = {t->pos.x + t->scale.x*0.5f,
                t->pos.y + t->scale.y,
                t->pos.z + t->scale.z*0.5f}
    };
}

static bool aabb_overlap(aabb_t a, aabb_t b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

static vec3_t aabb_penetration(aabb_t a, aabb_t b) {
    float ox = fmin_f(a.max.x-b.min.x, b.max.x-a.min.x);
    float oy = fmin_f(a.max.y-b.min.y, b.max.y-a.min.y);
    float oz = fmin_f(a.max.z-b.min.z, b.max.z-a.min.z);
    // Return smallest penetration axis
    if (ox <= oy && ox <= oz) return (vec3_t){ox,0,0};
    if (oy <= ox && oy <= oz) return (vec3_t){0,oy,0};
    return (vec3_t){0,0,oz};
}

// ── Vehicle physics ───────────────────────────────────────────────────────────
static void tick_vehicle(entity_id_t id, float dt) {
    vehicle_comp_t* v = ecs_get_vehicle(id);
    transform_t*    t = ecs_get_transform(id);
    physics_comp_t* p = ecs_get_physics(id);
    if (!v || !t || !p) return;

    float mass = v->spec.mass_kg > 0 ? v->spec.mass_kg : 1500.0f;

    // ── Engine torque curve ───────────────────────────────────────────────────
    // Peak torque at 60% of redline
    float rpm_norm = fclamp(v->rpm / v->spec.redline, 0.0f, 1.0f);
    float torque_mult = 4.0f * rpm_norm * (1.0f - rpm_norm); // parabola peak at 0.5
    float torque = v->spec.torque_nm * torque_mult * v->throttle;

    // Nitro
    if (v->nitro && v->nitro_fuel > 0.0f) {
        torque *= 2.8f;
        v->nitro_fuel -= 20.0f * dt;
        if (v->nitro_fuel < 0.0f) { v->nitro_fuel = 0.0f; v->nitro = false; }
    }

    // ── Drive force ───────────────────────────────────────────────────────────
    float wheel_r = v->spec.wheel_radius > 0 ? v->spec.wheel_radius : 0.35f;
    float drive_force = torque / wheel_r;

    // ── Aerodynamic drag ──────────────────────────────────────────────────────
    float spd2 = v->speed * v->speed;
    float drag = 0.5f * AIR_DENSITY * v->spec.drag_coeff *
                 v->spec.frontal_area * spd2;

    // ── Rolling resistance ────────────────────────────────────────────────────
    float roll_resist = 0.015f * mass * GRAVITY;

    // ── Braking force ─────────────────────────────────────────────────────────
    float brake_force = v->brake * v->spec.brake_force;

    // ── Net force → acceleration ──────────────────────────────────────────────
    float net = drive_force - drag - roll_resist - brake_force;
    float accel = net / mass;

    v->speed += accel * dt;
    v->speed  = fclamp(v->speed, 0.0f, v->spec.top_speed_ms);

    // ── RPM update ────────────────────────────────────────────────────────────
    float target_rpm = 800.0f + v->throttle * (v->spec.redline - 800.0f);
    v->rpm += (target_rpm - v->rpm) * 6.0f * dt;
    v->rpm  = fclamp(v->rpm, 800.0f, v->spec.redline);

    // ── Steering ──────────────────────────────────────────────────────────────
    if (fabs_f(v->speed) > 0.5f) {
        float spd_factor = 1.0f - fclamp(v->speed / v->spec.top_speed_ms, 0.0f, 0.8f);
        float steer_rate = v->steer * 2.2f * spd_factor * dt;
        t->rot.y += steer_rate;
    }

    // ── Lateral grip (prevent sliding too much) ───────────────────────────────
    float yaw = t->rot.y;
    float fwd_x = fsin(yaw);
    float fwd_z = fcos(yaw);

    // Velocity in world space
    float vx = p->vel.x, vz = p->vel.z;
    // Project velocity onto forward direction
    float fwd_spd = vx*fwd_x + vz*fwd_z;
    // Lateral component
    float lat_x = vx - fwd_spd*fwd_x;
    float lat_z = vz - fwd_spd*fwd_z;
    // Grip correction (reduce lateral slip)
    float grip = 0.85f;
    p->vel.x = fwd_spd*fwd_x + lat_x*grip;
    p->vel.z = fwd_spd*fwd_z + lat_z*grip;

    // ── Apply forward velocity ────────────────────────────────────────────────
    p->vel.x = fwd_x * v->speed;
    p->vel.z = fwd_z * v->speed;

    // ── Integrate position ────────────────────────────────────────────────────
    t->pos.x += p->vel.x * dt;
    t->pos.z += p->vel.z * dt;

    // ── Ground constraint ─────────────────────────────────────────────────────
    if (t->pos.y < 0.0f) { t->pos.y = 0.0f; p->vel.y = 0.0f; }
}

// ── Rigid body physics ────────────────────────────────────────────────────────
static void tick_rigidbody(entity_id_t id, float dt) {
    physics_comp_t* p = ecs_get_physics(id);
    transform_t*    t = ecs_get_transform(id);
    if (!p || !t || p->kinematic) return;

    // Gravity
    if (!p->grounded) p->vel.y -= GRAVITY * dt;

    // Damping
    p->vel.x *= (1.0f - (1.0f - p->friction) * dt * 60.0f);
    p->vel.z *= (1.0f - (1.0f - p->friction) * dt * 60.0f);

    // Integrate
    t->pos.x += p->vel.x * dt;
    t->pos.y += p->vel.y * dt;
    t->pos.z += p->vel.z * dt;

    // Ground plane
    if (t->pos.y < 0.0f) {
        t->pos.y  = 0.0f;
        p->vel.y  = fabs_f(p->vel.y) * 0.3f; // bounce
        p->grounded = true;
    } else {
        p->grounded = (t->pos.y < 0.05f);
    }
}

// ── Collision resolution ──────────────────────────────────────────────────────
static void resolve_collisions(void) {
    entity_id_t ids[ECS_MAX_ENTITIES];
    uint32_t count = 0;

    // Collect all physics entities
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
        if (!ecs_alive((entity_id_t)i)) continue;
        if (!ecs_get_physics((entity_id_t)i)) continue;
        if (!ecs_get_transform((entity_id_t)i)) continue;
        ids[count++] = (entity_id_t)i;
        if (count >= ECS_MAX_ENTITIES) break;
    }

    // Broad-phase AABB pairs
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t j = i+1; j < count; j++) {
            transform_t* ta = ecs_get_transform(ids[i]);
            transform_t* tb = ecs_get_transform(ids[j]);
            physics_comp_t* pa = ecs_get_physics(ids[i]);
            physics_comp_t* pb = ecs_get_physics(ids[j]);
            if (!ta||!tb||!pa||!pb) continue;
            if (pa->kinematic && pb->kinematic) continue;

            aabb_t aa = aabb_from_transform(ta);
            aabb_t ab = aabb_from_transform(tb);

            if (aabb_overlap(aa, ab)) {
                vec3_t pen = aabb_penetration(aa, ab);
                // Push apart along smallest axis
                if (!pa->kinematic) {
                    ta->pos.x += pen.x * 0.5f * fsign(ta->pos.x - tb->pos.x);
                    ta->pos.y += pen.y * 0.5f * fsign(ta->pos.y - tb->pos.y);
                    ta->pos.z += pen.z * 0.5f * fsign(ta->pos.z - tb->pos.z);
                }
                if (!pb->kinematic) {
                    tb->pos.x -= pen.x * 0.5f * fsign(ta->pos.x - tb->pos.x);
                    tb->pos.y -= pen.y * 0.5f * fsign(ta->pos.y - tb->pos.y);
                    tb->pos.z -= pen.z * 0.5f * fsign(ta->pos.z - tb->pos.z);
                }
                // Velocity response
                if (!pa->kinematic) pa->vel.y *= -0.3f;
                if (!pb->kinematic) pb->vel.y *= -0.3f;
            }
        }
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
void physics_init(void) {
    terminal_write("[PHYS] INSTINCT ENGINE physics initialized. 60Hz fixed step.\n");
}

void physics_tick(float dt) {
    // Fixed timestep accumulator
    static float accum = 0.0f;
    accum += dt;
    while (accum >= FIXED_DT) {
        // Tick all entities
        for (uint32_t i = 0; i < ECS_MAX_ENTITIES; i++) {
            if (!ecs_alive((entity_id_t)i)) continue;
            if (ecs_get_vehicle((entity_id_t)i))
                tick_vehicle((entity_id_t)i, FIXED_DT);
            else
                tick_rigidbody((entity_id_t)i, FIXED_DT);
        }
        resolve_collisions();
        accum -= FIXED_DT;
    }
}

// Apply an impulse to an entity (e.g. explosion, collision)
void physics_impulse(entity_id_t id, vec3_t force) {
    physics_comp_t* p = ecs_get_physics(id);
    if (!p || p->kinematic) return;
    float mass = 1.0f; // default
    p->vel.x += force.x / mass;
    p->vel.y += force.y / mass;
    p->vel.z += force.z / mass;
}

// Teleport entity to position (no physics)
void physics_teleport(entity_id_t id, vec3_t pos) {
    transform_t* t = ecs_get_transform(id);
    if (t) t->pos = pos;
    physics_comp_t* p = ecs_get_physics(id);
    if (p) { p->vel.x=0; p->vel.y=0; p->vel.z=0; }
}

// Get speed of a vehicle entity in km/h
float physics_get_speed_kmh(entity_id_t id) {
    vehicle_comp_t* v = ecs_get_vehicle(id);
    if (!v) return 0.0f;
    return v->speed * 3.6f;
}
