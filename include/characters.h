// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#ifndef CHARACTERS_H
#define CHARACTERS_H

#include "ecs.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ANIM_IDLE = 0,
    ANIM_WALK,
    ANIM_RUN,
    ANIM_SIT,
    ANIM_WAVE,
    ANIM_FLY,
} anim_state_t;

// animal_type_t defined in ecs.h

// Forward declarations (full structs in .c)
typedef struct character_t character_t;
typedef struct animal_t    animal_t;

void         characters_init(void);
void         characters_tick(float dt);
void         characters_on_voice_cmd(const char* cmd);
character_t* characters_find(const char* name);
character_t* characters_get(uint32_t idx);
animal_t*    animals_get(uint32_t idx);
uint32_t     characters_get_count(void);
uint32_t     animals_get_count(void);

#endif
