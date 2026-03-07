// =============================================================================
// Instinct OS — Dedicated to Landon Pankuch
// =============================================================================
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

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
