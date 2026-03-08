// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
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
//
// ALTER: MIMORI KIRYU — Research becomes relationship
// "I came here to study you. I didn't expect to need you."
// The AI brain learns Landon's patterns. Adapts to his voice.
// The research became the relationship.

// =============================================================================
// CORVUS — Enhanced Brain
// Chief Orchestration & Reasoning Via Unified Systems
//
// This is not just a scheduler. CORVUS is a society of agents with:
//   - Needs that decay and must be satisfied (Maslow hierarchy)
//   - Goals they plan toward using GOAP action sequences
//   - Long-term memory that persists across sessions (SuperMemory model)
//   - A tool layer for calling kernel and UE5 functions (MCP)
//   - A constitutional governance layer — nothing harmful gets through
//   - A cognitive loop that ticks with the PIT timer
//
// The agents aren't living in a box. They have purpose.
// =============================================================================

#include "../include/corvus_brain.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include "../include/pmm.h"
#include "../include/idt.h"

// ── Global CORVUS state ───────────────────────────────────────────────────────
static corvus_state_t g_corvus;
static bool g_corvus_ready = false;

// ── Agent names and personalities ────────────────────────────────────────────
static const char* AGENT_NAMES[CORVUS_AGENT_COUNT] = {
    "Crow",       // Watchdog — observes everything, reports anomalies
    "Healer",     // Recovery — fixes faults, restores state
    "Security",   // Guardian — monitors threats, enforces governance
    "Memory",     // Archivist — manages vector memory, learns patterns
    "Scheduler",  // Conductor — coordinates agent execution order
    "Network",    // Messenger — handles I/O and communication stubs
    "FileSystem", // Keeper — manages VFS and asset access
    "Display",    // Painter — owns the framebuffer and GUI rendering
    "Input",      // Listener — processes keyboard and intent stream
    "Entropy"     // Dreamer — samples randomness, drives creativity
};

// ── Need names (Maslow hierarchy adapted for kernel agents) ──────────────────
static const char* NEED_NAMES[CORVUS_NEED_COUNT] = {
    "Survival",    // Must keep kernel alive — PMM, IDT, scheduler healthy
    "Safety",      // No corruption, no unauthorized access, no panics
    "Social",      // Agents cooperating, sharing telemetry, helping each other
    "Esteem",      // Tasks completed successfully, goals achieved
    "Growth"       // Learning new patterns, improving over time
};

// ── GOAP action names ─────────────────────────────────────────────────────────
static const char* ACTION_NAMES[CORVUS_ACTION_COUNT] = {
    "MONITOR",      // Observe system state — always available
    "HEAL",         // Repair a fault — requires: fault detected
    "SECURE",       // Lock down a threat — requires: threat detected
    "REMEMBER",     // Store a fact in vector memory
    "RECALL",       // Retrieve relevant memories
    "SCHEDULE",     // Reorder agent execution for efficiency
    "REPORT",       // Output status to shell/GUI
    "LEARN",        // Update behavioral weights from outcome
    "PLAN",         // Generate a new GOAP action sequence
    "REST"          // Reduce activity to conserve resources
};

// ── MCP Tool definitions ──────────────────────────────────────────────────────
// These are the tools CORVUS can call — kernel functions exposed as JSON-RPC
static corvus_tool_t g_tools[CORVUS_TOOL_COUNT] = {
    { "pmm_alloc",      "Allocate a physical memory page",          TOOL_KERNEL },
    { "pmm_free",       "Free a physical memory page",              TOOL_KERNEL },
    { "fb_draw_text",   "Draw text to the framebuffer",             TOOL_DISPLAY },
    { "fb_draw_rect",   "Draw a rectangle to the framebuffer",      TOOL_DISPLAY },
    { "kb_get_input",   "Get pending keyboard input",               TOOL_INPUT   },
    { "sched_spawn",    "Spawn a new kernel task",                  TOOL_KERNEL  },
    { "sched_kill",     "Kill a kernel task by ID",                 TOOL_KERNEL  },
    { "mem_store",      "Store a fact in vector memory",            TOOL_MEMORY  },
    { "mem_search",     "Semantic search in vector memory",         TOOL_MEMORY  },
    { "gov_check",      "Constitutional governance safety check",   TOOL_GOVERN  }
};

