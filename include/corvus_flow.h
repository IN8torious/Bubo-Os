// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
//
// include/corvus_flow.h — CORVUS Flow State Machine
//
// The agent at the heart of Deep Flow OS is named CORVUS —
// in honor of the DeepFlow agent (DeepFlowcc/DeepFlow) whose
// MultiStepAgent ReAct architecture this is ported from.
// He built the loop. We brought it home to bare metal.
//
// DeepFlow's loop:  Thought → Code → Observation → repeat
// CORVUS's loop:    HELLO → LISTENING → THINKING → ACTING → DEEP_FLOW → REPORTING
//
// The Flow State machine is the heart of Deep Flow OS. Every voice command,
// every task, every self-patch runs through this loop. CORVUS enters
// DEEP_FLOW (Ultra Instinct) after 3+ consecutive successful steps —
// at that point it pre-loads the most probable next action before
// Landon finishes speaking.
//
// Named for: DeepFlowcc — the local badass who got brought home.
//
// Architecture ported from:
//   DeepFlowcc/DeepFlow — core/agent_core.py, core/memory.py (Corvus)
//   Haskell_ML          — FCN intent classifier
//   claude-haskell       — Claude Sonnet API client
// =============================================================================
#ifndef DEEPFLOW_CORVUS_FLOW_H
#define DEEPFLOW_CORVUS_FLOW_H

#include <stdint.h>
#include <stdbool.h>

// ── Flow State (maps to DeepFlow's AgentState) ───────────────────────────────
typedef enum {
    FLOW_HELLO      = 0,   // Idle — resting nervous system, soft greeting
    FLOW_LISTENING  = 1,   // Voice activity detected — converging particles
    FLOW_THINKING   = 2,   // Intent received — FCN classifying, orbit particles
    FLOW_ACTING     = 3,   // Command confirmed — executing, tight vortex
    FLOW_DEEP_FLOW  = 4,   // Ultra Instinct — 3+ successes, pre-loading next cmd
    FLOW_REPORTING  = 5,   // Task complete — burst particles, speaking result
    FLOW_ERROR      = 6,   // Constitutional veto or execution failure
    FLOW_ADMIN      = 7,   // Admin command — requires voice PIN
} flow_state_t;

// ── Step types (maps to DeepFlow's memory step classes) ──────────────────────
typedef enum {
    STEP_SYSTEM_PROMPT = 0,  // Agent identity and constitution
    STEP_TASK          = 1,  // User request (voice or typed)
    STEP_PLANNING      = 2,  // Facts + high-level plan (PlanningStep)
    STEP_ACTION        = 3,  // Tool call + observation (ActionStep)
    STEP_FINAL_ANSWER  = 4,  // Task complete, result ready
} flow_step_type_t;

// ── Multipliers (modify how CORVUS responds) ─────────────────────────────────
typedef struct {
    uint8_t  priority;      // 1–10: higher = skip queue, execute first
    uint8_t  confidence;    // min confidence threshold to act without confirm
    uint8_t  repetition;    // times cmd repeated in window → auto-execute
    uint8_t  context;       // CONTEXT_DESKTOP / CONTEXT_GAME / CONTEXT_VM
} flow_multipliers_t;

#define CONTEXT_DESKTOP  0
#define CONTEXT_GAME     1
#define CONTEXT_VM       2
#define CONTEXT_ADMIN    3

// ── Memory step (maps to DeepFlow's ActionStep / PlanningStep) ────────────────
#define FLOW_TEXT_LEN    256
#define FLOW_MAX_STEPS   32

typedef struct {
    flow_step_type_t type;
    uint64_t         timestamp;          // PIT tick when step was created
    char             thought[FLOW_TEXT_LEN];   // agent's reasoning (Thought:)
    char             action[FLOW_TEXT_LEN];    // tool call or command issued
    char             observation[FLOW_TEXT_LEN]; // result / error observed
    uint32_t         cmd_id;             // CORVUS command ID (0 = none)
    float            confidence;         // FCN confidence at time of action
    bool             success;            // did the action succeed?
    bool             is_final;           // is this the final answer step?
} flow_step_t;

