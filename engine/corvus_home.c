// =============================================================================
// INSTINCT ENGINE — CORVUS Home Scene
// Deep Flow OS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/corvus_home.c
// CORVUS's house. Nathan's world. Landon's safe space. Raphael's garage.
// Animals roam. The Rinnegan glows. The ravens watch.
// =============================================================================

#include "corvus_home.h"
#include "ecs.h"
#include "renderer3d.h"
#include "framebuffer.h"
#include "font.h"
#include "polish.h"
#include "corvus.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Scene entity IDs ──────────────────────────────────────────────────────────
static entity_id_t e_nathan     = ECS_INVALID_ID;
static entity_id_t e_corvus_char= ECS_INVALID_ID;
static entity_id_t e_landon     = ECS_INVALID_ID;
static entity_id_t e_raphael    = ECS_INVALID_ID;
static entity_id_t e_huginn     = ECS_INVALID_ID;  // Nathan's raven
static entity_id_t e_muninn     = ECS_INVALID_ID;  // CORVUS's raven
static entity_id_t e_dog        = ECS_INVALID_ID;
static entity_id_t e_cat        = ECS_INVALID_ID;
static entity_id_t e_demon170   = ECS_INVALID_ID;  // In the garage
static entity_id_t e_roadrunner = ECS_INVALID_ID;  // Raphael's car

static bool g_initialized = false;
static bool g_open        = false;
static bool g_minimized   = false;

// Camera
static vec3_t g_cam_pos  = {0.0f, 2.0f, -8.0f};
static vec3_t g_cam_look = {0.0f, 1.0f,  0.0f};
static float  g_cam_yaw  = 0.0f;

// ── Scene setup ───────────────────────────────────────────────────────────────
static entity_id_t make_character(const char* name, char_id_t who,
                                   float x, float z,
                                   uint32_t outfit, uint32_t accent,
                                   bool has_raven) {
    entity_id_t id = ecs_create(name, SCENE_CORVUS_HOME);
    transform_t* t = ecs_add_transform(id);
    t->pos = (vec3_t){x, 0.0f, z};
    t->scale = (vec3_t){1.0f, 1.0f, 1.0f};

    render_comp_t* r = ecs_add_render(id);
    r->mesh = (who == CHAR_CORVUS) ? MESH_CHARACTER_CORVUS :
              (who == CHAR_LANDON) ? MESH_CHARACTER_LANDON :
              (who == CHAR_RAPHAEL)? MESH_CHARACTER_RAPHAEL:
                                     MESH_CHARACTER_RAGNAR;
    r->color       = outfit;
    r->emit_color  = (who == CHAR_CORVUS) ? 0x8800FF : 0;
    r->emit_strength = (who == CHAR_CORVUS) ? 1.0f : 0.0f;
    r->visible     = true;
    r->cast_shadow = true;

    ecs_add_physics(id)->kinematic = true;  // Characters are kinematic

    ai_comp_t* ai = ecs_add_ai(id);
    ai->state = AI_IDLE;

    character_comp_t* c = ecs_add_character(id);
    c->who          = who;
    c->outfit_color = outfit;
    c->accent_color = accent;
    c->has_raven    = has_raven;
    c->anim_state   = (who == CHAR_LANDON) ? 3 : 0;  // Landon sits

    return id;
}

static entity_id_t make_animal(const char* name, animal_type_t type,
                                float x, float y, float z,
                                entity_id_t owner) {
    entity_id_t id = ecs_create(name, SCENE_CORVUS_HOME);
    transform_t* t = ecs_add_transform(id);
    t->pos   = (vec3_t){x, y, z};
    t->scale = (vec3_t){0.3f, 0.3f, 0.3f};

    render_comp_t* r = ecs_add_render(id);
    r->mesh    = (type == ANIMAL_RAVEN) ? MESH_ANIMAL_RAVEN :
                 (type == ANIMAL_DOG)   ? MESH_ANIMAL_DOG   :
                                          MESH_ANIMAL_CAT;
    r->color   = (type == ANIMAL_RAVEN) ? 0x111111 :
                 (type == ANIMAL_DOG)   ? 0xAA8844 : 0xCCAA88;
    r->visible = true;

    ecs_add_physics(id)->mass = 0.5f;

    animal_comp_t* a = ecs_add_animal(id);
    a->type     = type;
    a->owner_id = owner;
    a->state    = (type == ANIMAL_RAVEN) ? ANIMAL_PERCH : ANIMAL_IDLE;

    return id;
}

static entity_id_t make_car(const char* name, mesh_id_t mesh,
                              float x, float z, uint32_t color) {
    entity_id_t id = ecs_create(name, SCENE_CORVUS_HOME);
    transform_t* t = ecs_add_transform(id);
    t->pos   = (vec3_t){x, 0.0f, z};
    t->scale = (vec3_t){1.0f, 1.0f, 1.0f};

    render_comp_t* r = ecs_add_render(id);
    r->mesh        = mesh;
    r->color       = color;
    r->emit_color  = 0;
    r->visible     = true;
    r->cast_shadow = true;

    vehicle_comp_t* v = ecs_add_vehicle(id);
    if (mesh == MESH_CAR_DEMON170) {
        vehicle_spec_t s = DEMON170_SPEC;
        v->spec = s;
    } else {
        vehicle_spec_t s = ROADRUNNER_SPEC;
        v->spec = s;
    }

    return id;
}

