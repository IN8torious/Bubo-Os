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
// Instinct OS — Voice Recognition Engine Header
// Single source of truth for all VCMD_* command IDs
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

#define VOICE_PCM_BUF    4096
#define VOICE_TEXT_BUF   256
#define VAD_THRESHOLD    800
#define VAD_SILENCE_MS   500

// All voice command IDs — used by voice engine, landon center, kernel, LLM
typedef enum {
    VCMD_FASTER     = 0,
    VCMD_BRAKE      = 1,
    VCMD_DRIFT      = 2,
    VCMD_NITRO      = 3,
    VCMD_OVERTAKE   = 4,
    VCMD_TURN_LEFT  = 5,
    VCMD_TURN_RIGHT = 6,
    VCMD_PIT_STOP   = 7,
    VCMD_STATUS     = 8,
    VCMD_STOP       = 9,
    VCMD_LAUNCH     = 10,
    VCMD_MUSIC      = 11,
    VCMD_HELP       = 12,
    VCMD_CORVUS     = 13,
    VCMD_MANDATE    = 14,
    VCMD_UNKNOWN    = 15,
    VCMD_NONE       = 16,
    VCMD_MAX        = 17
} voice_cmd_t;

typedef struct {
    const char*  keyword;
    voice_cmd_t  cmd;
    const char*  response;
} voice_keyword_t;

typedef struct {
    bool        active;
    bool        listening;
    bool        speech_detected;
    voice_cmd_t last_cmd;
    uint32_t    cmd_count;
    uint32_t    silence_ms;
    uint32_t    energy;
} voice_state_t;

typedef void (*voice_cmd_callback_t)(voice_cmd_t cmd, const char* response);

bool           voice_init(void);
void           voice_set_callback(voice_cmd_callback_t cb);
voice_cmd_t    voice_process_text(const char* text);
voice_cmd_t    voice_process_pcm(const int16_t* samples, uint32_t count);
void           voice_tick(void);
void           voice_start_listening(void);
void           voice_stop_listening(void);
voice_state_t* voice_get_state(void);
const char*    voice_cmd_name(voice_cmd_t cmd);
