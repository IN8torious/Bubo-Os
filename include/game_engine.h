// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/game_engine.h — 2D/2.5D Game Engine Extension
// Tilemap, sprite animation, AABB collision, camera, dialogue, inventory.
// Sits on top of INSTINCT ENGINE (ECS + physics + renderer3d).
// =============================================================================
#ifndef DEEPFLOW_GAME_ENGINE_H
#define DEEPFLOW_GAME_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "ecs.h"

// ── Screen constants ──────────────────────────────────────────────────────────
#define GE_SCREEN_W     1024
#define GE_SCREEN_H     768
#define GE_TILE_SIZE    16       // 16×16 pixel tiles
#define GE_MAP_W        64       // tiles per row
#define GE_MAP_H        64       // tiles per column
#define GE_MAX_ENTITIES 256
#define GE_MAX_ITEMS    64
#define GE_MAX_DIALOGUE 16       // max dialogue lines per NPC

// ── Tile flags ────────────────────────────────────────────────────────────────
#define TILE_SOLID      (1 << 0)   // blocks movement
#define TILE_WATER      (1 << 1)   // slows movement
#define TILE_DAMAGE     (1 << 2)   // deals 1 damage per tick
#define TILE_WARP       (1 << 3)   // triggers map transition
#define TILE_ANIMATED   (1 << 4)   // cycles through frames

// ── Tile definition ───────────────────────────────────────────────────────────
typedef struct {
    uint8_t  id;          // tile type index
    uint8_t  flags;       // TILE_* bitmask
    uint32_t color;       // base render color (Akatsuki palette)
    uint8_t  anim_frames; // number of animation frames (1 = static)
    uint8_t  anim_speed;  // ticks per frame
} tile_def_t;

// ── Tilemap layer ─────────────────────────────────────────────────────────────
typedef enum {
    LAYER_BG   = 0,   // ground / floor
    LAYER_COL  = 1,   // collision objects (walls, trees)
    LAYER_FG   = 2,   // foreground overlay (roofs, canopy)
    LAYER_COUNT = 3
} map_layer_t;

typedef struct {
    uint8_t  tiles[LAYER_COUNT][GE_MAP_H][GE_MAP_W];
    uint16_t w, h;           // actual used dimensions
    uint32_t bg_color;       // sky/void fill color
    uint8_t  warp_x[8];      // warp destination tile X
    uint8_t  warp_y[8];      // warp destination tile Y
    uint8_t  warp_map[8];    // destination map ID
} tilemap_t;

// ── Sprite animation ──────────────────────────────────────────────────────────
typedef enum {
    ANIM_IDLE_DOWN = 0,
    ANIM_IDLE_UP,
    ANIM_IDLE_LEFT,
    ANIM_IDLE_RIGHT,
    ANIM_WALK_DOWN,
    ANIM_WALK_UP,
    ANIM_WALK_LEFT,
    ANIM_WALK_RIGHT,
    ANIM_ATTACK_DOWN,
    ANIM_ATTACK_UP,
    ANIM_ATTACK_LEFT,
    ANIM_ATTACK_RIGHT,
    ANIM_HURT,
    ANIM_DEATH,
    ANIM_COUNT
} anim_state_t;

typedef struct {
    uint8_t     frame_count;    // how many frames in this state
    uint8_t     ticks_per_frame;
    uint32_t    colors[4];      // pixel color palette for this anim
} anim_def_t;

typedef struct {
    anim_state_t  state;
    anim_state_t  prev_state;
    uint8_t       frame;        // current frame index
    uint8_t       tick;         // ticks elapsed in this frame
    anim_def_t    defs[ANIM_COUNT];
} sprite_t;

// ── AABB collision box ────────────────────────────────────────────────────────
typedef struct {
    int32_t x, y;   // top-left in world pixels
    int32_t w, h;   // dimensions
} aabb_t;

// ── Entity direction ──────────────────────────────────────────────────────────
typedef enum {
    DIR_DOWN = 0,
    DIR_UP,
    DIR_LEFT,
    DIR_RIGHT
} direction_t;

// ── Game entity ───────────────────────────────────────────────────────────────
typedef struct {
    entity_id_t  ecs_id;
    bool         active;
    int32_t      world_x, world_y;   // position in world pixels
    int32_t      vel_x, vel_y;       // velocity (pixels per tick * 100)
    direction_t  facing;
    sprite_t     sprite;
    aabb_t       hitbox;
    int16_t      hp, hp_max;
    uint8_t      attack;
    uint8_t      defense;
    bool         is_player;
    bool         is_npc;
    bool         is_enemy;
    uint8_t      npc_dialogue_id;    // index into dialogue table
    uint8_t      item_drops[4];      // item IDs dropped on death
    uint8_t      item_drop_count;
} game_entity_t;

