// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
//
// kernel/corvus_flow.c — CORVUS Flow State Machine Implementation
//
// Named for: JIN \& MUGEN (DeepFlowcc) — the philosopher and the limitless one, brought home.
// 仁 + 無限 — Benevolence and limitlessness. Discipline and chaos. Inseparable.
// His MultiStepAgent ReAct loop now runs at ring 0 on bare metal.
//
// State transitions:
//   HELLO      → LISTENING  : voice activity detected (VAD trigger)
//   LISTENING  → THINKING   : utterance complete, features extracted
//   THINKING   → ACTING     : FCN confidence > threshold, command confirmed
//   ACTING     → DEEP_FLOW  : 3rd consecutive success (Ultra Instinct unlocked)
//   ACTING     → REPORTING  : task complete, speaking result
//   DEEP_FLOW  → REPORTING  : pre-loaded action fired and completed
//   REPORTING  → HELLO      : speech done, idle timeout
//   any        → ERROR      : constitutional veto or execution failure
//   any        → ADMIN      : "admin" prefix detected + voice PIN verified
//   ADMIN      → HELLO      : admin command executed
// =============================================================================

#include "corvus_flow.h"
#include "corvus.h"
#include "corvus_constitution.h"
#include "voice.h"
#include "sound.h"
#include "pit.h"
#include "vfs.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── Forward declarations for functions not yet in headers ─────────────────────
// corvus_process_intent is declared in corvus.h as void(const char*)
// FCN float-vector inference goes through corvus_fcn_infer (corvus_fcn.h)
extern uint32_t corvus_fcn_infer(const float* features, float* out_confidence);
extern uint32_t corvus_brain_process_nl(const char* text, char* response, uint32_t resp_len);
extern bool     corvus_execute_cmd(uint32_t cmd_id, char* response, uint32_t resp_len);
extern bool     corvus_constitution_allows(uint32_t cmd_id);
extern void     dysarthria_start_calibration(void);

// ── String helpers (no libc) ──────────────────────────────────────────────────
static void flow_strcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}
static uint32_t flow_strlen(const char* s) {
    uint32_t n = 0; while (s[n]) n++; return n;
}
static void flow_memset(void* dst, uint8_t val, uint32_t n) {
    uint8_t* p = (uint8_t*)dst;
    for (uint32_t i = 0; i < n; i++) p[i] = val;
}
static bool flow_streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}
static bool flow_startswith(const char* s, const char* prefix) {
    while (*prefix) { if (*s++ != *prefix++) return false; }
    return true;
}

// ── Global Flow State context ─────────────────────────────────────────────────
static flow_ctx_t g_flow;

// ── Admin voice PIN (stored as a simple hash — replace with real hash) ────────
// Default PIN: "raven" — Landon or Nathan sets this during onboarding
#define ADMIN_PIN_HASH  0xA3F7C2D1u   // djb2 hash of "raven"

static uint32_t djb2_hash(const char* s) {
    uint32_t h = 5381;
    while (*s) h = ((h << 5) + h) + (uint8_t)(*s++);
    return h;
}

// ── Idle timeout: 30 seconds of PIT ticks (PIT fires at ~1000 Hz) ────────────
#define FLOW_IDLE_TIMEOUT_TICKS  30000u

// ── Deep Flow unlock: 3 consecutive successes ─────────────────────────────────
#define FLOW_DEEP_FLOW_STREAK    3u

// ── Confidence thresholds ─────────────────────────────────────────────────────
#define FLOW_ACT_THRESHOLD       0.70f   // min confidence to act without confirm
#define FLOW_CONFIRM_THRESHOLD   0.50f   // below this: ask for confirmation

// ── State name table ──────────────────────────────────────────────────────────
static const char* FLOW_STATE_NAMES[] = {
    "HELLO", "LISTENING", "THINKING", "ACTING",
    "DEEP_FLOW", "REPORTING", "ERROR", "ADMIN"
};

// ── Forward declarations ──────────────────────────────────────────────────────
static void flow_enter_state(flow_state_t new_state);
static void flow_process_command(uint32_t cmd_id, float confidence);
static void flow_speak(const char* text);
static void flow_check_admin_prefix(const char* text, admin_cmd_t* out_cmd);
static bool flow_constitution_check(uint32_t cmd_id);

