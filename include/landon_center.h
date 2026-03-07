#pragma once
// =============================================================================
// Raven AOS — Landon Pankuch Accessibility Center Header
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

// Voice command IDs
typedef enum {
    VCMD_FASTER   = 0,
    VCMD_BRAKE    = 1,
    VCMD_DRIFT    = 2,
    VCMD_NITRO    = 3,
    VCMD_OVERTAKE = 4,
    VCMD_LEFT     = 5,
    VCMD_RIGHT    = 6,
    VCMD_PIT      = 7,
    VCMD_STATUS   = 8,
    VCMD_STOP     = 9,
    VCMD_LAUNCH   = 10,
    VCMD_MUSIC    = 11,
    VCMD_HELP     = 12,
} landon_vcmd_id_t;

typedef struct {
    const char*      phrase;
    landon_vcmd_id_t id;
    const char*      response;
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
