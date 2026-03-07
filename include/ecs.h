// =============================================================================
// RAVEN ENGINE — Entity Component System Header
// Raven AOS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/ecs.h
// =============================================================================

#ifndef ECS_H
#define ECS_H

#include <stdint.h>
#include <stdbool.h>

// ── Limits ────────────────────────────────────────────────────────────────────
#define ECS_MAX_ENTITIES  512
#define ECS_NAME_LEN       32
#define ECS_MAX_STR       "512"
#define ECS_INVALID_ID    ((entity_id_t)0xFFFFFFFF)

typedef uint32_t entity_id_t;

// ── Component mask bits ───────────────────────────────────────────────────────
#define COMP_TRANSFORM   (1u << 0)
#define COMP_RENDER      (1u << 1)
#define COMP_PHYSICS     (1u << 2)
#define COMP_AI          (1u << 3)
#define COMP_CHARACTER   (1u << 4)
#define COMP_VEHICLE     (1u << 5)
#define COMP_ANIMAL      (1u << 6)

// ── Scene IDs (matches world_scene_t values in world.h) ─────────────────────
#define SCENE_NONE        0
#define SCENE_EARTH       1
#define SCENE_NURBURGRING 2
#define SCENE_CORVUS_HOME 3
#define SCENE_CUSTOM      4
typedef uint32_t scene_id_t;

// ── Math types ────────────────────────────────────────────────────────────────
typedef struct { float x, y, z; } vec3_t;
typedef struct { float x, y, z, w; } quat_t;

// ── Transform ────────────────────────────────────────────────────────────────
typedef struct {
    vec3_t pos;
    vec3_t rot;    // Euler angles (radians)
    vec3_t scale;
} transform_t;

// ── Render component ──────────────────────────────────────────────────────────
typedef enum {
    MESH_NONE = 0,
    MESH_CUBE, MESH_SPHERE, MESH_CHARACTER_RAGNAR,
    MESH_CHARACTER_CORVUS, MESH_CHARACTER_LANDON,
    MESH_CHARACTER_RAPHAEL, MESH_ANIMAL_RAVEN,
    MESH_ANIMAL_DOG, MESH_ANIMAL_CAT,
    MESH_CAR_DEMON170, MESH_CAR_ROADRUNNER,
    MESH_BUILDING, MESH_ROAD, MESH_EARTH_SPHERE
} mesh_id_t;

typedef struct {
    mesh_id_t mesh;
    uint32_t  color;
    uint32_t  emit_color;   // Glow/emissive color (0 = none)
    float     emit_strength;
    bool      visible;
    bool      cast_shadow;
} render_comp_t;

// ── Physics component ─────────────────────────────────────────────────────────
typedef struct {
    vec3_t  vel;
    vec3_t  accel;
    float   mass;
    float   friction;
    bool    grounded;
    bool    kinematic;
} physics_comp_t;

// ── Vehicle specs ─────────────────────────────────────────────────────────────
typedef struct {
    float mass_kg;
    float torque_nm;
    float redline;
    float top_speed_ms;
    float drag_coeff;
    float frontal_area;
    float brake_force;
    float wheel_radius;
} vehicle_spec_t;

// Demon 170 specs
#define DEMON170_SPEC { \
    .mass_kg=1942.0f, .torque_nm=1491.0f, .redline=6500.0f, \
    .top_speed_ms=103.0f, .drag_coeff=0.35f, .frontal_area=2.2f, \
    .brake_force=18000.0f, .wheel_radius=0.35f }

// 1970 Road Runner specs
#define ROADRUNNER_SPEC { \
    .mass_kg=1565.0f, .torque_nm=678.0f, .redline=5500.0f, \
    .top_speed_ms=62.0f, .drag_coeff=0.42f, .frontal_area=2.4f, \
    .brake_force=9000.0f, .wheel_radius=0.36f }

typedef struct {
    float         speed;
    float         throttle;
    float         brake;
    float         steer;
    int32_t       gear;
    float         rpm;
    bool          nitro;
    float         nitro_fuel;
    vehicle_spec_t spec;
} vehicle_comp_t;

// ── AI component ──────────────────────────────────────────────────────────────
typedef enum {
    AI_IDLE=0, AI_MOVE_TO, AI_FOLLOW, AI_FLEE, AI_INTERACT, AI_RACE
} ai_state_t;

typedef struct {
    ai_state_t  state;
    entity_id_t target_id;
    vec3_t      target_pos;
    uint32_t    timer;
} ai_comp_t;

// ── Character component ───────────────────────────────────────────────────────
typedef enum {
    CHAR_NATHAN=0, CHAR_CORVUS, CHAR_LANDON, CHAR_RAPHAEL
} char_id_t;

typedef struct {
    char_id_t   who;
    uint32_t    outfit_color;
    uint32_t    accent_color;
    bool        has_raven;       // Huginn perched on shoulder
    float       anim_frame;
    uint32_t    anim_state;      // 0=idle,1=walk,2=run,3=sit,4=drive
} character_comp_t;

// ── Animal component ──────────────────────────────────────────────────────────
typedef enum {
    ANIMAL_IDLE=0, ANIMAL_WANDER, ANIMAL_FOLLOW, ANIMAL_PERCH, ANIMAL_PLAY
} animal_state_t;

typedef enum {
    ANIMAL_RAVEN=0, ANIMAL_DOG, ANIMAL_CAT
} animal_type_t;

typedef struct {
    animal_type_t  type;
    animal_state_t state;
    entity_id_t    owner_id;
    uint32_t       wander_timer;
    vec3_t         wander_target;
} animal_comp_t;

// ── Entity record ─────────────────────────────────────────────────────────────
typedef struct {
    entity_id_t id;
    bool        alive;
    uint32_t    mask;
    char        name[ECS_NAME_LEN];
    scene_id_t  scene;
} raven_entity_t;

// ── Public API ────────────────────────────────────────────────────────────────
void             ecs_init(void);
void             ecs_tick(float dt);
entity_id_t      ecs_create(const char* name, scene_id_t scene);
void             ecs_destroy(entity_id_t id);
bool             ecs_alive(entity_id_t id);

transform_t*     ecs_add_transform(entity_id_t id);
render_comp_t*   ecs_add_render(entity_id_t id);
physics_comp_t*  ecs_add_physics(entity_id_t id);
ai_comp_t*       ecs_add_ai(entity_id_t id);
character_comp_t*ecs_add_character(entity_id_t id);
vehicle_comp_t*  ecs_add_vehicle(entity_id_t id);
animal_comp_t*   ecs_add_animal(entity_id_t id);

transform_t*     ecs_get_transform(entity_id_t id);
render_comp_t*   ecs_get_render(entity_id_t id);
physics_comp_t*  ecs_get_physics(entity_id_t id);
ai_comp_t*       ecs_get_ai(entity_id_t id);
character_comp_t*ecs_get_character(entity_id_t id);
vehicle_comp_t*  ecs_get_vehicle(entity_id_t id);
animal_comp_t*   ecs_get_animal(entity_id_t id);
raven_entity_t*  ecs_get_entity(entity_id_t id);

uint32_t         ecs_entity_count(void);
uint32_t         ecs_tick_count(void);
uint32_t         ecs_get_scene_entities(scene_id_t scene,
                                        entity_id_t* out, uint32_t max);
void             ecs_voice_command(const char* cmd, entity_id_t target);

#endif // ECS_H