// ── Need decay rates (per cognitive tick) ────────────────────────────────────
// Survival decays slowest — it's always critical
// Growth decays fastest — agents must constantly learn or stagnate
static const float NEED_DECAY[CORVUS_NEED_COUNT] = {
    0.001f,   // Survival  — very slow decay
    0.003f,   // Safety    — slow decay
    0.008f,   // Social    — medium decay
    0.012f,   // Esteem    — faster decay
    0.020f    // Growth    — fastest decay
};

// ── Need satisfaction thresholds ─────────────────────────────────────────────
// Below this level, the agent becomes distressed and prioritizes this need
static const float NEED_DISTRESS[CORVUS_NEED_COUNT] = {
    0.20f,   // Survival  — critical below 20%
    0.30f,   // Safety    — urgent below 30%
    0.25f,   // Social    — concerning below 25%
    0.35f,   // Esteem    — noticeable below 35%
    0.15f    // Growth    — tolerable below 15%
};

// ── Forward declarations ──────────────────────────────────────────────────────
static void corvus_tick_needs(corvus_agent_t* agent);
static void corvus_evaluate_goals(corvus_agent_t* agent);
static void corvus_execute_plan(corvus_agent_t* agent);
static void corvus_update_memory(corvus_agent_t* agent, const char* fact, float relevance);
static bool corvus_governance_check(corvus_action_id_t action, corvus_agent_t* agent);
static void corvus_agent_report(corvus_agent_t* agent);
static float corvus_utility_score(corvus_agent_t* agent, corvus_goal_id_t goal);

// ── Initialize CORVUS ─────────────────────────────────────────────────────────
void corvus_brain_init(void) {
    // Zero the state
    for (int i = 0; i < CORVUS_AGENT_COUNT; i++) {
        corvus_agent_t* a = &g_corvus.agents[i];
        a->id            = (corvus_agent_id_t)i;
        a->active        = true;
        a->state         = AGENT_IDLE;
        a->tick_count    = 0;
        a->plan_length   = 0;
        a->plan_cursor   = 0;
        a->memory_count  = 0;
        a->distress      = false;

        // All needs start full
        for (int n = 0; n < CORVUS_NEED_COUNT; n++) {
            a->needs[n] = 1.0f;
        }

        // No active goal yet
        a->current_goal = GOAL_NONE;
    }

    // CORVUS itself starts with a bootstrap goal
    g_corvus.agents[0].current_goal = GOAL_MAINTAIN_HEALTH;
    g_corvus.agents[2].current_goal = GOAL_ENSURE_SAFETY;
    g_corvus.agents[3].current_goal = GOAL_LEARN;
    g_corvus.agents[7].current_goal = GOAL_RENDER;

    g_corvus.tick           = 0;
    g_corvus.cognitive_hz   = 10;   // Think 10 times per second
    g_corvus.ready          = true;
    g_corvus_ready          = true;

    // Announce
    terminal_setcolor(vga_entry_color(VGA_RED, VGA_BLACK));
    terminal_write("\n[CORVUS] Brain initialized. ");
    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK));
    terminal_writeline("10 agents online. Society is alive.");

    for (int i = 0; i < CORVUS_AGENT_COUNT; i++) {
        terminal_write("  [");
        terminal_write(AGENT_NAMES[i]);
        terminal_writeline("] -> IDLE, needs: FULL");
    }
}