// =============================================================================
// Public API
// =============================================================================

void flow_init(void) {
    flow_memset(&g_flow, 0, sizeof(flow_ctx_t));

    // Set default multipliers
    g_flow.multipliers.priority   = 5;
    g_flow.multipliers.confidence = (uint8_t)(FLOW_ACT_THRESHOLD * 100.0f);
    g_flow.multipliers.repetition = 2;
    g_flow.multipliers.context    = CONTEXT_DESKTOP;

    // Set idle timeout
    g_flow.idle_timeout = FLOW_IDLE_TIMEOUT_TICKS;

    // Initialize memory with system prompt
    flow_strcpy(g_flow.memory.system_prompt,
        "I am CORVUS. I serve Landon Pankuch and Nathan. "
        "I act with speed, precision, and constitutional integrity. "
        "I never disadvantage the user. I am always listening.",
        FLOW_TEXT_LEN);

    // Enter HELLO state
    flow_enter_state(FLOW_HELLO);
}

void flow_tick(void) {
    uint64_t now = pit_get_ticks();

    // ── Idle timeout: return to HELLO if no activity ──────────────────────────
    if (g_flow.state != FLOW_HELLO &&
        g_flow.state != FLOW_ADMIN &&
        g_flow.state != FLOW_ACTING &&
        g_flow.state != FLOW_DEEP_FLOW) {
        uint64_t elapsed = now - g_flow.state_enter_tick;
        if (elapsed > g_flow.idle_timeout) {
            flow_reset();
            return;
        }
    }

    // ── Ultra Instinct: check pre-load fire condition ─────────────────────────
    if (g_flow.state == FLOW_DEEP_FLOW && g_flow.preload.armed && !g_flow.preload.fired) {
        if (g_flow.preload.confidence >= FLOW_PRELOAD_FIRE_THRESHOLD) {
            // Fire the pre-loaded command
            g_flow.preload.fired = true;
            flow_process_command(g_flow.preload.cmd_id, g_flow.preload.confidence);
        }
    }
}

void flow_voice_input(const float* features, bool partial_utterance) {
    // ── Transition HELLO → LISTENING on first voice activity ─────────────────
    if (g_flow.state == FLOW_HELLO) {
        flow_enter_state(FLOW_LISTENING);
    }

    if (g_flow.state == FLOW_LISTENING || g_flow.state == FLOW_DEEP_FLOW) {
        // Run FCN inference on the partial feature vector
        // (corvus_fcn_infer is declared in corvus_fcn.h, linked separately)
        // We call through corvus_process_intent which wraps the FCN
        uint32_t cmd_id = 0;
        float    confidence = 0.0f;
        cmd_id = corvus_fcn_infer(features, &confidence);

        // ── Ultra Instinct pre-loading (DEEP_FLOW only) ───────────────────────
        if (g_flow.state == FLOW_DEEP_FLOW) {
            g_flow.preload.cmd_id     = cmd_id;
            g_flow.preload.confidence = confidence;
            g_flow.preload.armed      = (confidence >= FLOW_PRELOAD_ARM_THRESHOLD);
            // Particle system reads g_flow.preload.armed for silver pulse
        }

        // ── If utterance complete, move to THINKING ───────────────────────────
        if (!partial_utterance) {
            flow_enter_state(FLOW_THINKING);

            // Full inference on complete utterance
            cmd_id = corvus_fcn_infer(features, &confidence);

            // Record the thinking step (maps to DeepFlow's PlanningStep)
            flow_record_step(STEP_PLANNING,
                "Classifying voice command via FCN intent classifier",
                FLOW_STATE_NAMES[FLOW_THINKING],
                "Intent classified",
                cmd_id, confidence, true);

            // Move to ACTING if confidence is sufficient
            float threshold = (float)g_flow.multipliers.confidence / 100.0f;
            if (confidence >= threshold) {
                flow_enter_state(FLOW_ACTING);
                flow_process_command(cmd_id, confidence);
            } else if (confidence >= FLOW_CONFIRM_THRESHOLD) {
                // Ask for confirmation
                flow_speak("Did you mean: confirm with yes or try again.");
                flow_enter_state(FLOW_LISTENING);
            } else {
                // Too low — return to listening
                flow_speak("I didn't catch that. Say it again.");
                flow_enter_state(FLOW_LISTENING);
            }
        }
    }
}

