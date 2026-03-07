// =============================================================================
// RAVEN ENGINE — Physics Engine Header
// Raven AOS v1.1 | Built by Nathan Samuel (IN8torious)
// "NO MAS DISADVANTAGED"
// =============================================================================

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