// ── Item system ───────────────────────────────────────────────────────────────
typedef enum {
    ITEM_NONE = 0,
    ITEM_SWORD,
    ITEM_BOW,
    ITEM_BOMB,
    ITEM_SHIELD,
    ITEM_HEART,          // restores 2 HP
    ITEM_KEY,
    ITEM_RUPEE_GREEN,    // 1 coin
    ITEM_RUPEE_BLUE,     // 5 coins
    ITEM_RUPEE_RED,      // 20 coins
    // Sports items
    ITEM_BASKETBALL,
    ITEM_FOOTBALL,
    ITEM_BASEBALL,
    ITEM_BAT,
    ITEM_COUNT
} item_id_t;

typedef struct {
    item_id_t id;
    const char* name;
    uint32_t    color;
    bool        stackable;
    uint8_t     quantity;
} item_t;

typedef struct {
    item_t  slots[GE_MAX_ITEMS];
    uint8_t count;
    uint8_t equipped_a;   // A-button item slot index
    uint8_t equipped_b;   // B-button item slot index
    uint32_t rupees;
} inventory_t;

// ── Dialogue system ───────────────────────────────────────────────────────────
typedef struct {
    const char* lines[GE_MAX_DIALOGUE];
    uint8_t     line_count;
    uint8_t     current_line;
    bool        active;
    const char* speaker_name;
    uint32_t    speaker_color;
} dialogue_t;

// ── Camera ────────────────────────────────────────────────────────────────────
typedef struct {
    int32_t x, y;          // top-left world pixel of viewport
    int32_t target_x, target_y;
    bool    locked;
} camera_t;

// ── Game state ────────────────────────────────────────────────────────────────
typedef enum {
    GS_OVERWORLD = 0,
    GS_DUNGEON,
    GS_BATTLE,
    GS_DIALOGUE,
    GS_INVENTORY,
    GS_GAME_OVER,
    GS_VICTORY,
    GS_SPORTS_HUB,
    GS_BASKETBALL,
    GS_FOOTBALL,
    GS_BASEBALL
} game_state_t;

// ── Voice command IDs for games ───────────────────────────────────────────────
#define GCMD_MOVE_UP        0x10
#define GCMD_MOVE_DOWN      0x11
#define GCMD_MOVE_LEFT      0x12
#define GCMD_MOVE_RIGHT     0x13
#define GCMD_ATTACK         0x14
#define GCMD_USE_ITEM       0x15
#define GCMD_BLOCK          0x16
#define GCMD_OPEN_INV       0x17
#define GCMD_CONFIRM        0x18
#define GCMD_CANCEL         0x19
// Sports
#define GCMD_PASS           0x20
#define GCMD_SHOOT          0x21
#define GCMD_DRIBBLE        0x22
#define GCMD_DEFEND         0x23
#define GCMD_HIKE           0x24
#define GCMD_RUN_LEFT       0x25
#define GCMD_RUN_RIGHT      0x26
#define GCMD_TACKLE         0x27
#define GCMD_PITCH_FAST     0x28
#define GCMD_PITCH_CURVE    0x29
#define GCMD_SWING          0x2A
#define GCMD_STEAL_BASE     0x2B

// ── Public API ────────────────────────────────────────────────────────────────
void         game_engine_init(void);
void         game_engine_tick(float dt);
void         game_engine_draw(void);
void         game_engine_handle_voice(uint32_t cmd_id);
void         game_engine_handle_key(uint8_t key);

// Tilemap
void         ge_load_map(tilemap_t* map);
void         ge_draw_map(camera_t* cam, map_layer_t layer);
bool         ge_tile_solid(tilemap_t* map, int32_t world_x, int32_t world_y);

// Entities
game_entity_t* ge_spawn_entity(int32_t x, int32_t y, bool is_player);
void           ge_destroy_entity(game_entity_t* e);
void           ge_move_entity(game_entity_t* e, int32_t dx, int32_t dy, tilemap_t* map);
void           ge_attack(game_entity_t* attacker, game_entity_t* target);
void           ge_update_sprite(game_entity_t* e);
void           ge_draw_entity(game_entity_t* e, camera_t* cam);

// Camera
void         ge_camera_follow(camera_t* cam, game_entity_t* target, tilemap_t* map);

// Dialogue
void         ge_dialogue_start(dialogue_t* d, const char* speaker, uint32_t color);
void         ge_dialogue_next(dialogue_t* d);
void         ge_dialogue_draw(dialogue_t* d);

// Inventory
bool         ge_inventory_add(inventory_t* inv, item_id_t id, uint8_t qty);
void         ge_inventory_draw(inventory_t* inv);
item_t*      ge_inventory_get_equipped(inventory_t* inv, uint8_t slot);

// Collision
bool         ge_aabb_overlap(aabb_t* a, aabb_t* b);

#endif // DEEPFLOW_GAME_ENGINE_H
