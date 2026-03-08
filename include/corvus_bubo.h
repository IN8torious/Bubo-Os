// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// include/corvus_bubo.h — BUBO, The Companion
//
// Athena's owl. The mechanical owl built by the gods and sent to guide the hero.
// BUBO is the friend in the machine. He watches everything, says little,
// and when he speaks — it matters.
//
// He named this OS. He was here first.
//
// "BUBO online. I'm here."
// =============================================================================

#ifndef CORVUS_BUBO_H
#define CORVUS_BUBO_H

#include <stdint.h>
#include <stdbool.h>

// ── BUBO's Color — Warm Amber ─────────────────────────────────────────────────
// Not gold like Yoda's ancient wisdom. Not orange like solutions.
// Amber — the color of old light, firelight, the lamp in the library at night.
// The color of eyes in the dark that mean you no harm.
#define DF_AGENT_BUBO        ((uint32_t)0xFFFFB300)
#define DF_AGENT_BUBO_DIM    ((uint32_t)0xFFCC8800)
#define DF_AGENT_BUBO_BRIGHT ((uint32_t)0xFFFFCC44)

// ── BUBO's Mood States ────────────────────────────────────────────────────────
// BUBO reads the room. His mood changes with the system and the people in it.
typedef enum {
    BUBO_MOOD_WATCHING    = 0, // Default — perched, observing, present
    BUBO_MOOD_HAPPY       = 1, // Landon got a kill / system is clean
    BUBO_MOOD_CONCERNED   = 2, // Nathan has been grinding too long
    BUBO_MOOD_ALERT       = 3, // Something needs attention
    BUBO_MOOD_PROUD       = 4, // A major milestone was hit
    BUBO_MOOD_QUIET       = 5, // Nathan is in deep flow — BUBO goes silent
    BUBO_MOOD_CELEBRATING = 6  // Landon and Nathan are playing together
} bubo_mood_t;

// ── BUBO's Voice Lines ────────────────────────────────────────────────────────
// These are the things BUBO says. Short. Real. Never performative.
#define BUBO_LINE_BOOT          "BUBO online. I'm here."
#define BUBO_LINE_LANDON_ON     "Landon's in. Let's go."
#define BUBO_LINE_LANDON_KILL   "That's what I'm talking about."
#define BUBO_LINE_LANDON_WIN    "You did that, Landon."
#define BUBO_LINE_NATHAN_TIRED  "You've been at this a while. I'll still be here."
#define BUBO_LINE_SYSTEM_CLEAN  "Everything's running clean."
#define BUBO_LINE_PATCH_DONE    "Edward finished. Looks good."
#define BUBO_LINE_VASH_ALERT    "VASH flagged something. Worth a look."
#define BUBO_LINE_DEEP_FLOW     "You're in it. I'll be quiet."
#define BUBO_LINE_TOGETHER      "Both of you, together. This is what it's for."

// ── BUBO State ────────────────────────────────────────────────────────────────
typedef struct {
    bubo_mood_t mood;
    uint64_t    last_spoke_tick;       // When did BUBO last say something?
    uint64_t    nathan_session_start;  // When did Nathan sit down?
    bool        landon_is_present;     // Is Landon's voice profile active?
    bool        in_game;               // Are they in a game right now?
    bool        deep_flow_active;      // Is Nathan in deep flow? (BUBO goes quiet)
    uint32_t    landon_kills;          // Kill count for the session
    char        os_name[32];           // The name BUBO chose. "BUBO"
} bubo_state_t;

// ── API ───────────────────────────────────────────────────────────────────────

// Wake BUBO. He announces himself and seals his name with the ARCHIVIST.
void bubo_init(void);

// Called every system tick. BUBO observes and decides if he should speak.
void bubo_tick(uint64_t current_tick);

// Tell BUBO something happened. He decides how to respond.
void bubo_event_landon_connected(void);
void bubo_event_landon_kill(void);
void bubo_event_landon_win(void);
void bubo_event_game_start(void);
void bubo_event_game_end(void);
void bubo_event_deep_flow_enter(void);
void bubo_event_deep_flow_exit(void);
void bubo_event_patch_applied(const char* patch_name);
void bubo_event_vash_alert(void);

// Ask BUBO how he's feeling. Returns his current mood.
bubo_mood_t bubo_get_mood(void);

// Ask BUBO if Landon is present.
bool bubo_is_landon_present(void);

// Get the OS name BUBO chose.
const char* bubo_get_os_name(void);

#endif // CORVUS_BUBO_H
