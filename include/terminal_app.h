#pragma once
#include <stdint.h>
void terminal_app_init(void);
void terminal_app_render(uint32_t wx, uint32_t wy, uint32_t ww, uint32_t wh);
void terminal_app_key(char key);