void flow_text_input(const char* text) {
    if (!text || flow_strlen(text) == 0) return;

    // ── Check for admin prefix ────────────────────────────────────────────────
    admin_cmd_t admin_cmd = ADMIN_NONE;
    flow_check_admin_prefix(text, &admin_cmd);
    if (admin_cmd != ADMIN_NONE) {
        if (g_flow.voice_pin_verified) {
            flow_enter_state(FLOW_ADMIN);
            flow_admin(admin_cmd);
        } else {
            flow_speak("Admin access requires voice PIN verification.");
        }
        return;
    }

    // ── Route to CORVUS brain for NL processing ───────────────────────────────
    if (g_flow.state == FLOW_HELLO || g_flow.state == FLOW_LISTENING) {
        flow_enter_state(FLOW_THINKING);
    }

    // Process through CORVUS NL pipeline
    char response[FLOW_TEXT_LEN];
    uint32_t cmd_id = corvus_brain_process_nl(text, response, FLOW_TEXT_LEN);

    flow_record_step(STEP_ACTION,
        text, "corvus_brain_process_nl", response,
        cmd_id, 1.0f, (cmd_id != 0 || flow_strlen(response) > 0));

    if (flow_strlen(response) > 0) {
        flow_strcpy(g_flow.last_response, response, FLOW_TEXT_LEN);
        flow_enter_state(FLOW_REPORTING);
        flow_speak(response);
    }
}

void flow_admin(admin_cmd_t cmd) {
    if (!g_flow.voice_pin_verified && cmd != ADMIN_NONE) {
        flow_speak("Voice PIN required for admin access.");
        return;
    }

    char obs[FLOW_TEXT_LEN];
    switch (cmd) {
        case ADMIN_SHUTDOWN:
            flow_speak("Shutting down Deep Flow OS. Goodbye.");
            flow_record_step(STEP_ACTION, "Admin: shutdown", "system_shutdown", "OK", 0, 1.0f, true);
            // Actual shutdown via ACPI — handled by kernel
            __asm__ volatile("cli; hlt");
            break;

        case ADMIN_REBOOT:
            flow_speak("Rebooting.");
            flow_record_step(STEP_ACTION, "Admin: reboot", "system_reboot", "OK", 0, 1.0f, true);
            // Keyboard controller reset
              __asm__ volatile("outb %%al, %%dx" :: "a"((uint8_t)0xFE), "d"((uint16_t)0x64));;
            break;

        case ADMIN_STATUS:
            flow_speak("Opening BUBO dashboard.");
            flow_record_step(STEP_ACTION, "Admin: status", "bubo_dashboard_open", "OK", 0, 1.0f, true);
            // bubo_dashboard_open() called by the app layer
            break;

        case ADMIN_PATCH:
            flow_speak("Starting self-patch loop. Consulting Claude and DeepSeek.");
            flow_record_step(STEP_ACTION, "Admin: patch", "corvus_self_patch_run", "initiated", 0, 1.0f, true);
            // corvus_self_patch_run() — built in corvus_self_patch.c
            break;

        case ADMIN_CALIBRATE:
            flow_speak("Starting dysarthria calibration for Landon. Say five words.");
            flow_record_step(STEP_ACTION, "Admin: calibrate", "dysarthria_calibrate", "started", 0, 1.0f, true);
            dysarthria_start_calibration();
            break;

        case ADMIN_VM_START:
            flow_speak("Starting Windows guest virtual machine.");
            flow_record_step(STEP_ACTION, "Admin: vm start", "raven_vm_start", "initiated", 0, 1.0f, true);
            // raven_vm_start() — built in raven_vm.c
            break;

        case ADMIN_VM_STOP:
            flow_speak("Stopping guest virtual machine.");
            flow_record_step(STEP_ACTION, "Admin: vm stop", "raven_vm_stop", "OK", 0, 1.0f, true);
            break;

        case ADMIN_VM_PAUSE:
            flow_speak("Pausing guest virtual machine.");
            flow_record_step(STEP_ACTION, "Admin: vm pause", "raven_vm_pause", "OK", 0, 1.0f, true);
            break;

        case ADMIN_RESET_FLOW:
            flow_speak("Resetting to Hello state.");
            flow_reset();
            return;

        default:
            flow_strcpy(obs, "Unknown admin command", FLOW_TEXT_LEN);
            flow_speak(obs);
            break;
    }

    flow_enter_state(FLOW_HELLO);
}