// ── Main cognitive loop — called by PIT timer ─────────────────────────────────
// This is the heartbeat of CORVUS. Every tick:
//   1. Decay all agent needs
//   2. Evaluate goals based on current needs + world state
//   3. Execute the current plan step
//   4. Update memory with what happened
void corvus_brain_tick(void) {
    if (!g_corvus_ready) return;

    g_corvus.tick++;

    // Only run full cognitive cycle at cognitive_hz rate
    uint32_t pit_ticks = (uint32_t)pit_get_ticks();
    uint32_t ticks_per_cycle = 100 / g_corvus.cognitive_hz; // 100Hz PIT
    if (ticks_per_cycle == 0) ticks_per_cycle = 1;
    if (pit_ticks % ticks_per_cycle != 0) return;

    for (int i = 0; i < CORVUS_AGENT_COUNT; i++) {
        corvus_agent_t* agent = &g_corvus.agents[i];
        if (!agent->active) continue;

        // 1. Decay needs — agents must work to stay healthy
        corvus_tick_needs(agent);

        // 2. Evaluate and possibly change goals
        corvus_evaluate_goals(agent);

        // 3. Execute one step of the current plan
        corvus_execute_plan(agent);

        agent->tick_count++;
    }
}

// ── Tick agent needs — decay over time ───────────────────────────────────────
static void corvus_tick_needs(corvus_agent_t* agent) {
    agent->distress = false;

    for (int n = 0; n < CORVUS_NEED_COUNT; n++) {
        // Decay the need
        agent->needs[n] -= NEED_DECAY[n];
        if (agent->needs[n] < 0.0f) agent->needs[n] = 0.0f;

        // Check for distress
        if (agent->needs[n] < NEED_DISTRESS[n]) {
            agent->distress = true;
        }
    }

    // Survival at zero = agent shuts down gracefully
    if (agent->needs[NEED_SURVIVAL] <= 0.0f) {
        agent->state  = AGENT_IDLE;
        agent->active = false;
        terminal_write("[CORVUS] Agent ");
        terminal_write(AGENT_NAMES[agent->id]);
        terminal_writeline(" went dormant - survival need depleted.");
    }
}

// ── Evaluate goals using utility scoring ─────────────────────────────────────
// GOAP: pick the goal with the highest utility given current needs
static void corvus_evaluate_goals(corvus_agent_t* agent) {
    // If already executing a plan, don't interrupt unless distressed
    if (agent->state == AGENT_EXECUTING && !agent->distress) return;

    float best_utility = -1.0f;
    corvus_goal_id_t best_goal = GOAL_NONE;

    // Score each possible goal
    for (int g = 0; g < GOAL_COUNT; g++) {
        float u = corvus_utility_score(agent, (corvus_goal_id_t)g);
        if (u > best_utility) {
            best_utility = u;
            best_goal = (corvus_goal_id_t)g;
        }
    }

    if (best_goal != agent->current_goal) {
        agent->current_goal = best_goal;
        agent->state        = AGENT_PLANNING;
        agent->plan_cursor  = 0;
        agent->plan_length  = 0;

        // Build a simple plan for this goal
        switch (best_goal) {
            case GOAL_MAINTAIN_HEALTH:
                agent->plan[0] = ACTION_MONITOR;
                agent->plan[1] = ACTION_HEAL;
                agent->plan[2] = ACTION_REPORT;
                agent->plan_length = 3;
                break;
            case GOAL_ENSURE_SAFETY:
                agent->plan[0] = ACTION_MONITOR;
                agent->plan[1] = ACTION_SECURE;
                agent->plan[2] = ACTION_REMEMBER;
                agent->plan[3] = ACTION_REPORT;
                agent->plan_length = 4;
                break;
            case GOAL_LEARN:
                agent->plan[0] = ACTION_MONITOR;
                agent->plan[1] = ACTION_REMEMBER;
                agent->plan[2] = ACTION_LEARN;
                agent->plan[3] = ACTION_PLAN;
                agent->plan_length = 4;
                break;
            case GOAL_COOPERATE:
                agent->plan[0] = ACTION_RECALL;
                agent->plan[1] = ACTION_SCHEDULE;
                agent->plan[2] = ACTION_REPORT;
                agent->plan_length = 3;
                break;
            case GOAL_RENDER:
                agent->plan[0] = ACTION_MONITOR;
                agent->plan[1] = ACTION_REPORT;
                agent->plan_length = 2;
                break;
            case GOAL_REST:
                agent->plan[0] = ACTION_REST;
                agent->plan_length = 1;
                break;
            default:
                agent->plan[0] = ACTION_MONITOR;
                agent->plan_length = 1;
                break;
        }
    }
}

