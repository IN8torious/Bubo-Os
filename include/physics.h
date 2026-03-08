// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#ifndef PHYSICS_H
#define PHYSICS_H

#include "ecs.h"
#include <stdint.h>

void  physics_init(void);
void  physics_tick(float dt);
void  physics_impulse(entity_id_t id, vec3_t force);
void  physics_teleport(entity_id_t id, vec3_t pos);
float physics_get_speed_kmh(entity_id_t id);

#endif
