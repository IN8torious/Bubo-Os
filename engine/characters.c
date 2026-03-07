// =============================================================================
// RAVEN ENGINE — Character System
// Raven AOS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/characters.c
// Skeletal animation, clothing layers, animal AI behavior trees.
// Characters: Nathan (Ragnar), CORVUS, Landon, Raphael
// Animals: Huginn (Nathan's raven), Muninn (CORVUS's raven), dog, cat
// =============================================================================

#include "characters.h"
#include "ecs.h"
#include "physics.h"
#include "framebuffer.h"
#include "font.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Compatibility aliases ─────────────────────────────────────────────────────
#define ENTITY_INVALID  ECS_INVALID_ID
static inline entity_id_t ecs_create_entity(void) {
    return ecs_create("entity", SCENE_NONE);
}

// ── Math helpers ──────────────────────────────────────────────────────────────
static float fsin(float x) {
    while (x >  3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x2=x*x;
    return x*(1.0f - x2/6.0f + x2*x2/120.0f);
}
static float fcos(float x) { return fsin(x+1.5708f); }
static float fabs_f(float x) { return x<0?-x:x; }
static float flerp(float a, float b, float t) { return a + (b-a)*t; }
static float fclamp(float x, float mn, float mx) {
    return x<mn?mn:(x>mx?mx:x);
}

// ── String helpers ────────────────────────────────────────────────────────────
static void str_copy(char* dst, const char* src, uint32_t max) {
    uint32_t i=0;
    while (src[i]&&i<max-1){dst[i]=src[i];i++;}
    dst[i]=0;
}
static bool str_eq(const char* a, const char* b) {
    while(*a&&*b){if(*a!=*b)return false;a++;b++;}
    return *a==*b;
}
static bool str_starts_with(const char* s, const char* prefix) {
    while(*prefix){if(*s!=*prefix)return false;s++;prefix++;}
    return true;
}
static bool str_contains(const char* s, const char* sub) {
    while(*s){
        const char* a=s; const char* b=sub;
        while(*a&&*b&&*a==*b){a++;b++;}
        if(!*b) return true;
        s++;
    }
    return false;
}

// ── Bone indices ──────────────────────────────────────────────────────────────
#define BONE_ROOT       0
#define BONE_SPINE      1
#define BONE_CHEST      2
#define BONE_NECK       3
#define BONE_HEAD       4
#define BONE_L_SHOULDER 5
#define BONE_L_ELBOW    6
#define BONE_L_HAND     7
#define BONE_R_SHOULDER 8
#define BONE_R_ELBOW    9
#define BONE_R_HAND    10
#define BONE_L_HIP     11
#define BONE_L_KNEE    12
#define BONE_L_FOOT    13
#define BONE_R_HIP     14
#define BONE_R_KNEE    15
#define BONE_R_FOOT    16
#define BONE_COUNT     17

// ── Skeleton ──────────────────────────────────────────────────────────────────
typedef struct {
    float rot_x, rot_y, rot_z;  // Euler angles for this bone
    float len;                   // Bone length
} bone_t;

typedef struct {
    bone_t bones[BONE_COUNT];
    float  anim_time;
    char   anim_name[32];
} skeleton_t;

// ── Clothing layer ────────────────────────────────────────────────────────────
typedef struct {
    uint32_t torso_color;
    uint32_t pants_color;
    uint32_t shoes_color;
    uint32_t hair_color;
    uint32_t skin_color;
    char     accessory[32];  // "raven", "headset", "crown", "none"
} outfit_t;

// ── Character definitions ─────────────────────────────────────────────────────
#define CHAR_COUNT 4
#define ANIMAL_COUNT 4

typedef struct character_t {
    char       name[32];
    entity_id_t entity;
    skeleton_t  skeleton;
    outfit_t    outfit;
    anim_state_t state;
    float       walk_speed;
    vec3_t      target_pos;
    bool        moving;
    char        voice_callsign[32];
} character_t;

typedef struct animal_t {
    char        name[32];
    entity_id_t entity;
    animal_type_t type;
    float       anim_time;
    vec3_t      target_pos;
    bool        following;
    entity_id_t follow_target;
    float       idle_timer;
    float       wander_timer;
} animal_t;

static character_t g_chars[CHAR_COUNT];
static animal_t    g_animals[ANIMAL_COUNT];
static uint32_t    g_char_count   = 0;
static uint32_t    g_animal_count = 0;

// ── Default skeleton (T-pose) ─────────────────────────────────────────────────
static void skeleton_init_tpose(skeleton_t* sk) {
    for (uint32_t i = 0; i < BONE_COUNT; i++) {
        sk->bones[i].rot_x = 0;
        sk->bones[i].rot_y = 0;
        sk->bones[i].rot_z = 0;
    }
    // Bone lengths (meters)
    sk->bones[BONE_ROOT].len      = 0.0f;
    sk->bones[BONE_SPINE].len     = 0.30f;
    sk->bones[BONE_CHEST].len     = 0.20f;
    sk->bones[BONE_NECK].len      = 0.10f;
    sk->bones[BONE_HEAD].len      = 0.22f;
    sk->bones[BONE_L_SHOULDER].len= 0.15f;
    sk->bones[BONE_L_ELBOW].len   = 0.28f;
    sk->bones[BONE_L_HAND].len    = 0.10f;
    sk->bones[BONE_R_SHOULDER].len= 0.15f;
    sk->bones[BONE_R_ELBOW].len   = 0.28f;
    sk->bones[BONE_R_HAND].len    = 0.10f;
    sk->bones[BONE_L_HIP].len     = 0.12f;
    sk->bones[BONE_L_KNEE].len    = 0.42f;
    sk->bones[BONE_L_FOOT].len    = 0.40f;
    sk->bones[BONE_R_HIP].len     = 0.12f;
    sk->bones[BONE_R_KNEE].len    = 0.42f;
    sk->bones[BONE_R_FOOT].len    = 0.40f;
    sk->anim_time = 0.0f;
}

// ── Animation: walk cycle ─────────────────────────────────────────────────────
static void anim_walk(skeleton_t* sk, float t) {
    float s = fsin(t * 6.28f);
    float c = fcos(t * 6.28f);
    // Legs swing
    sk->bones[BONE_L_HIP].rot_x  =  s * 0.4f;
    sk->bones[BONE_R_HIP].rot_x  = -s * 0.4f;
    sk->bones[BONE_L_KNEE].rot_x = fclamp(-s * 0.3f, -0.5f, 0.0f);
    sk->bones[BONE_R_KNEE].rot_x = fclamp( s * 0.3f, -0.5f, 0.0f);
    // Arms swing opposite
    sk->bones[BONE_L_SHOULDER].rot_x = -s * 0.3f;
    sk->bones[BONE_R_SHOULDER].rot_x =  s * 0.3f;
    // Spine slight sway
    sk->bones[BONE_SPINE].rot_z = s * 0.05f;
    // Head bob
    sk->bones[BONE_HEAD].rot_x = fabs_f(s) * 0.02f;
}

// ── Animation: idle ───────────────────────────────────────────────────────────
static void anim_idle(skeleton_t* sk, float t) {
    float s = fsin(t * 1.2f);
    // Gentle breathing
    sk->bones[BONE_CHEST].rot_x = s * 0.02f;
    sk->bones[BONE_SPINE].rot_x = s * 0.01f;
    // Slight head movement
    sk->bones[BONE_HEAD].rot_y  = fsin(t * 0.5f) * 0.05f;
}

// ── Animation: sit (Landon in racing chair) ───────────────────────────────────
static void anim_sit(skeleton_t* sk, float t) {
    sk->bones[BONE_L_HIP].rot_x  = -1.5f;
    sk->bones[BONE_R_HIP].rot_x  = -1.5f;
    sk->bones[BONE_L_KNEE].rot_x =  1.5f;
    sk->bones[BONE_R_KNEE].rot_x =  1.5f;
    sk->bones[BONE_SPINE].rot_x  = -0.1f;
    // Breathing
    float s = fsin(t * 1.0f);
    sk->bones[BONE_CHEST].rot_x  = s * 0.02f;
}

// ── Animation: fly (for ravens) ──────────────────────────────────────────────
static void anim_fly(skeleton_t* sk, float t) {
    float s = fsin(t * 8.0f);
    sk->bones[BONE_L_SHOULDER].rot_z =  s * 0.8f;
    sk->bones[BONE_R_SHOULDER].rot_z = -s * 0.8f;
    sk->bones[BONE_L_ELBOW].rot_z    =  fabs_f(s) * 0.4f;
    sk->bones[BONE_R_ELBOW].rot_z    =  fabs_f(s) * 0.4f;
    // Body dips with wing beats
    sk->bones[BONE_ROOT].rot_x = fabs_f(s) * 0.1f;
}

// ── Character creation ────────────────────────────────────────────────────────
static character_t* char_create(const char* name, const char* callsign,
                                  outfit_t outfit, float walk_speed) {
    if (g_char_count >= CHAR_COUNT) return NULL;
    character_t* c = &g_chars[g_char_count++];
    str_copy(c->name, name, 32);
    str_copy(c->voice_callsign, callsign, 32);
    c->outfit     = outfit;
    c->walk_speed = walk_speed;
    c->state      = ANIM_IDLE;
    c->moving     = false;
    skeleton_init_tpose(&c->skeleton);
    c->entity = ecs_create_entity();
    return c;
}

// ── Animal creation ───────────────────────────────────────────────────────────
static animal_t* animal_create(const char* name, animal_type_t type,
                                 entity_id_t follow_target) {
    if (g_animal_count >= ANIMAL_COUNT) return NULL;
    animal_t* a = &g_animals[g_animal_count++];
    str_copy(a->name, name, 32);
    a->type          = type;
    a->following     = (follow_target != ENTITY_INVALID);
    a->follow_target = follow_target;
    a->idle_timer    = 0.0f;
    a->wander_timer  = 3.0f;
    a->entity        = ecs_create_entity();
    return a;
}

// ── Characters init ───────────────────────────────────────────────────────────
void characters_init(void) {
    g_char_count   = 0;
    g_animal_count = 0;

    // Nathan — Ragnar
    character_t* nathan = char_create(
        "Nathan", "Ragnar",
        (outfit_t){
            .torso_color = 0x1A1A1A,  // Black tactical jacket
            .pants_color = 0x2A2A3A,  // Dark grey
            .shoes_color = 0x111111,  // Black boots
            .hair_color  = 0x3A2A1A,  // Dark brown
            .skin_color  = 0xC8A882,  // Natural
            .accessory   = "raven"    // Huginn on shoulder
        },
        1.4f
    );

    // CORVUS — the sovereign AI
    character_t* corvus = char_create(
        "CORVUS", "CORVUS",
        (outfit_t){
            .torso_color = 0x0A0A0A,  // Void black cloak
            .pants_color = 0x0A0A0A,
            .shoes_color = 0x0A0A0A,
            .hair_color  = 0x0A0A0A,
            .skin_color  = 0x8844CC,  // Purple — supernatural
            .accessory   = "raven"    // Muninn on shoulder
        },
        1.6f
    );

    // Landon — seated in racing chair
    character_t* landon = char_create(
        "Landon", "Landon",
        (outfit_t){
            .torso_color = 0xCC2222,  // Racing suit — Demon 170 red
            .pants_color = 0xCC2222,
            .shoes_color = 0x111111,
            .hair_color  = 0x4A3A2A,
            .skin_color  = 0xC8A882,
            .accessory   = "headset"
        },
        0.0f  // Landon doesn't walk — he races
    );
    if (landon) landon->state = ANIM_SIT;

    // Raphael — mechanic jacket, Road Runner colors
    char_create(
        "Raphael", "Raphael",
        (outfit_t){
            .torso_color = 0x1A3A5A,  // Mechanic jacket — Road Runner blue
            .pants_color = 0x2A2A2A,
            .shoes_color = 0x3A2A1A,
            .hair_color  = 0x1A0A0A,  // Dark
            .skin_color  = 0xC8A882,
            .accessory   = "none"
        },
        1.3f
    );

    // Animals
    // Huginn — Nathan's raven (thought)
    animal_create("Huginn", ANIMAL_RAVEN,
                   nathan ? nathan->entity : ENTITY_INVALID);

    // Muninn — CORVUS's raven (memory)
    animal_create("Muninn", ANIMAL_RAVEN,
                   corvus ? corvus->entity : ENTITY_INVALID);

    // Dog — follows nearest character
    animal_create("Dog", ANIMAL_DOG,
                   nathan ? nathan->entity : ENTITY_INVALID);

    // Cat — does whatever it wants
    animal_create("Cat", ANIMAL_CAT, ENTITY_INVALID);

    terminal_write("[CHAR] Characters initialized: Nathan, CORVUS, Landon, Raphael\n");
    terminal_write("[CHAR] Animals initialized: Huginn, Muninn, Dog, Cat\n");
}

// ── Character tick ────────────────────────────────────────────────────────────
static void char_tick(character_t* c, float dt) {
    c->skeleton.anim_time += dt;
    float t = c->skeleton.anim_time;

    // Movement
    if (c->moving) {
        transform_t* tr = ecs_get_transform(c->entity);
        if (tr) {
            float dx = c->target_pos.x - tr->pos.x;
            float dz = c->target_pos.z - tr->pos.z;
            float dist = dx*dx + dz*dz;
            if (dist < 0.01f) {
                c->moving = false;
                c->state  = ANIM_IDLE;
            } else {
                float inv_d = 1.0f / (dist < 0.0001f ? 0.0001f : dist);
                tr->pos.x += dx * inv_d * c->walk_speed * dt;
                tr->pos.z += dz * inv_d * c->walk_speed * dt;
                c->state = ANIM_WALK;
            }
        }
    }

    // Apply animation
    switch (c->state) {
        case ANIM_WALK:  anim_walk(&c->skeleton, t); break;
        case ANIM_SIT:   anim_sit (&c->skeleton, t); break;
        case ANIM_IDLE:
        default:         anim_idle(&c->skeleton, t); break;
    }
}

// ── Animal tick ───────────────────────────────────────────────────────────────
static void animal_tick(animal_t* a, float dt) {
    a->anim_time += dt;
    a->idle_timer   -= dt;
    a->wander_timer -= dt;

    transform_t* tr = ecs_get_transform(a->entity);
    if (!tr) return;

    switch (a->type) {
        case ANIMAL_RAVEN: {
            // Ravens follow their owner, perch on shoulder when close
            if (a->following) {
                transform_t* owner = ecs_get_transform(a->follow_target);
                if (owner) {
                    float dx = owner->pos.x - tr->pos.x;
                    float dz = owner->pos.z - tr->pos.z;
                    float dist = dx*dx + dz*dz;
                    if (dist > 0.5f) {
                        // Fly toward owner
                        float inv_d = 1.0f / (dist < 0.0001f ? 0.0001f : dist);
                        tr->pos.x += dx * inv_d * 2.5f * dt;
                        tr->pos.y  = flerp(tr->pos.y, owner->pos.y + 1.8f, dt*3.0f);
                        tr->pos.z += dz * inv_d * 2.5f * dt;
                    } else {
                        // Perch on shoulder
                        tr->pos.x = owner->pos.x + 0.3f;
                        tr->pos.y = owner->pos.y + 1.6f;
                        tr->pos.z = owner->pos.z;
                    }
                }
            }
            // Fly animation
            /* raven fly animation applied via transform */
            break;
        }

        case ANIMAL_DOG: {
            // Dog follows owner, sits when idle
            if (a->following) {
                transform_t* owner = ecs_get_transform(a->follow_target);
                if (owner) {
                    float dx = owner->pos.x - tr->pos.x;
                    float dz = owner->pos.z - tr->pos.z;
                    float dist = dx*dx + dz*dz;
                    if (dist > 1.5f) {
                        float inv_d = 1.0f / (dist < 0.0001f ? 0.0001f : dist);
                        tr->pos.x += dx * inv_d * 1.8f * dt;
                        tr->pos.z += dz * inv_d * 1.8f * dt;
                    }
                }
            }
            break;
        }

        case ANIMAL_CAT: {
            // Cat wanders randomly, ignores everyone
            if (a->wander_timer <= 0.0f) {
                // Pick a new random target nearby
                a->target_pos.x = tr->pos.x + (float)((a->entity * 1664525 + 1013904223) & 0xFF) / 255.0f * 4.0f - 2.0f;
                a->target_pos.z = tr->pos.z + (float)(((a->entity * 22695477) >> 8) & 0xFF) / 255.0f * 4.0f - 2.0f;
                a->wander_timer = 3.0f + (float)((a->entity * 6364136) & 0xFF) / 255.0f * 5.0f;
            }
            float dx = a->target_pos.x - tr->pos.x;
            float dz = a->target_pos.z - tr->pos.z;
            float dist = dx*dx + dz*dz;
            if (dist > 0.1f) {
                float inv_d = 1.0f / (dist < 0.0001f ? 0.0001f : dist);
                tr->pos.x += dx * inv_d * 0.8f * dt;
                tr->pos.z += dz * inv_d * 0.8f * dt;
            }
            break;
        }
    }
}

// ── Public tick ───────────────────────────────────────────────────────────────
void characters_tick(float dt) {
    for (uint32_t i = 0; i < g_char_count; i++)
        char_tick(&g_chars[i], dt);
    for (uint32_t i = 0; i < g_animal_count; i++)
        animal_tick(&g_animals[i], dt);
}

// ── Voice command handlers ────────────────────────────────────────────────────
void characters_on_voice_cmd(const char* cmd) {
    // "CORVUS COME HERE" — CORVUS walks to Nathan
    if (str_starts_with(cmd, "CORVUS COME HERE")) {
        character_t* corvus = characters_find("CORVUS");
        character_t* nathan = characters_find("Nathan");
        if (corvus && nathan) {
            transform_t* nt = ecs_get_transform(nathan->entity);
            if (nt) {
                corvus->target_pos = (vec3_t){nt->pos.x+0.8f, 0, nt->pos.z};
                corvus->moving = true;
            }
        }
    }
    // "CORVUS SHOW LANDON" — camera moves to Landon
    else if (str_starts_with(cmd, "CORVUS SHOW LANDON")) {
        terminal_write("[CHAR] Camera moving to Landon...\n");
        // Camera transition handled by renderer
    }
    // "CORVUS CALL RAVEN" — Huginn flies to Nathan
    else if (str_starts_with(cmd, "CORVUS CALL RAVEN")) {
        for (uint32_t i = 0; i < g_animal_count; i++) {
            if (str_eq(g_animals[i].name, "Huginn")) {
                character_t* nathan = characters_find("Nathan");
                if (nathan) {
                    g_animals[i].following     = true;
                    g_animals[i].follow_target = nathan->entity;
                }
            }
        }
    }
    // "CORVUS CHANGE [character] [item] TO [color]"
    else if (str_starts_with(cmd, "CORVUS CHANGE")) {
        // Parse: CORVUS CHANGE NATHAN JACKET TO RED
        // Simplified: just update Nathan's torso color
        if (str_contains(cmd, "RED"))
            g_chars[0].outfit.torso_color = 0xCC2222;
        else if (str_contains(cmd, "BLUE"))
            g_chars[0].outfit.torso_color = 0x2244CC;
        else if (str_contains(cmd, "BLACK"))
            g_chars[0].outfit.torso_color = 0x111111;
        else if (str_contains(cmd, "WHITE"))
            g_chars[0].outfit.torso_color = 0xEEEEEE;
    }
}

// ── Lookup helpers ────────────────────────────────────────────────────────────
character_t* characters_find(const char* name) {
    for (uint32_t i = 0; i < g_char_count; i++)
        if (str_eq(g_chars[i].name, name)) return &g_chars[i];
    return NULL;
}

uint32_t characters_get_count(void) { return g_char_count; }
uint32_t animals_get_count(void)    { return g_animal_count; }
character_t* characters_get(uint32_t idx) {
    return idx < g_char_count ? &g_chars[idx] : NULL;
}
animal_t* animals_get(uint32_t idx) {
    return idx < g_animal_count ? &g_animals[idx] : NULL;
}