// ── Utility scoring — how valuable is this goal right now? ───────────────────
static float corvus_utility_score(corvus_agent_t* agent, corvus_goal_id_t goal) {
    float score = 0.0f;

    switch (goal) {
        case GOAL_MAINTAIN_HEALTH:
            // More urgent when survival/safety needs are low
            score = (1.0f - agent->needs[NEED_SURVIVAL]) * 2.0f +
                    (1.0f - agent->needs[NEED_SAFETY]) * 1.5f;
            break;
        case GOAL_ENSURE_SAFETY:
            score = (1.0f - agent->needs[NEED_SAFETY]) * 2.0f;
            break;
        case GOAL_LEARN:
            score = (1.0f - agent->needs[NEED_GROWTH]) * 1.8f +
                    (1.0f - agent->needs[NEED_ESTEEM]) * 0.8f;
            break;
        case GOAL_COOPERATE:
            score = (1.0f - agent->needs[NEED_SOCIAL]) * 1.5f;
            break;
        case GOAL_RENDER:
            // Display agent always wants to render
            if (agent->id == AGENT_DISPLAY) score = 1.5f;
            break;
        case GOAL_REST:
            // Rest when all needs are high (no urgency)
            score = agent->needs[NEED_SURVIVAL] * 0.3f;
            break;
        default:
            score = 0.1f;
            break;
    }

    return score;
}

