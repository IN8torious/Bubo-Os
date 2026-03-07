// =============================================================================
// Raven AOS — Dedicated to Landon Pankuch
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
// =============================================================================
// Raven AOS — Landon Pankuch Accessibility Center Header
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

// NOTE: Voice command enum (VCMD_*) is defined in voice.h
// landon_center.h uses uint32_t cmd_id to avoid circular includes

typedef struct {
    const char*  phrase;
    uint32_t     id;
    const char*  response;
} landon_voice_cmd_t;

typedef struct {
    bool     active;
    bool     voice_active;
    bool     race_running;
    bool     corvus_driving;
    bool     nitro_available;
    uint32_t speed_mph;
    uint32_t lap;
    uint32_t position;
} landon_center_t;

// Public API
void landon_center_init(void);
void landon_center_render(uint32_t wx, uint32_t wy, uint32_t ww, uint32_t wh);
void landon_process_voice(const char* cmd);
void landon_execute_cmd(uint32_t cmd_id);
void landon_center_key(char key);
void landon_center_tick(void);

// String helpers
void itoa_simple(uint32_t n, char* buf);
void str_append(char* dst, const char* src);
void str_copy(char* dst, const char* src, int max);
bool str_match(const char* a, const char* b);