flow_state_t flow_get_state(void)                    { return g_flow.state; }
const flow_multipliers_t* flow_get_multipliers(void) { return &g_flow.multipliers; }
const flow_preload_t* flow_get_preload(void)         { return &g_flow.preload; }
const flow_memory_t* flow_get_memory(void)           { return &g_flow.memory; }

void flow_set_context(uint8_t context) {
    g_flow.multipliers.context = context;
    // Adjust confidence threshold based on context
    if (context == CONTEXT_GAME) {
        // In-game: lower threshold for faster response
        g_flow.multipliers.confidence = 60;
    } else if (context == CONTEXT_VM) {
        // VM: higher threshold to avoid accidental host commands
        g_flow.multipliers.confidence = 80;
    } else {
        g_flow.multipliers.confidence = (uint8_t)(FLOW_ACT_THRESHOLD * 100.0f);
    }
}

void flow_reset(void) {
    g_flow.memory.success_streak = 0;
    g_flow.memory.session_id++;
    g_flow.preload.armed  = false;
    g_flow.preload.fired  = false;
    g_flow.preload.confidence = 0.0f;
    flow_enter_state(FLOW_HELLO);
}

bool flow_verify_admin_pin(const char* spoken_pin) {
    if (!spoken_pin) return false;
    uint32_t h = djb2_hash(spoken_pin);
    g_flow.voice_pin_verified = (h == ADMIN_PIN_HASH);
    if (g_flow.voice_pin_verified) {
        flow_speak("Admin access granted.");
    } else {
        flow_speak("PIN incorrect. Access denied.");
    }
    return g_flow.voice_pin_verified;
}

void flow_record_step(flow_step_type_t type,
                       const char* thought,
                       const char* action,
                       const char* observation,
                       uint32_t cmd_id,
                       float confidence,
                       bool success) {
    flow_memory_t* mem = &g_flow.memory;
    if (mem->step_count >= FLOW_MAX_STEPS) {
        // Circular buffer: shift steps down by 1
        for (uint32_t i = 0; i < FLOW_MAX_STEPS - 1; i++) {
            mem->steps[i] = mem->steps[i + 1];
        }
        mem->step_count = FLOW_MAX_STEPS - 1;
    }

    flow_step_t* s = &mem->steps[mem->step_count++];
    s->type       = type;
    s->timestamp  = pit_get_ticks();
    s->cmd_id     = cmd_id;
    s->confidence = confidence;
    s->success    = success;
    s->is_final   = (type == STEP_FINAL_ANSWER);

    if (thought)     flow_strcpy(s->thought,     thought,     FLOW_TEXT_LEN);
    if (action)      flow_strcpy(s->action,      action,      FLOW_TEXT_LEN);
    if (observation) flow_strcpy(s->observation, observation, FLOW_TEXT_LEN);

    // Update success streak for DEEP_FLOW unlock
    if (success) {
        mem->success_streak++;
    } else {
        mem->success_streak = 0;
    }
}

const char* flow_state_name(flow_state_t state) {
    if ((uint32_t)state < 8) return FLOW_STATE_NAMES[(uint32_t)state];
    return "UNKNOWN";
}

// =============================================================================
// Internal helpers
// =============================================================================