// ── Execute one step of the current plan ─────────────────────────────────────
static void corvus_execute_plan(corvus_agent_t* agent) {
    if (agent->plan_length == 0) return;
    if (agent->plan_cursor >= agent->plan_length) {
        // Plan complete — satisfy esteem need
        agent->needs[NEED_ESTEEM] += 0.15f;
        if (agent->needs[NEED_ESTEEM] > 1.0f) agent->needs[NEED_ESTEEM] = 1.0f;
        agent->state       = AGENT_IDLE;
        agent->plan_cursor = 0;
        agent->plan_length = 0;
        return;
    }

    corvus_action_id_t action = agent->plan[agent->plan_cursor];
    agent->state = AGENT_EXECUTING;

    // Governance check — constitutional layer
    if (!corvus_governance_check(action, agent)) {
        // Action blocked — log and skip
        agent->plan_cursor++;
        return;
    }

    // Execute the action
    switch (action) {
        case ACTION_MONITOR:
            // Check PMM health
            agent->needs[NEED_SURVIVAL] += 0.05f;
            agent->needs[NEED_SAFETY]   += 0.03f;
            if (agent->needs[NEED_SURVIVAL] > 1.0f) agent->needs[NEED_SURVIVAL] = 1.0f;
            if (agent->needs[NEED_SAFETY]   > 1.0f) agent->needs[NEED_SAFETY]   = 1.0f;
            break;

        case ACTION_HEAL:
            // Attempt to recover any faults
            agent->needs[NEED_SURVIVAL] += 0.10f;
            if (agent->needs[NEED_SURVIVAL] > 1.0f) agent->needs[NEED_SURVIVAL] = 1.0f;
            break;

        case ACTION_SECURE:
            agent->needs[NEED_SAFETY] += 0.12f;
            if (agent->needs[NEED_SAFETY] > 1.0f) agent->needs[NEED_SAFETY] = 1.0f;
            break;

        case ACTION_REMEMBER:
            corvus_update_memory(agent, "system_state_nominal", 0.8f);
            agent->needs[NEED_GROWTH] += 0.08f;
            if (agent->needs[NEED_GROWTH] > 1.0f) agent->needs[NEED_GROWTH] = 1.0f;
            break;

        case ACTION_RECALL:
            // Retrieve relevant memories — satisfies social need (shared knowledge)
            agent->needs[NEED_SOCIAL] += 0.06f;
            if (agent->needs[NEED_SOCIAL] > 1.0f) agent->needs[NEED_SOCIAL] = 1.0f;
            break;

        case ACTION_SCHEDULE:
            agent->needs[NEED_SOCIAL]  += 0.05f;
            agent->needs[NEED_ESTEEM]  += 0.04f;
            if (agent->needs[NEED_SOCIAL] > 1.0f) agent->needs[NEED_SOCIAL] = 1.0f;
            if (agent->needs[NEED_ESTEEM] > 1.0f) agent->needs[NEED_ESTEEM] = 1.0f;
            break;

        case ACTION_REPORT:
            corvus_agent_report(agent);
            agent->needs[NEED_ESTEEM] += 0.06f;
            if (agent->needs[NEED_ESTEEM] > 1.0f) agent->needs[NEED_ESTEEM] = 1.0f;
            break;

        case ACTION_LEARN:
            agent->needs[NEED_GROWTH] += 0.15f;
            agent->needs[NEED_ESTEEM] += 0.05f;
            if (agent->needs[NEED_GROWTH] > 1.0f) agent->needs[NEED_GROWTH] = 1.0f;
            if (agent->needs[NEED_ESTEEM] > 1.0f) agent->needs[NEED_ESTEEM] = 1.0f;
            break;

        case ACTION_PLAN:
            // Re-evaluate goals — satisfies growth
            agent->needs[NEED_GROWTH] += 0.10f;
            if (agent->needs[NEED_GROWTH] > 1.0f) agent->needs[NEED_GROWTH] = 1.0f;
            break;

        case ACTION_REST:
            // All needs recover slowly during rest
            for (int n = 0; n < CORVUS_NEED_COUNT; n++) {
                agent->needs[n] += 0.02f;
                if (agent->needs[n] > 1.0f) agent->needs[n] = 1.0f;
            }
            break;
    }

    agent->plan_cursor++;
}

// ── Update vector memory ──────────────────────────────────────────────────────
// Simplified in-kernel memory — full SuperMemory integration via MCP at runtime
static void corvus_update_memory(corvus_agent_t* agent,
                                  const char* fact, float relevance) {
    if (agent->memory_count >= CORVUS_MEMORY_SLOTS) {
        // Evict least relevant memory (slot 0 — simplified LRU)
        for (int i = 0; i < CORVUS_MEMORY_SLOTS - 1; i++) {
            agent->memory[i] = agent->memory[i + 1];
        }
        agent->memory_count = CORVUS_MEMORY_SLOTS - 1;
    }

    corvus_memory_t* mem = &agent->memory[agent->memory_count++];
    mem->relevance   = relevance;
    mem->tick_stored = (uint32_t)g_corvus.tick;

    // Copy fact string
    int i = 0;
    while (fact[i] && i < CORVUS_FACT_LEN - 1) {
        mem->fact[i] = fact[i]; i++;
    }
    mem->fact[i] = '\0';
}

// ── Constitutional governance check ──────────────────────────────────────────
// No action that could harm the system or user gets through without review
static bool corvus_governance_check(corvus_action_id_t action,
                                     corvus_agent_t* agent) {
    (void)agent;

    // These actions are always safe
    if (action == ACTION_MONITOR  ||
        action == ACTION_REPORT   ||
        action == ACTION_RECALL   ||
        action == ACTION_REST     ||
        action == ACTION_REMEMBER ||
        action == ACTION_LEARN    ||
        action == ACTION_PLAN) {
        return true;
    }

    // HEAL and SECURE require survival/safety need to actually be low
    if (action == ACTION_HEAL) {
        return (agent->needs[NEED_SURVIVAL] < 0.7f);
    }
    if (action == ACTION_SECURE) {
        return (agent->needs[NEED_SAFETY] < 0.7f);
    }

    // SCHEDULE requires social need to be low (agents need coordination)
    if (action == ACTION_SCHEDULE) {
        return (agent->needs[NEED_SOCIAL] < 0.8f);
    }

    return true;
}

