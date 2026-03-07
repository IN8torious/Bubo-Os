// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/settings_app.h — Header for CORVUS Settings App
// =============================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "session.h"
#include "framebuffer.h"
#include "polish.h"
#include "voice.h"

// Function prototypes for the CORVUS Settings App
void settings_init(void);
void settings_draw(void);
void settings_handle_key(uint8_t key);
void settings_voice_cmd(uint32_t cmd_id);
