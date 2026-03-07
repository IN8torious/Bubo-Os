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
// Instinct OS — CORVUS Dysarthria Adaptation Engine
//
// Built for Landon Pankuch. Built for anyone whose brain moves faster
// than their mouth. Built for anyone who has ever been told a machine
// couldn't understand them.
//
// This engine does NOT require perfect speech.
// It understands slurred words, partial words, merged words,
// dropped consonants, vowel distortion, and rapid-fire commands.
//
// Inspired by: jmaczan/asr-dysarthria (wav2vec2 fine-tuning on TORGO/UASpeech)
// Implemented as: bare-metal C phonetic adaptation layer for CORVUS
//
// "NO MAS DISADVANTAGED" — the machine adapts to the human.
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

// ── Configuration ─────────────────────────────────────────────────────────────
#define DYSARTHRIA_MAX_PHONEMES     64
#define DYSARTHRIA_MAX_VARIANTS     16   // slur variants per command
#define DYSARTHRIA_MAX_COMMANDS     20
#define DYSARTHRIA_HISTORY_SIZE     32   // rolling window of recent commands
#define DYSARTHRIA_CALIBRATION_SAMPLES 10

// Confidence threshold — below this, ask for confirmation
#define DYSARTHRIA_CONFIDENCE_HIGH  85   // 0-100 scale
#define DYSARTHRIA_CONFIDENCE_MED   60
#define DYSARTHRIA_CONFIDENCE_LOW   40

// ── Phoneme categories for slur detection ─────────────────────────────────────
// Dysarthric speech commonly affects:
//   - Consonant clusters (str→s, br→b, tr→t)
//   - Final consonants dropped (faster→faste, brake→brak)
//   - Vowel centralization (nitro→netro, neutro)
//   - Voiced/unvoiced confusion (brake→prake, drift→driff)
//   - Syllable reduction (overtake→overt, ovtak)
//   - Word merging when brain outruns mouth (turn left→ternlef)

typedef enum {
    PHON_STOP    = 0,   // p, b, t, d, k, g
    PHON_FRIC    = 1,   // f, v, s, z, sh, zh
    PHON_NASAL   = 2,   // m, n, ng
    PHON_LIQUID  = 3,   // l, r
    PHON_VOWEL   = 4,   // a, e, i, o, u
    PHON_APPROX  = 5,   // w, y
    PHON_UNKNOWN = 6
} phoneme_class_t;

// ── A single dysarthria-aware command entry ────────────────────────────────────
typedef struct {
    const char*  canonical;          // The "perfect" word: "FASTER"
    uint32_t     cmd_id;             // Maps to voice_cmd_t
    const char*  variants[DYSARTHRIA_MAX_VARIANTS]; // Known slur variants
    uint32_t     variant_count;
    uint32_t     min_match_len;      // Minimum chars needed for partial match
    uint32_t     hit_count;          // How many times Landon has used this
} dysarthria_cmd_t;

// ── Personal calibration profile ──────────────────────────────────────────────
// CORVUS learns Landon's specific speech patterns over time.
// Each session, it updates weights for his most common substitutions.
typedef struct {
    char     substitutions[26][4];   // char → most common replacement(s)
    uint8_t  sub_confidence[26];     // how confident we are in each sub
    uint32_t total_corrections;      // lifetime correction count
    uint32_t session_corrections;    // this session
    bool     calibrated;             // has enough data to trust profile
} dysarthria_profile_t;

// ── Match result ──────────────────────────────────────────────────────────────
typedef struct {
    uint32_t    cmd_id;              // matched command ID (voice_cmd_t)
    uint8_t     confidence;          // 0-100
    const char* canonical;           // what CORVUS heard it as
    const char* original;            // what was actually said
    bool        was_corrected;       // did dysarthria engine change it?
    bool        needs_confirm;       // confidence too low, ask Landon
} dysarthria_match_t;

// ── Engine state ──────────────────────────────────────────────────────────────
typedef struct {
    bool                 active;
    bool                 learning_mode;      // adapt to new patterns
    dysarthria_profile_t profile;
    uint32_t             history[DYSARTHRIA_HISTORY_SIZE];
    uint32_t             history_head;
    uint32_t             total_processed;
    uint32_t             total_corrected;
    uint32_t             total_failed;
} dysarthria_state_t;

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize the engine with Landon's default profile
void dysarthria_init(void);

// Core function: take raw text input (possibly slurred), return best match
dysarthria_match_t dysarthria_match(const char* raw_input);

// Feed confirmed correction back to engine (learning)
void dysarthria_confirm(const char* raw_input, uint32_t correct_cmd_id);

// Feed rejection back to engine (learning)
void dysarthria_reject(const char* raw_input);

// Get current engine state
dysarthria_state_t* dysarthria_get_state(void);

// Print calibration stats to terminal
void dysarthria_print_stats(void);

// Reset personal profile (start fresh calibration)
void dysarthria_reset_profile(void);

// Internal helpers (exposed for testing)
uint32_t dysarthria_edit_distance(const char* a, const char* b);
uint32_t dysarthria_phonetic_distance(const char* a, const char* b);
bool     dysarthria_is_prefix_match(const char* input, const char* target, uint32_t min_len);
void     dysarthria_normalize(const char* input, char* output, uint32_t max_len);