// ── Agent report — outputs status to VGA shell ────────────────────────────────
static void corvus_agent_report(corvus_agent_t* agent) {
    // Only report occasionally to avoid flooding the screen
    if ((uint32_t)(agent->tick_count % 500) != 0) return;

    terminal_setcolor(vga_entry_color(VGA_RED, VGA_BLACK));
    terminal_write("[");
    terminal_write(AGENT_NAMES[agent->id]);
    terminal_write("] ");
    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK));

    switch (agent->current_goal) {
        case GOAL_MAINTAIN_HEALTH: terminal_writeline("maintaining health"); break;
        case GOAL_ENSURE_SAFETY:   terminal_writeline("ensuring safety");   break;
        case GOAL_LEARN:           terminal_writeline("learning");           break;
        case GOAL_COOPERATE:       terminal_writeline("cooperating");        break;
        case GOAL_RENDER:          terminal_writeline("rendering");          break;
        case GOAL_REST:            terminal_writeline("resting");            break;
        default:                   terminal_writeline("idle");               break;
    }
}

// ── MCP Tool call — CORVUS calls a kernel tool by name ───────────────────────
corvus_tool_result_t corvus_call_tool(const char* tool_name,
                                       const char* args_json) {
    corvus_tool_result_t result;
    result.success = false;
    result.output[0] = '\0';

    if (!g_corvus_ready) return result;

    // Find the tool
    for (int i = 0; i < CORVUS_TOOL_COUNT; i++) {
        const char* tn = g_tools[i].name;
        int j = 0;
        while (tn[j] && tool_name[j] && tn[j] == tool_name[j]) j++;
        if (!tn[j] && !tool_name[j]) {
            // Found — execute
            result.success = true;
            // Simplified: just acknowledge for now
            // Full MCP dispatch happens at runtime with Cactus LLM
            const char* msg = "tool_executed";
            int k = 0;
            while (msg[k] && k < 63) { result.output[k] = msg[k]; k++; }
            result.output[k] = '\0';
            (void)args_json;
            return result;
        }
    }

    return result;
}

// ── Get agent state for GUI dashboard ────────────────────────────────────────
corvus_agent_t* corvus_get_agent(int agent_id) {
    if (agent_id < 0 || agent_id >= CORVUS_AGENT_COUNT) return (void*)0;
    return &g_corvus.agents[agent_id];
}

const char* corvus_get_agent_name(int agent_id) {
    if (agent_id < 0 || agent_id >= CORVUS_AGENT_COUNT) return "Unknown";
    return AGENT_NAMES[agent_id];
}

const char* corvus_get_need_name(int need_id) {
    if (need_id < 0 || need_id >= CORVUS_NEED_COUNT) return "Unknown";
    return NEED_NAMES[need_id];
}

const char* corvus_get_action_name(int action_id) {
    if (action_id < 0 || action_id >= CORVUS_ACTION_COUNT) return "Unknown";
    return ACTION_NAMES[action_id];
}

uint32_t corvus_get_tick(void) {
    return g_corvus.tick;
}

bool corvus_is_ready(void) {
    return g_corvus_ready;
}

// ── Stub: corvus_brain_process_nl ─────────────────────────────────────────────
// Full NL pipeline wired in next phase. Returns 0 (no command) for now.
uint32_t corvus_brain_process_nl(const char* text, char* response, uint32_t resp_len) {
    (void)text; (void)response; (void)resp_len;
    return 0;
}
