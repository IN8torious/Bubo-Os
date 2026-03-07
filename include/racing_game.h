#pragma once
#include <stdint.h>

void        racing_game_init(void);
void        racing_game_tick(void);
void        racing_voice_input(const char* text);
const char* racing_get_corvus_response(void);