// ── Agent memory (maps to DeepFlow's AgentMemory) ────────────────────────────
typedef struct {
    flow_step_t  steps[FLOW_MAX_STEPS];
    uint32_t     step_count;
    uint32_t     success_streak;    // consecutive successes → DEEP_FLOW trigger
    uint32_t     session_id;        // increments each time we return to HELLO
    char         system_prompt[FLOW_TEXT_LEN]; // agent identity/constitution
} flow_memory_t;

// ── Pre-load queue (Ultra Instinct — DEEP_FLOW only) ─────────────────────────
// When in DEEP_FLOW, the top-1 predicted next command is pre-loaded here.
// If confidence crosses FLOW_PRELOAD_FIRE_THRESHOLD before speech ends,
// the action fires immediately.
#define FLOW_PRELOAD_FIRE_THRESHOLD  0.95f
#define FLOW_PRELOAD_ARM_THRESHOLD   0.85f

typedef struct {
    uint32_t  cmd_id;          // pre-loaded command ID
    float     confidence;      // current confidence (updated per phoneme chunk)
    bool      armed;           // confidence > ARM_THRESHOLD
    bool      fired;           // action already dispatched
    uint64_t  arm_tick;        // PIT tick when armed
} flow_preload_t;

// ── Admin commands ────────────────────────────────────────────────────────────
typedef enum {
    ADMIN_NONE       = 0,
    ADMIN_SHUTDOWN   = 1,
    ADMIN_REBOOT     = 2,
    ADMIN_STATUS     = 3,   // show full BUBO dashboard
    ADMIN_PATCH      = 4,   // trigger self-patch loop
    ADMIN_CALIBRATE  = 5,   // dysarthria recalibration for Landon
    ADMIN_VM_START   = 6,   // start Windows/Linux guest VM
    ADMIN_VM_STOP    = 7,   // stop guest VM
    ADMIN_VM_PAUSE   = 8,   // pause guest VM
    ADMIN_RESET_FLOW = 9,   // reset flow state to HELLO
} admin_cmd_t;

// ── Full Flow State context ───────────────────────────────────────────────────
typedef struct {
    flow_state_t      state;            // current state
    flow_state_t      prev_state;       // previous state (for transitions)
    flow_memory_t     memory;           // episodic step memory
    flow_multipliers_t multipliers;     // active multipliers
    flow_preload_t    preload;          // Ultra Instinct pre-load queue
    uint64_t          state_enter_tick; // PIT tick when we entered current state
    uint32_t          idle_timeout;     // ticks before returning to HELLO
    bool              voice_pin_verified; // true after admin voice PIN check
    char              last_response[FLOW_TEXT_LEN]; // last spoken response
} flow_ctx_t;

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize the Flow State machine — call once at boot after CORVUS init
void         flow_init(void);

// Tick — call every frame from the main kernel loop
// Handles timeouts, state transitions, and particle state updates
void         flow_tick(void);

// Voice input received — raw phoneme feature vector from dysarthria engine
// features: float[FCN_INPUT_DIM], partial_utterance: true if still speaking
void         flow_voice_input(const float* features, bool partial_utterance);

// Text/typed input received (from terminal or syscall)
void         flow_text_input(const char* text);

// Execute an admin command (requires voice_pin_verified = true)
void         flow_admin(admin_cmd_t cmd);

// Get current state (for particle system and health bar)
flow_state_t flow_get_state(void);

// Get current multipliers
const flow_multipliers_t* flow_get_multipliers(void);

// Get the pre-load queue (for Ultra Instinct particle pulse)
const flow_preload_t* flow_get_preload(void);

// Get the full memory (for BUBO dashboard)
const flow_memory_t* flow_get_memory(void);

// Set context multiplier (DESKTOP / GAME / VM / ADMIN)
void         flow_set_context(uint8_t context);

// Reset to HELLO state (called after task complete or on timeout)
void         flow_reset(void);

// Verify admin voice PIN — returns true if PIN matches stored hash
bool         flow_verify_admin_pin(const char* spoken_pin);

// Add a completed step to memory (called by CORVUS after each action)
void         flow_record_step(flow_step_type_t type,
                               const char* thought,
                               const char* action,
                               const char* observation,
                               uint32_t cmd_id,
                               float confidence,
                               bool success);

// Get human-readable state name (for debug and health bar tooltip)
const char*  flow_state_name(flow_state_t state);

#endif // DEEPFLOW_CORVUS_FLOW_H
