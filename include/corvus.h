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

#ifndef RAVEN_CORVUS_H
#define RAVEN_CORVUS_H

// =============================================================================
// CORVUS — Chief Orchestration & Reasoning Via Unified Systems
// Layer 5 of the Instinct OS 7-Layer Architecture
//
// CORVUS is the embedded reasoning engine of Raven OS.
// It coordinates all kernel agents, interprets user intent,
// enforces the constitutional governance layer, and drives
// autonomous system optimization.
//
// "The chef who runs the kitchen."
// =============================================================================

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ── Agent IDs ─────────────────────────────────────────────────────────────────
#define AGENT_CROW          0   // Watchdog & self-heal
#define AGENT_HEALER        1   // Auto-repair corrupted state
#define AGENT_SECURITY      2   // Threat detection & syscall monitoring
#define AGENT_PRIVACY       3   // Tracker & telemetry blocker
#define AGENT_PERFORMANCE   4   // Memory & CPU optimizer
#define AGENT_SESSION       5   // Session & auth manager
#define AGENT_FILE          6   // File integrity monitor
#define AGENT_NOTIF         7   // Smart notification filter
#define AGENT_DIAGNOSTICS   8   // System health diagnostics
#define AGENT_ACCESSIBILITY 9   // A11y & input assistance
#define CORVUS_AGENT_COUNT  10

// ── Agent States (BDI model) ──────────────────────────────────────────────────
typedef enum {
    AGENT_IDLE      = 0,    // No active goal
    AGENT_SENSING   = 1,    // Reading telemetry
    AGENT_PLANNING  = 2,    // Formulating response
    AGENT_ACTING    = 3,    // Executing tool calls
    AGENT_REPORTING = 4,    // Sending result to CORVUS
    AGENT_SUSPENDED = 5,    // Paused by governance layer
} agent_state_t;

// ── Agent Belief-Desire-Intention (BDI) struct ────────────────────────────────
typedef struct {
    uint8_t       id;
    const char*   name;
    agent_state_t state;

    // Beliefs: what the agent currently knows
    uint32_t      last_event_tick;
    uint32_t      events_processed;
    uint32_t      anomalies_detected;

    // Desires: what the agent wants to achieve
    uint8_t       priority;         // 0-255, higher = more urgent
    bool          goal_active;
    char          goal[64];

    // Intentions: current action plan
    uint8_t       action_queue[8];  // Tool IDs queued for execution
    uint8_t       action_count;

    // Performance tracking (feeds vector memory)
    uint32_t      success_count;
    uint32_t      failure_count;
    // Health score 0-100: derived from success_count vs failure_count
    uint8_t       health;
} corvus_agent_t;

// ── Tool Abstraction Layer (MCP-style) ────────────────────────────────────────
#define TOOL_PMM_ALLOC      0
#define TOOL_PMM_FREE       1
#define TOOL_VMM_MAP        2
#define TOOL_VMM_UNMAP      3
#define TOOL_SCHED_BOOST    4
#define TOOL_SCHED_DEMOTE   5
#define TOOL_LOG_WRITE      6
#define TOOL_SECURITY_ALERT 7
#define TOOL_HEALER_PATCH   8
#define TOOL_SHELL_RESPOND  9
#define CORVUS_TOOL_COUNT   10

typedef struct {
    uint8_t     tool_id;
    const char* name;
    const char* description;
    uint32_t    arg0;
    uint32_t    arg1;
    uint32_t    result;
    bool        blocked_by_governance;
} corvus_tool_call_t;

// ── Constitutional Governance Rules ──────────────────────────────────────────
// Hard-coded C++ guards — AI cannot override these
typedef enum {
    GOV_ALLOW   = 0,
    GOV_DENY    = 1,
    GOV_AUDIT   = 2,   // Allow but log
} gov_verdict_t;

// ── Vector Memory Entry (simplified RAG store) ────────────────────────────────
#define VECTOR_MEM_SIZE 64

typedef struct {
    uint32_t tick;
    uint8_t  agent_id;
    uint8_t  tool_id;
    uint8_t  outcome;   // 0=success, 1=failure, 2=blocked
    char     context[48];
} vector_entry_t;

// ── CORVUS Core State ─────────────────────────────────────────────────────────
typedef struct {
    bool              initialized;
    uint32_t          tick;
    corvus_agent_t    agents[CORVUS_AGENT_COUNT];
    vector_entry_t    vector_mem[VECTOR_MEM_SIZE];
    uint32_t          vector_head;
    uint32_t          total_decisions;
    uint32_t          blocked_by_governance;
    char              last_user_intent[128];
    char              last_response[128];
} corvus_state_t;

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize CORVUS and all 10 agents
void corvus_init(void);

// Called every PIT tick — runs the cognitive loop
void corvus_tick(void);

// Submit a tool call request — governance checks before execution
bool corvus_invoke_tool(uint8_t agent_id, corvus_tool_call_t* call);

// Submit natural language intent from the shell
void corvus_process_intent(const char* intent);

// Get CORVUS status string for display
const char* corvus_status(void);

// Governance check — returns verdict for a proposed action
gov_verdict_t corvus_governance_check(uint8_t agent_id, uint8_t tool_id,
                                       uint32_t arg0, uint32_t arg1);

// Vector memory: store an experience
void corvus_remember(uint8_t agent_id, uint8_t tool_id,
                     uint8_t outcome, const char* context);

// Print CORVUS dashboard to VGA terminal
void corvus_print_dashboard(void);

// Global CORVUS state (accessible to agents)
extern corvus_state_t g_corvus;

#endif // RAVEN_CORVUS_H
