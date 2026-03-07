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
// =============================================================================
// Instinct OS — Sound Driver Header (AC97 / Intel HDA)
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     initialized;
    bool     capture_active;
    bool     playback_active;
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bits;
} sound_state_t;

bool          sound_init(void);
void          sound_start_capture(void);
void          sound_stop_capture(void);
uint32_t      sound_read_capture(int16_t* buf, uint32_t max_samples);
void          sound_beep(uint32_t freq_hz, uint32_t duration_ms);
void          sound_corvus_ack(void);
void          sound_corvus_ready(void);
void          sound_corvus_error(void);
void          sound_nitro_activate(void);
sound_state_t* sound_get_state(void);
