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

#ifndef CORVUS_BRAIN_H
#define CORVUS_BRAIN_H

#include <stdint.h>
#include <stdbool.h>

// ── Counts ────────────────────────────────────────────────────────────────────
#define CORVUS_AGENT_COUNT    10
#define CORVUS_NEED_COUNT      5
#define CORVUS_ACTION_COUNT   10
#define CORVUS_GOAL_COUNT      7
#define CORVUS_TOOL_COUNT     10
#define CORVUS_MEMORY_SLOTS   32
#define CORVUS_FACT_LEN       64
#define CORVUS_PLAN_MAX       16

// ── Agent IDs ─────────────────────────────────────────────────────────────────
typedef enum {
    AGENT_CROW = 0, AGENT_HEALER, AGENT_SECURITY, AGENT_MEMORY,
    AGENT_SCHEDULER, AGENT_NETWORK, AGENT_FILESYSTEM,
    AGENT_DISPLAY, AGENT_INPUT, AGENT_ENTROPY
} corvus_agent_id_t;

// ── Need IDs (Maslow hierarchy) ───────────────────────────────────────────────
typedef enum {
    NEED_SURVIVAL = 0, NEED_SAFETY, NEED_SOCIAL, NEED_ESTEEM, NEED_GROWTH
} corvus_need_id_t;

// ── Action IDs (GOAP) ─────────────────────────────────────────────────────────
typedef enum {
    ACTION_MONITOR = 0, ACTION_HEAL, ACTION_SECURE, ACTION_REMEMBER,
    ACTION_RECALL, ACTION_SCHEDULE, ACTION_REPORT, ACTION_LEARN,
    ACTION_PLAN, ACTION_REST
} corvus_action_id_t;

// ── Goal IDs ──────────────────────────────────────────────────────────────────
typedef enum {
    GOAL_NONE = 0, GOAL_MAINTAIN_HEALTH, GOAL_ENSURE_SAFETY,
    GOAL_LEARN, GOAL_COOPERATE, GOAL_RENDER, GOAL_REST,
    GOAL_COUNT
} corvus_goal_id_t;

// ── Agent states ──────────────────────────────────────────────────────────────
typedef enum {
    AGENT_IDLE = 0, AGENT_PLANNING, AGENT_EXECUTING, AGENT_RESTING, AGENT_DORMANT
} corvus_agent_state_t;

// ── Tool categories ───────────────────────────────────────────────────────────
typedef enum {
    TOOL_KERNEL = 0, TOOL_DISPLAY, TOOL_INPUT, TOOL_MEMORY, TOOL_GOVERN
} corvus_tool_category_t;

// ── Memory slot ───────────────────────────────────────────────────────────────
typedef struct {
    char     fact[CORVUS_FACT_LEN];
    float    relevance;
    uint32_t tick_stored;
} corvus_memory_t;

// ── Tool definition ───────────────────────────────────────────────────────────
typedef struct {
    const char*            name;
    const char*            description;
    corvus_tool_category_t category;
} corvus_tool_t;

// ── Tool call result ──────────────────────────────────────────────────────────
typedef struct {
    bool success;
    char output[64];
} corvus_tool_result_t;

// ── Agent ─────────────────────────────────────────────────────────────────────
typedef struct {
    corvus_agent_id_t    id;
    bool                 active;
    corvus_agent_state_t state;
    corvus_goal_id_t     current_goal;

    // Needs (0.0 = empty/distressed, 1.0 = fully satisfied)
    float                needs[CORVUS_NEED_COUNT];
    bool                 distress;

    // GOAP plan
    corvus_action_id_t   plan[CORVUS_PLAN_MAX];
    int                  plan_length;
    int                  plan_cursor;

    // Vector memory (simplified in-kernel store)
    corvus_memory_t      memory[CORVUS_MEMORY_SLOTS];
    int                  memory_count;

    uint32_t             tick_count;
} corvus_agent_t;

// ── CORVUS global state ───────────────────────────────────────────────────────
typedef struct {
    corvus_agent_t agents[CORVUS_AGENT_COUNT];
    uint32_t       tick;
    uint32_t       cognitive_hz;
    bool           ready;
} corvus_state_t;

// ── API ───────────────────────────────────────────────────────────────────────
void                 corvus_brain_init(void);
void                 corvus_brain_tick(void);
corvus_tool_result_t corvus_call_tool(const char* tool_name, const char* args_json);
corvus_agent_t*      corvus_get_agent(int agent_id);
const char*          corvus_get_agent_name(int agent_id);
const char*          corvus_get_need_name(int need_id);
const char*          corvus_get_action_name(int action_id);
uint32_t             corvus_get_tick(void);
bool                 corvus_is_ready(void);

#endif // CORVUS_BRAIN_H