void corvus_home_init(void) {
    if (g_initialized) return;

    terminal_write("[HOME] Initializing CORVUS Home scene...\n");

    // ── Characters ────────────────────────────────────────────────────────────
    // Nathan — dark outfit, crimson accent, Huginn on shoulder
    e_nathan = make_character("Nathan", CHAR_NATHAN,
                               -1.5f, 0.0f, 0x1A1A2E, 0xCC0000, true);

    // CORVUS — black cloak, purple Rinnegan glow
    e_corvus_char = make_character("BUBO", CHAR_CORVUS,
                                    0.0f, 0.0f, 0x0D0D0D, 0x8800FF, false);

    // Landon — racing suit, seated in his chair
    e_landon = make_character("Landon", CHAR_LANDON,
                               2.0f, 1.0f, 0x1A1A8B, 0xFF4400, false);

    // Raphael — mechanic jacket, near the garage
    e_raphael = make_character("Raphael", CHAR_RAPHAEL,
                                4.0f, -2.0f, 0x2A4A2A, 0xFFAA00, false);

    // ── Ravens ────────────────────────────────────────────────────────────────
    // Huginn — Nathan's raven, perched on his left shoulder
    e_huginn = make_animal("Huginn", ANIMAL_RAVEN,
                            -1.8f, 1.7f, 0.0f, e_nathan);

    // Muninn — CORVUS's raven, perched on CORVUS's shoulder
    e_muninn = make_animal("Muninn", ANIMAL_RAVEN,
                            -0.3f, 1.7f, 0.0f, e_corvus_char);

    // ── Other animals ─────────────────────────────────────────────────────────
    e_dog = make_animal("Dog", ANIMAL_DOG, 1.0f, 0.0f, 2.0f, e_nathan);
    e_cat = make_animal("Cat", ANIMAL_CAT, 3.0f, 0.8f, -1.0f, ECS_INVALID_ID);

    // Cat sits on a shelf — elevated
    transform_t* ct = ecs_get_transform(e_cat);
    if (ct) ct->pos.y = 1.2f;

    // ── Cars in the garage ────────────────────────────────────────────────────
    e_demon170   = make_car("Demon 170",    MESH_CAR_DEMON170,
                             6.0f, 0.0f, 0x1A0000);  // Deep crimson
    e_roadrunner = make_car("Road Runner",  MESH_CAR_ROADRUNNER,
                             8.5f, 0.0f, 0xDDAA00);  // Classic yellow

    g_initialized = true;
    g_open        = true;
    terminal_write("[HOME] CORVUS Home ready. Nathan, Landon, Raphael present.\n");
    terminal_write("[HOME] Huginn and Muninn are watching.\n");
}

void corvus_home_open(void)     { g_open = true;  g_minimized = false; }
void corvus_home_close(void)    { g_open = false; }
void corvus_home_minimize(void) { g_minimized = true; }
bool corvus_home_is_open(void)  { return g_open && !g_minimized; }

// ── Camera control ────────────────────────────────────────────────────────────
void corvus_home_look_at(entity_id_t target) {
    if (!ecs_alive(target)) return;
    transform_t* t = ecs_get_transform(target);
    if (!t) return;
    g_cam_look = t->pos;
    g_cam_pos  = (vec3_t){
        t->pos.x - 3.0f,
        t->pos.y + 2.0f,
        t->pos.z - 5.0f
    };
}

void corvus_home_focus_nathan(void)   { corvus_home_look_at(e_nathan); }
void corvus_home_focus_corvus(void)   { corvus_home_look_at(e_corvus_char); }
void corvus_home_focus_landon(void)   { corvus_home_look_at(e_landon); }
void corvus_home_focus_raphael(void)  { corvus_home_look_at(e_raphael); }
void corvus_home_focus_garage(void)   { corvus_home_look_at(e_demon170); }