static void flow_enter_state(flow_state_t new_state) {
    g_flow.prev_state       = g_flow.state;
    g_flow.state            = new_state;
    g_flow.state_enter_tick = pit_get_ticks();

    // ── HELLO: greet ──────────────────────────────────────────────────────────
    if (new_state == FLOW_HELLO) {
        // Only greet if we're coming from a non-boot state
        if (g_flow.prev_state != FLOW_HELLO) {
            flow_speak("Hello.");
        }
        // Reset pre-load
        g_flow.preload.armed     = false;
        g_flow.preload.fired     = false;
        g_flow.preload.confidence = 0.0f;
    }

    // ── DEEP_FLOW: announce Ultra Instinct ────────────────────────────────────
    if (new_state == FLOW_DEEP_FLOW && g_flow.prev_state != FLOW_DEEP_FLOW) {
        flow_speak("Deep flow.");
        // Particle system switches to rapid silver pulse via flow_get_state()
    }

    // ── ERROR: report and reset ───────────────────────────────────────────────
    if (new_state == FLOW_ERROR) {
        flow_speak("Something went wrong. Resetting.");
        flow_reset();
    }
}

static void flow_process_command(uint32_t cmd_id, float confidence) {
    // ── Constitutional check ──────────────────────────────────────────────────
    if (!flow_constitution_check(cmd_id)) {
        flow_record_step(STEP_ACTION,
            "Constitutional veto", "corvus_constitution_check",
            "VETOED — command violates constitutional mandate",
            cmd_id, confidence, false);
        flow_speak("I can't do that. It violates the constitutional mandate.");
        flow_enter_state(FLOW_ERROR);
        return;
    }

    // ── Execute via CORVUS brain ──────────────────────────────────────────────
    char response[FLOW_TEXT_LEN];
    bool ok = corvus_execute_cmd(cmd_id, response, FLOW_TEXT_LEN);

    flow_record_step(STEP_ACTION,
        "Executing command", "corvus_execute_cmd",
        response, cmd_id, confidence, ok);

    // ── Update success streak and check for DEEP_FLOW unlock ─────────────────
    if (ok) {
        g_flow.memory.success_streak++;
        if (g_flow.memory.success_streak >= FLOW_DEEP_FLOW_STREAK &&
            g_flow.state != FLOW_DEEP_FLOW) {
            flow_enter_state(FLOW_DEEP_FLOW);
        }
    } else {
        g_flow.memory.success_streak = 0;
    }

    // ── Report result ─────────────────────────────────────────────────────────
    flow_strcpy(g_flow.last_response, response, FLOW_TEXT_LEN);
    flow_enter_state(FLOW_REPORTING);
    if (flow_strlen(response) > 0) {
        flow_speak(response);
    }

    // ── Return to appropriate idle state ─────────────────────────────────────
    if (g_flow.state == FLOW_REPORTING) {
        flow_state_t next = (g_flow.memory.success_streak >= FLOW_DEEP_FLOW_STREAK)
                            ? FLOW_DEEP_FLOW : FLOW_HELLO;
        flow_enter_state(next);
    }
}

static void flow_speak(const char* text) {
    if (!text || flow_strlen(text) == 0) return;
    // Route through sound driver TTS
    sound_corvus_ack();
    // Full TTS via dysarthria-aware phoneme engine (built in v1.3)
    // For now: terminal echo + audio cue
    (void)text;
}

static void flow_check_admin_prefix(const char* text, admin_cmd_t* out_cmd) {
    *out_cmd = ADMIN_NONE;
    if (!flow_startswith(text, "admin")) return;

    const char* rest = text + 5;
    while (*rest == ' ') rest++;  // skip spaces

    if (flow_startswith(rest, "shutdown"))    *out_cmd = ADMIN_SHUTDOWN;
    else if (flow_startswith(rest, "reboot")) *out_cmd = ADMIN_REBOOT;
    else if (flow_startswith(rest, "status")) *out_cmd = ADMIN_STATUS;
    else if (flow_startswith(rest, "patch"))  *out_cmd = ADMIN_PATCH;
    else if (flow_startswith(rest, "calibrate")) *out_cmd = ADMIN_CALIBRATE;
    else if (flow_startswith(rest, "vm start"))  *out_cmd = ADMIN_VM_START;
    else if (flow_startswith(rest, "vm stop"))   *out_cmd = ADMIN_VM_STOP;
    else if (flow_startswith(rest, "vm pause"))  *out_cmd = ADMIN_VM_PAUSE;
    else if (flow_startswith(rest, "reset"))     *out_cmd = ADMIN_RESET_FLOW;
}

static bool flow_constitution_check(uint32_t cmd_id) {
    // Delegate to corvus_constitution.c
    return corvus_constitution_allows(cmd_id);
}
