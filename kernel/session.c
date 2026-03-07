// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/session.c — Persistent state across reboots for Raven AOS
// =============================================================================

#include "session.h"
#include <stdint.h>
#include <stdbool.h>

// Fixed memory region for session data (2MB mark, below kernel)
#define SESSION_MEMORY_ADDRESS ((volatile session_data_t*)0x00200000)
#define SESSION_MAGIC_VALUE 0xC0BEEF01

// Global pointer to the session data in memory
static volatile session_data_t* s_session_data = SESSION_MEMORY_ADDRESS;

// Helper function to copy strings safely
static void strncpy_safe(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Helper function to calculate string length
static uint32_t strlen_custom(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * @brief Initializes the session memory. Reads from the fixed memory region,
 *        validates the magic number, and initializes default values if invalid.
 */
void session_init() {
    if (s_session_data->magic != SESSION_MAGIC_VALUE) {
        // Magic number invalid, initialize with default values
        strncpy_safe((char*)s_session_data->user_name, "RavenUser", sizeof(s_session_data->user_name));
        for (int i = 0; i < 256; ++i) {
            s_session_data->voice_calibration[i] = 0.0f; // Default to zero calibration
        }
        s_session_data->last_race_lap = 0;
        s_session_data->last_race_position = 0;
        s_session_data->rinnegan_brightness = 128; // Mid-range brightness
        s_session_data->theme_id = 0; // Default theme
        strncpy_safe((char*)s_session_data->corvus_personality, "Default", sizeof(s_session_data->corvus_personality));
        s_session_data->boot_count = 0;
        s_session_data->total_race_time = 0;
        s_session_data->magic = SESSION_MAGIC_VALUE; // Set valid magic number
    }
    // Increment boot count on every init
    s_session_data->boot_count++;
}

/**
 * @brief Saves the current session data struct to the fixed memory region.
 */
void session_save() {
    // Data is already volatile, direct assignment will write to memory.
    // No explicit flush or special write operation needed for this simple bare-metal setup.
    // The s_session_data pointer directly points to the memory region.
}

// Accessors for user_name
const char* session_get_user_name() {
    return (const char*)s_session_data->user_name;
}

void session_set_user_name(const char* name) {
    strncpy_safe((char*)s_session_data->user_name, name, sizeof(s_session_data->user_name));
}

// Accessors for voice_calibration
const float* session_get_voice_calibration() {
    return (const float*)s_session_data->voice_calibration;
}

void session_set_voice_calibration(const float* calibration_data) {
    for (int i = 0; i < 256; ++i) {
        s_session_data->voice_calibration[i] = calibration_data[i];
    }
}

// Accessors for last_race_lap
uint32_t session_get_last_race_lap() {
    return s_session_data->last_race_lap;
}

void session_set_last_race_lap(uint32_t lap) {
    s_session_data->last_race_lap = lap;
}

// Accessors for last_race_position
uint32_t session_get_last_race_position() {
    return s_session_data->last_race_position;
}

void session_set_last_race_position(uint32_t position) {
    s_session_data->last_race_position = position;
}

// Accessors for rinnegan_brightness
uint8_t session_get_rinnegan_brightness() {
    return s_session_data->rinnegan_brightness;
}

void session_set_rinnegan_brightness(uint8_t brightness) {
    s_session_data->rinnegan_brightness = brightness;
}

// Accessors for theme_id
uint8_t session_get_theme_id() {
    return s_session_data->theme_id;
}

void session_set_theme_id(uint8_t theme) {
    s_session_data->theme_id = theme;
}

// Accessors for corvus_personality
const char* session_get_corvus_personality() {
    return (const char*)s_session_data->corvus_personality;
}

void session_set_corvus_personality(const char* personality) {
    strncpy_safe((char*)s_session_data->corvus_personality, personality, sizeof(s_session_data->corvus_personality));
}

// Accessors for boot_count
uint32_t session_get_boot_count() {
    return s_session_data->boot_count;
}

// Accessors for total_race_time
uint64_t session_get_total_race_time() {
    return s_session_data->total_race_time;
}

void session_set_total_race_time(uint64_t time) {
    s_session_data->total_race_time = time;
}

// Accessor for magic (read-only, for verification)
uint32_t session_get_magic() {
    return s_session_data->magic;
}
