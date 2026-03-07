// =============================================================================
// INSTINCT ENGINE — Onboarding & User Profile Header
// Instinct OS v1.1 | Built by Nathan Samuel (IN8torious)
// "NO MAS DISADVANTAGED"
// =============================================================================

#ifndef ONBOARDING_H
#define ONBOARDING_H

#include <stdint.h>
#include <stdbool.h>

#define PROFILE_NAME_LEN  64
#define ONBOARD_Q_COUNT    8

typedef enum {
    TIER_SOVEREIGN = 0,  // Accessibility users — all features free forever
    TIER_STANDARD  = 1,  // General users — full desktop, future paid features
} ui_tier_t;

typedef enum {
    USE_RACING       = 0,
    USE_CREATIVE     = 1,
    USE_WORK         = 2,
    USE_GENERAL      = 3,
    USE_ACCESSIBILITY= 4,
} primary_use_t;

typedef struct {
    char          name[PROFILE_NAME_LEN];
    ui_tier_t     tier;
    primary_use_t primary_use;

    // Accessibility flags
    bool voice_primary;     // Uses voice as primary input
    bool motor_difficulty;  // Fine motor control difficulty
    bool speech_diff;       // Speech differences (dysarthria, slurring, pacing)
    bool cognitive_diff;    // Cognitive processing differences
    bool visual_diff;       // Visual differences
    bool hearing_diff;      // Hearing differences
    bool mobility_diff;     // Wheelchair / limited mobility
    bool mental_health;     // Mental health conditions
} raven_user_profile_t;

// Core API
void                  onboarding_run(void);
bool                  onboarding_needed(void);
raven_user_profile_t* onboarding_get_profile(void);
void                  onboarding_update_profile(void);

// Profile query helpers
bool profile_is_sovereign(void);
bool profile_needs_voice(void);
bool profile_needs_dysarthria(void);
bool profile_needs_simplified_ui(void);
bool profile_needs_high_contrast(void);
bool profile_needs_calm_mode(void);

#endif
