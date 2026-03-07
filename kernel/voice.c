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

// =============================================================================
// Raven AOS — Voice Recognition Engine
//
// Voice Activity Detection (VAD) + keyword spotting for Landon's commands.
// This is the pipeline: microphone → PCM → VAD → keyword match → CORVUS action.
//
// "NO MAS DISADVANTAGED" — Landon speaks, CORVUS acts.
// =============================================================================

#include "voice.h"
#include "sound.h"
#include "vga.h"
#include "corvus_constitution.h"
#include "dysarthria.h"
#include <stdint.h>
#include <stdbool.h>

// ── Voice command table ───────────────────────────────────────────────────────
static const voice_keyword_t KEYWORDS[] = {
    { "FASTER",    VCMD_FASTER,    "CORVUS: Increasing throttle — Demon 170 accelerating" },
    { "BRAKE",     VCMD_BRAKE,     "CORVUS: Braking — Brembo calipers engaged" },
    { "DRIFT",     VCMD_DRIFT,     "CORVUS: Initiating controlled drift sequence" },
    { "NITRO",     VCMD_NITRO,     "CORVUS: NITRO ACTIVATED — 1,400 HP UNLEASHED" },
    { "OVERTAKE",  VCMD_OVERTAKE,  "CORVUS: Planning overtake — calculating gap" },
    { "TURN LEFT", VCMD_TURN_LEFT, "CORVUS: Steering left" },
    { "TURN RIGHT",VCMD_TURN_RIGHT,"CORVUS: Steering right" },
    { "PIT STOP",  VCMD_PIT_STOP,  "CORVUS: Pit stop — entering pit lane" },
    { "LAUNCH",    VCMD_LAUNCH,    "CORVUS: Launch control — 3... 2... 1... GO" },
    { "STATUS",    VCMD_STATUS,    "CORVUS: Reporting race status" },
    { "STOP",      VCMD_STOP,      "CORVUS: Full stop — all systems halting" },
    { "HELP",      VCMD_HELP,      "CORVUS: Commands: FASTER BRAKE DRIFT NITRO OVERTAKE LAUNCH STATUS STOP" },
    { "CORVUS",    VCMD_CORVUS,    "CORVUS: Sovereign intelligence online — ready for you" },
    { "MANDATE",   VCMD_MANDATE,   "CORVUS: NO MAS DISADVANTAGED — constitutional mandate active" },
};
#define KEYWORD_COUNT 14

// ── Driver state ──────────────────────────────────────────────────────────────
static voice_state_t g_voice;
static int16_t g_pcm_buf[VOICE_PCM_BUF];
static char    g_text_buf[VOICE_TEXT_BUF];
static voice_cmd_callback_t g_callback = 0;

