#pragma once
#include <stdint.h>
#include <stdbool.h>

void corvus_draw_boot_screen(void);
void corvus_draw_boot_progress(uint32_t percent, const char* label);
void corvus_draw_desktop(void);
void corvus_draw_agent_panel(int32_t x, int32_t y);
void corvus_draw_shell_window(int32_t x, int32_t y, int32_t w, int32_t h);
void corvus_draw_memory_bar(int32_t x, int32_t y, int32_t w,
                             uint32_t used_kb, uint32_t total_kb);
void corvus_draw_dashboard(void);
