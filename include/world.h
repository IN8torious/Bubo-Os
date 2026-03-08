// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SCENE_NONE        = 0,
    SCENE_EARTH       = 1,  // Boot cinematic — full Earth view
    SCENE_NURBURGRING = 2,  // Racing world — Nürburgring Nordschleife
    SCENE_CORVUS_HOME = 3,  // CORVUS Home — 3D living space
    SCENE_CUSTOM      = 4,  // User-defined GPS location
} world_scene_t;

typedef struct {
    float    lat, lon;
    float    width, depth, height;
    uint32_t color;
} osm_building_t;

void           world_init(void);
void           world_load_scene(world_scene_t scene);
void           world_set_camera_gps(float lat, float lon, float alt);
void           world_render_top_down(void);
world_scene_t  world_get_current_scene(void);
uint32_t       world_get_building_count(void);
osm_building_t* world_get_buildings(void);
uint32_t       world_nurb_waypoint_count(void);
void           world_nurb_waypoint(uint32_t idx, float* lat, float* lon);

#endif
