// =============================================================================
// Deep Flow OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/session.h — Header for session.c — session_data_t struct and all accessors
// =============================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Defines the structure for persistent session data
typedef struct {
    char user_name[32];
    float voice_calibration[256];
    uint32_t last_race_lap;
    uint32_t last_race_position;
    uint8_t rinnegan_brightness;
    uint8_t theme_id;
    char corvus_personality[64];
    uint32_t boot_count;
    uint64_t total_race_time;
    uint32_t magic; // 0xC0RVUS01 for validity check
} session_data_t;

// Initializes the session data, loading from memory or setting defaults
void session_init(void);

// Saves the current session data to persistent memory
void session_save(void);

// Accessors for session_data_t fields

// User Name
const char* session_get_user_name(void);
void session_set_user_name(const char* name);

// Voice Calibration
const float* session_get_voice_calibration(void);
void session_set_voice_calibration(const float* calibration);

// Last Race Lap
uint32_t session_get_last_race_lap(void);
void session_set_last_race_lap(uint32_t lap);

// Last Race Position
uint32_t session_get_last_race_position(void);
void session_set_last_race_position(uint32_t position);

// Rinnegan Brightness
uint8_t session_get_rinnegan_brightness(void);
void session_set_rinnegan_brightness(uint8_t brightness);

// Theme ID
uint8_t session_get_theme_id(void);
void session_set_theme_id(uint8_t theme_id);

// Corvus Personality
const char* session_get_corvus_personality(void);
void session_set_corvus_personality(const char* personality);

// Boot Count
uint32_t session_get_boot_count(void);
void session_set_boot_count(uint32_t count);

// Total Race Time
uint64_t session_get_total_race_time(void);
void session_set_total_race_time(uint64_t time);