// ── Voice commands ────────────────────────────────────────────────────────────
void corvus_home_voice(const char* cmd) {
    if (!cmd || !cmd[0]) return;

    // "CORVUS HOME" — open the scene
    if (cmd[0]=='H' && cmd[1]=='O') {
        corvus_home_open();
        return;
    }
    // "SHOW LANDON"
    if (cmd[0]=='S' && cmd[5]=='L') {
        corvus_home_focus_landon(); return;
    }
    // "SHOW GARAGE" / "SHOW CARS"
    if (cmd[0]=='S' && (cmd[5]=='G' || cmd[5]=='C')) {
        corvus_home_focus_garage(); return;
    }
    // "CALL RAVEN" / "CALL HUGINN"
    if (cmd[0]=='C' && cmd[1]=='A') {
        animal_comp_t* a = ecs_get_animal(e_huginn);
        if (a) a->state = ANIMAL_FOLLOW;
        return;
    }
    // "CORVUS COME HERE"
    if (cmd[0]=='C' && cmd[1]=='O') {
        ai_comp_t* ai = ecs_get_ai(e_corvus_char);
        if (ai) {
            ai->state     = AI_MOVE_TO;
            ai->target_id = e_nathan;
        }
        return;
    }
    // "CALL DOG"
    if (cmd[0]=='D') {
        animal_comp_t* a = ecs_get_animal(e_dog);
        if (a) { a->state = ANIMAL_FOLLOW; a->owner_id = e_nathan; }
        return;
    }
}

// ── Tick ──────────────────────────────────────────────────────────────────────
void corvus_home_tick(float dt) {
    if (!g_open || g_minimized) return;

    // Huginn stays on Nathan's shoulder
    if (ecs_alive(e_huginn) && ecs_alive(e_nathan)) {
        animal_comp_t* a = ecs_get_animal(e_huginn);
        if (a && a->state == ANIMAL_PERCH) {
            transform_t* ht = ecs_get_transform(e_huginn);
            transform_t* nt = ecs_get_transform(e_nathan);
            if (ht && nt) {
                ht->pos.x = nt->pos.x - 0.3f;
                ht->pos.y = nt->pos.y + 1.7f;
                ht->pos.z = nt->pos.z;
            }
        }
    }

    // Muninn stays on CORVUS's shoulder
    if (ecs_alive(e_muninn) && ecs_alive(e_corvus_char)) {
        animal_comp_t* a = ecs_get_animal(e_muninn);
        if (a && a->state == ANIMAL_PERCH) {
            transform_t* mt = ecs_get_transform(e_muninn);
            transform_t* ct = ecs_get_transform(e_corvus_char);
            if (mt && ct) {
                mt->pos.x = ct->pos.x + 0.3f;
                mt->pos.y = ct->pos.y + 1.7f;
                mt->pos.z = ct->pos.z;
            }
        }
    }

    // CORVUS character glow pulses with agent activity
    render_comp_t* cr = ecs_get_render(e_corvus_char);
    if (cr) {
        corvus_agent_t* agents = corvus_get_agents();
        float avg_load = 0.0f;
        for (uint32_t i = 0; i < CORVUS_AGENT_COUNT; i++)
            avg_load += agents[i].health;
        avg_load /= CORVUS_AGENT_COUNT;
        cr->emit_strength = 0.3f + avg_load * 0.7f;
    }

    (void)dt;
}

// ── Draw (2D overlay on framebuffer — 3D via renderer3d) ─────────────────────
void corvus_home_draw(void) {
    if (!g_open || g_minimized) return;

    // The 3D scene is rendered by renderer3d_draw_scene(SCENE_CORVUS_HOME)
    // Here we draw the 2D HUD overlay on top

    uint32_t sw = fb_get_info()->width;
    uint32_t sh = fb_get_info()->height;

    // Scene label
    polish_glass_rect(4, 4, 220, 20, 0x0D0820, 200);
    font_draw_string(8, 7, "CORVUS HOME", 0xFF4444, 0, true);

    // Character status strip at bottom
    int32_t sy = (int32_t)sh - 28;
    polish_glass_rect(0, sy, (int32_t)sw, 28, 0x0D0820, 210);

    const char* chars[] = { "Nathan", "BUBO", "Landon", "Raphael" };
    entity_id_t ids[]   = { e_nathan, e_corvus_char, e_landon, e_raphael };
    for (uint32_t i = 0; i < 4; i++) {
        int32_t cx = 8 + (int32_t)(i * 180);
        uint32_t col = ecs_alive(ids[i]) ? 0x00CC44 : 0x444444;
        fb_fill_rect((uint32_t)(cx), (uint32_t)(sy+8), 10, 10, col);
        font_draw_string(cx+14, sy+8, chars[i], 0xCCCCCC, 0, true);
    }

    // Animal status
    font_draw_string(740, sy+8, "Huginn", 0x8888FF, 0, true);
    font_draw_string(810, sy+8, "Muninn", 0x8888FF, 0, true);
    font_draw_string(880, sy+8, "Dog", 0xAA8844, 0, true);
    font_draw_string(920, sy+8, "Cat", 0xCCAA88, 0, true);
}

// ── Accessors ─────────────────────────────────────────────────────────────────
entity_id_t corvus_home_get_nathan(void)   { return e_nathan; }
entity_id_t corvus_home_get_corvus(void)   { return e_corvus_char; }
entity_id_t corvus_home_get_landon(void)   { return e_landon; }
entity_id_t corvus_home_get_raphael(void)  { return e_raphael; }
entity_id_t corvus_home_get_huginn(void)   { return e_huginn; }
entity_id_t corvus_home_get_demon170(void) { return e_demon170; }
