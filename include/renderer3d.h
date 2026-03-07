// =============================================================================
// INSTINCT ENGINE — 3D Renderer Header
// Instinct OS v1.1 | Built by Nathan Samuel (IN8torious)
// "NO MAS DISADVANTAGED"
// =============================================================================

#ifndef RENDERER3D_H
#define RENDERER3D_H

#include "ecs.h"
#include <stdint.h>

void renderer3d_init(void);
void renderer3d_set_camera(vec3_t pos, vec3_t look_at);
void renderer3d_clear(uint32_t sky_color);
void renderer3d_draw_scene(scene_id_t scene);

#endif
