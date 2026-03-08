// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#ifndef RENDERER3D_H
#define RENDERER3D_H

#include "ecs.h"
#include <stdint.h>

void renderer3d_init(void);
void renderer3d_set_camera(vec3_t pos, vec3_t look_at);
void renderer3d_clear(uint32_t sky_color);
void renderer3d_draw_scene(scene_id_t scene);

#endif