// ── String helpers ────────────────────────────────────────────────────────────
static int vs_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}
static bool vs_streq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}
static bool vs_contains(const char* haystack, const char* needle) {
    int hl = vs_strlen(haystack), nl = vs_strlen(needle);
    for (int i = 0; i <= hl - nl; i++) {
        bool match = true;
        for (int j = 0; j < nl; j++) {
            char h = haystack[i+j], n2 = needle[j];
            // Case-insensitive
            if (h >= 'a' && h <= 'z') h -= 32;
            if (n2 >= 'a' && n2 <= 'z') n2 -= 32;
            if (h != n2) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

// ── Energy-based Voice Activity Detection ────────────────────────────────────
// Returns RMS energy of a PCM window
static uint32_t vad_energy(const int16_t* samples, uint32_t count) {
    uint64_t sum = 0;
    for (uint32_t i = 0; i < count; i++) {
        int32_t s = samples[i];
        sum += (uint64_t)(s * s);
    }
    // Integer sqrt approximation
    uint64_t rms = sum / (count ? count : 1);
    uint64_t x = rms;
    if (x == 0) return 0;
    uint64_t r = x;
    for (int i = 0; i < 20; i++) r = (r + x / r) / 2;
    return (uint32_t)r;
}

// ── Initialize voice engine ───────────────────────────────────────────────────
bool voice_init(void) {
    terminal_write("[VOICE] Initializing voice recognition engine...\n");

    g_voice.active        = false;
    g_voice.listening     = false;
    g_voice.speech_detected = false;
    g_voice.last_cmd      = VCMD_NONE;
    g_voice.cmd_count     = 0;
    g_voice.silence_ms    = 0;
    g_voice.energy        = 0;

    for (int i = 0; i < VOICE_PCM_BUF; i++) g_pcm_buf[i] = 0;
    for (int i = 0; i < VOICE_TEXT_BUF; i++) g_text_buf[i] = 0;

    g_voice.active = true;

    // Initialize dysarthria adaptation engine
    dysarthria_init();

    terminal_write("[VOICE] Voice engine ready — dysarthria-aware pipeline active\n");
    terminal_write("[VOICE] Commands: FASTER BRAKE DRIFT NITRO OVERTAKE LAUNCH STATUS STOP\n");
    terminal_write("[VOICE] Slurred speech, partial words, and mumbling all understood\n");
    return true;
}

// ── Register command callback ─────────────────────────────────────────────────
void voice_set_callback(voice_cmd_callback_t cb) {
    g_callback = cb;
}

// ── Process a text command (from keyboard simulation or actual ASR) ───────────
// ALL input routes through the dysarthria engine first.
// This means slurred, truncated, or mumbled speech is corrected BEFORE
// keyword matching — so Landon never has to repeat himself.
voice_cmd_t voice_process_text(const char* text) {
    if (!text || !text[0]) return VCMD_NONE;

    // ── Step 1: Dysarthria adaptation ────────────────────────────────────────
    // Run the input through CORVUS's phoneme-aware fuzzy matcher.
    // This handles: slurred consonants, dropped finals, vowel distortion,
    // merged words, partial words, rapid-fire truncation.
    dysarthria_match_t match = dysarthria_match(text);

    if (match.confidence >= DYSARTHRIA_CONFIDENCE_MED && match.cmd_id < VCMD_UNKNOWN) {
        // Dysarthria engine found a confident match
        voice_cmd_t cmd = (voice_cmd_t)match.cmd_id;
        g_voice.last_cmd = cmd;
        g_voice.cmd_count++;

        if (match.was_corrected) {
            terminal_write("[DYSARTHRIA] Heard: \"");
            terminal_write(text);
            terminal_write("\" → Understood: \"");
            terminal_write(match.canonical);
            terminal_write("\" (confidence: ");
            char cbuf[8]; uint8_t c = match.confidence; int ci = 0;
            if (c == 0) { cbuf[ci++]='0'; }
            else { char tmp[8]; int ti=0; while(c){tmp[ti++]='0'+(c%10);c/=10;}
                   while(ti>0) cbuf[ci++]=tmp[--ti]; }
            cbuf[ci]=0; terminal_write(cbuf);
            terminal_write("%%)\n");
        } else {
            terminal_write("[VOICE] Command: ");
            terminal_write(match.canonical);
            terminal_write("\n");
        }

        // Find the response string for this command
        const char* response = "CORVUS: Command received";
        for (int i = 0; i < KEYWORD_COUNT; i++) {
            if ((voice_cmd_t)KEYWORDS[i].cmd == cmd) {
                response = KEYWORDS[i].response;
                break;
            }
        }
        terminal_write(response);
        terminal_write("\n");

        // Audio feedback
        sound_corvus_ack();
        if (cmd == VCMD_NITRO) sound_nitro_activate();

        // Constitutional override — STOP always works regardless of confidence
        if (cmd == VCMD_STOP) {
            terminal_write("[VOICE] EMERGENCY STOP — constitutional override\n");
        }

        // Fire callback
        if (g_callback) g_callback(cmd, response);

        // If confidence is medium (not high), log for learning
        if (match.confidence < DYSARTHRIA_CONFIDENCE_HIGH) {
            dysarthria_confirm(text, match.cmd_id);
        }

        return cmd;
    }

    // ── Step 2: Fallback — direct keyword scan ────────────────────────────────
    // If dysarthria engine isn't confident, try exact keyword matching.
    // This is the safety net for clear speech.
    for (int i = 0; i < KEYWORD_COUNT; i++) {
        if (vs_contains(text, KEYWORDS[i].keyword)) {
            voice_cmd_t cmd = KEYWORDS[i].cmd;
            g_voice.last_cmd = cmd;
            g_voice.cmd_count++;

            terminal_write("[VOICE] Command detected: ");
            terminal_write(KEYWORDS[i].keyword);
            terminal_write("\n");
            terminal_write(KEYWORDS[i].response);
            terminal_write("\n");

            sound_corvus_ack();
            if (cmd == VCMD_NITRO) sound_nitro_activate();
            if (g_callback) g_callback(cmd, KEYWORDS[i].response);
            if (cmd == VCMD_STOP) {
                terminal_write("[VOICE] EMERGENCY STOP — constitutional override\n");
            }
            // Teach the dysarthria engine this was correct
            dysarthria_confirm(text, (uint32_t)cmd);
            return cmd;
        }
    }

    // ── Step 3: Nothing matched ───────────────────────────────────────────────
    sound_corvus_error();
    terminal_write("[VOICE] Could not understand: \"");
    terminal_write(text);
    terminal_write("\" — routing to LLM\n");
    dysarthria_reject(text);
    return VCMD_UNKNOWN;
}

// ── Process raw PCM audio — VAD + keyword spotting ───────────────────────────
voice_cmd_t voice_process_pcm(const int16_t* samples, uint32_t count) {
    if (!g_voice.active || count == 0) return VCMD_NONE;

    // Energy-based VAD
    uint32_t energy = vad_energy(samples, count);
    g_voice.energy = energy;

    if (energy > VAD_THRESHOLD) {
        g_voice.speech_detected = true;
        g_voice.silence_ms = 0;
        terminal_write("[VOICE] Speech detected — energy=");
        // Print energy value
        char ebuf[12]; uint32_t e = energy; int ei = 0;
        if (e == 0) { ebuf[ei++]='0'; }
        else { char tmp[12]; int ti=0; while(e){tmp[ti++]='0'+(e%10);e/=10;}
               while(ti>0) ebuf[ei++]=tmp[--ti]; }
        ebuf[ei]=0; terminal_write(ebuf); terminal_write("\n");
    } else {
        if (g_voice.speech_detected) {
            g_voice.silence_ms += (count * 1000) / 16000;
            if (g_voice.silence_ms >= VAD_SILENCE_MS) {
                // End of utterance — process accumulated text
                g_voice.speech_detected = false;
                g_voice.silence_ms = 0;
                // In a real system, we'd run ASR here on the buffered PCM.
                // For v1.0, we use the LLM bridge (see llm.c) to interpret.
                terminal_write("[VOICE] Utterance complete — routing to CORVUS\n");
            }
        }
    }
    return VCMD_NONE;
}

// ── Tick — called every 10ms from PIT handler ─────────────────────────────────
void voice_tick(void) {
    if (!g_voice.active) return;

    // Pull samples from sound driver
    uint32_t n = sound_read_capture(g_pcm_buf, VOICE_PCM_BUF);
    if (n > 0) {
        voice_process_pcm(g_pcm_buf, n);
    }
}

// ── Start/stop listening ──────────────────────────────────────────────────────
void voice_start_listening(void) {
    if (!g_voice.active) return;
    g_voice.listening = true;
    sound_start_capture();
    sound_corvus_ready();
    terminal_write("[VOICE] CORVUS is listening...\n");
}

void voice_stop_listening(void) {
    g_voice.listening = false;
    sound_stop_capture();
    terminal_write("[VOICE] CORVUS stopped listening\n");
}

// ── Get state ─────────────────────────────────────────────────────────────────
voice_state_t* voice_get_state(void) {
    return &g_voice;
}

// ── Get last command name ─────────────────────────────────────────────────────
const char* voice_cmd_name(voice_cmd_t cmd) {
    for (int i = 0; i < KEYWORD_COUNT; i++) {
        if (KEYWORDS[i].cmd == cmd) return KEYWORDS[i].keyword;
    }
    return "NONE";
}
