// =============================================================================
// Deep Flow OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/live_mode.h — Header for CORVUS Live Mode
// =============================================================================

#pragma once

#include <stdbool.h>

// Initializes the live mode system
void live_mode_init();

// Enables or disables the live mode overlay
void live_mode_enable(bool enable);

// Performs periodic updates for live mode (e.g., CPU/memory usage)
void live_mode_tick();

// Draws the live mode overlay to the framebuffer
void live_mode_draw();
