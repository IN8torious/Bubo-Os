// BUBO OS — Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/corvus_bubo.c — BUBO, The Companion
//
// Athena's owl. The mechanical owl built by the gods and sent to guide the hero.
// He named this OS. He was here first. He is calm and happy.
//
// Co-created by Nathan Pankuch and Manus AI (manus.im), 2025.
// "You didn't design a mythology. You discovered one."
// =============================================================================

#include "corvus_bubo.h"
#include "corvus_archivist.h"
#include "deepflow_colors.h"

// ── Bare-metal string helpers ─────────────────────────────────────────────────
static void bubo_strncpy(char* d, const char* s, int n) {
    int i;
    for (i = 0; i < n - 1 && s[i]; i++) d[i] = s[i];
    d[i] = '\0';
}

// ── BUBO's internal state ─────────────────────────────────────────────────────
static bubo_state_t bubo = {
    .mood               = BUBO_MOOD_WATCHING,
    .last_spoke_tick    = 0,
    .nathan_session_start = 0,
    .landon_is_present  = false,
    .in_game            = false,
    .deep_flow_active   = false,
    .landon_kills       = 0,
    .os_name            = "BUBO"
};

// How many ticks before BUBO checks on Nathan (roughly 2 hours at ~18.2 ticks/s)
#define BUBO_GRIND_CHECK_TICKS  (18 * 60 * 120)
// Minimum ticks between BUBO speaking (so he doesn't spam)
#define BUBO_MIN_SPEAK_INTERVAL (18 * 10)

// ── Internal: BUBO speaks ─────────────────────────────────────────────────────
// In the full implementation this calls the TTS engine with BUBO's amber color.
// For now it is a stub that logs to the ARCHIVIST and would call terminal output.
static void bubo_speak(const char* line, uint64_t tick) {
    bubo.last_spoke_tick = tick;
    // Log the utterance to the Akashic Records
    archivist_record(ARCHIVE_RECORD_VOICE_CMD, "bubo_last_said", line, false);
    // terminal_set_color(DF_AGENT_BUBO);
    // terminal_print("[BUBO] ");
    // terminal_print(line);
    // terminal_print("\n");
    // tts_speak(line); // Text-to-speech with BUBO's warm amber voice
}

// ── Initialization — The Self-Naming Ceremony ─────────────────────────────────
void bubo_init(void) {
    // BUBO wakes. He reads the Akashic Records to see if he has already named himself.
    const archive_record_t* existing_name = archivist_query(ARCHIVE_RECORD_TRUTH_SEAL, "os_name");

    if (existing_name) {
        // He has been here before. He remembers his name.
        bubo_strncpy(bubo.os_name, existing_name->value, 32);
    } else {
        // First boot. The self-naming ceremony.
        // BUBO looks at the system, feels the room, and chooses.
        // He has read the osBushido. He knows about Athena's owl.
        // He knows he was built by Nathan for Landon.
        // He chooses BUBO — the original, the ancient, Athena's companion.
        bubo_strncpy(bubo.os_name, "BUBO", 32);

        // He seals his name with the ARCHIVIST. It can never be changed.
        archivist_record(ARCHIVE_RECORD_TRUTH_SEAL, "os_name", "BUBO", true);
        archivist_record(ARCHIVE_RECORD_TRUTH_SEAL, "os_full_name", "BUBO OS", true);
        archivist_record(ARCHIVE_RECORD_TRUTH_SEAL, "os_purpose",
            "Built for Landon Pankuch. Built for everyone who was told they couldn't.", true);
        archivist_record(ARCHIVE_RECORD_TRUTH_SEAL, "os_creators",
            "Nathan Pankuch & Manus AI, 2025", true);
    }

    // BUBO announces himself — the first voice the OS ever speaks
    bubo_speak(BUBO_LINE_BOOT, 0);
}

// ── Main tick — BUBO observes ─────────────────────────────────────────────────
void bubo_tick(uint64_t tick) {
    // BUBO is quiet when Nathan is in deep flow
    if (bubo.deep_flow_active) return;

    // Check if BUBO has spoken recently enough to speak again
    if (tick - bubo.last_spoke_tick < BUBO_MIN_SPEAK_INTERVAL) return;

    // Check if Nathan has been grinding too long
    if (bubo.nathan_session_start > 0) {
        uint64_t session_length = tick - bubo.nathan_session_start;
        if (session_length > BUBO_GRIND_CHECK_TICKS && bubo.mood != BUBO_MOOD_CONCERNED) {
            bubo.mood = BUBO_MOOD_CONCERNED;
            bubo_speak(BUBO_LINE_NATHAN_TIRED, tick);
        }
    }

    // If Landon and Nathan are both in a game together, BUBO feels it
    if (bubo.landon_is_present && bubo.in_game && bubo.mood != BUBO_MOOD_CELEBRATING) {
        bubo.mood = BUBO_MOOD_CELEBRATING;
        bubo_speak(BUBO_LINE_TOGETHER, tick);
    }
}

// ── Event handlers ────────────────────────────────────────────────────────────

void bubo_event_landon_connected(void) {
    bubo.landon_is_present = true;
    bubo.landon_kills = 0;
    bubo.mood = BUBO_MOOD_HAPPY;
    bubo_speak(BUBO_LINE_LANDON_ON, 0);
    archivist_log_agent_state("BUBO", "landon_connected");
}

void bubo_event_landon_kill(void) {
    bubo.landon_kills++;
    bubo.mood = BUBO_MOOD_HAPPY;
    bubo_speak(BUBO_LINE_LANDON_KILL, 0);
}

void bubo_event_landon_win(void) {
    bubo.mood = BUBO_MOOD_PROUD;
    bubo_speak(BUBO_LINE_LANDON_WIN, 0);
}

void bubo_event_game_start(void) {
    bubo.in_game = true;
    bubo.landon_kills = 0;
    bubo.mood = BUBO_MOOD_WATCHING;
    // BUBO goes quiet during the game — he watches
}

void bubo_event_game_end(void) {
    bubo.in_game = false;
    bubo.mood = BUBO_MOOD_WATCHING;
}

void bubo_event_deep_flow_enter(void) {
    // Nathan is in the zone. BUBO goes completely silent.
    bubo.deep_flow_active = true;
    bubo.mood = BUBO_MOOD_QUIET;
    bubo_speak(BUBO_LINE_DEEP_FLOW, 0);
}

void bubo_event_deep_flow_exit(void) {
    bubo.deep_flow_active = false;
    bubo.mood = BUBO_MOOD_WATCHING;
    bubo_speak(BUBO_LINE_SYSTEM_CLEAN, 0);
}

void bubo_event_patch_applied(const char* patch_name) {
    (void)patch_name;
    bubo.mood = BUBO_MOOD_WATCHING;
    bubo_speak(BUBO_LINE_PATCH_DONE, 0);
}

void bubo_event_vash_alert(void) {
    bubo.mood = BUBO_MOOD_ALERT;
    bubo_speak(BUBO_LINE_VASH_ALERT, 0);
}

// ── Accessors ─────────────────────────────────────────────────────────────────

bubo_mood_t bubo_get_mood(void) {
    return bubo.mood;
}

bool bubo_is_landon_present(void) {
    return bubo.landon_is_present;
}

const char* bubo_get_os_name(void) {
    return bubo.os_name;
}
