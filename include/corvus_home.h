// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#ifndef CORVUS_HOME_H
#define CORVUS_HOME_H

#include "ecs.h"
#include <stdbool.h>

void        corvus_home_init(void);
void        corvus_home_open(void);
void        corvus_home_close(void);
void        corvus_home_minimize(void);
bool        corvus_home_is_open(void);
void        corvus_home_tick(float dt);
void        corvus_home_draw(void);
void        corvus_home_voice(const char* cmd);

void        corvus_home_focus_nathan(void);
void        corvus_home_focus_corvus(void);
void        corvus_home_focus_landon(void);
void        corvus_home_focus_raphael(void);
void        corvus_home_focus_garage(void);
void        corvus_home_look_at(entity_id_t target);

entity_id_t corvus_home_get_nathan(void);
entity_id_t corvus_home_get_corvus(void);
entity_id_t corvus_home_get_landon(void);
entity_id_t corvus_home_get_raphael(void);
entity_id_t corvus_home_get_huginn(void);
entity_id_t corvus_home_get_demon170(void);

#endif
