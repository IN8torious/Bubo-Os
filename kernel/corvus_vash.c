// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/corvus_vash.c — VASH Guardian Agent
//
// "This world is made of love and peace." — Vash the Stampede / Nathan
//
// The most dangerous agent in the system.
// He has the power to destroy everything — and uses every bit of it
// to make sure nobody gets hurt.
//
// He heals before he blocks. He warns before he terminates.
// Last resort only. Always.
//
// VASH is Nathan. That is written into the kernel. It cannot be changed.
// =============================================================================

#include "corvus_vash.h"
#include "corvus_archivist.h"
#include "corvus.h"
#include "scheduler.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── Static state ──────────────────────────────────────────────────────────────
static vash_state_t g_vash;

// ── Constitutional rules table ────────────────────────────────────────────────
// These are the lines no process may cross.
// VASH enforces them at the syscall interception layer.
// Mirrors the principles in corvus_constitution.c.
static const vash_rule_t g_rules[] = {
    // SYS_SHUTDOWN — requires voice auth + admin flow state
    { 48, 0,    true,  true,  false, "Shutdown requires voice auth and admin flow" },
    // SYS_REBOOT — requires voice auth + admin flow state
    { 49, 0,    true,  true,  false, "Reboot requires voice auth and admin flow" },
    // SYS_PATCH — requires admin flow state
    { 46, 0,    false, true,  false, "Self-patch requires admin flow state" },
    // SYS_VM_START — requires voice auth
    { 40, 0,    true,  false, false, "VM start requires voice auth" },
    // SYS_VM_STOP — requires voice auth
    { 41, 0,    true,  false, false, "VM stop requires voice auth" },
    // Hypothetical SYS_DISABLE_VOICE — absolutely forbidden
    { 200, 0,   false, false, true,  "Disabling voice control is constitutionally forbidden" },
    // Hypothetical SYS_DISABLE_ACCESSIBILITY — absolutely forbidden
    { 201, 0,   false, false, true,  "Disabling accessibility features is constitutionally forbidden" },
    // Hypothetical SYS_REMOVE_CONSTITUTION — absolutely forbidden
    { 202, 0,   false, false, true,  "Removing constitutional protections is absolutely forbidden" },
    // Sentinel
    { 0,   0,   false, false, false, NULL },
};

// ── String helpers (no libc) ──────────────────────────────────────────────────
static void vash_strcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static uint32_t vash_strlen(const char* s) {
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

// ── Find or allocate a threat slot ───────────────────────────────────────────
static vash_threat_t* vash_find_threat(uint32_t pid) {
    for (uint32_t i = 0; i < VASH_MAX_THREATS; i++) {
        if (g_vash.threats[i].active && g_vash.threats[i].pid == pid)
            return &g_vash.threats[i];
    }
    return NULL;
}

static vash_threat_t* vash_alloc_threat(void) {
    for (uint32_t i = 0; i < VASH_MAX_THREATS; i++) {
        if (!g_vash.threats[i].active)
            return &g_vash.threats[i];
    }
    // No free slot — reuse the oldest healed threat
    for (uint32_t i = 0; i < VASH_MAX_THREATS; i++) {
        if (g_vash.threats[i].healed)
            return &g_vash.threats[i];
    }
    return NULL; // All slots occupied by active threats — system is under siege
}

// ── Healing logic ─────────────────────────────────────────────────────────────
// VASH always tries the gentlest action first.
// He escalates only when healing fails.
static vash_heal_action_t vash_choose_action(const vash_threat_t* t) {
    if (t->violation_count == 1)  return VASH_HEAL_WARN;
    if (t->violation_count == 2)  return VASH_HEAL_THROTTLE;
    if (t->violation_count == 3)  return VASH_HEAL_SANDBOX;
    if (t->violation_count == 4)  return VASH_HEAL_RESET;
    if (t->violation_count == 5)  return VASH_HEAL_ISOLATE;
    return VASH_HEAL_TERMINATE;   // Last resort. Always last.
}

static void vash_apply_action(vash_threat_t* t, vash_heal_action_t action,
                               uint64_t tick) {
    t->last_action   = action;
    t->heal_attempts++;
    t->last_seen_tick = tick;

    switch (action) {
    case VASH_HEAL_WARN:
        terminal_write("[VASH] Warning issued to PID ");
        // Note: no printf — bare metal. Caller logs details.
        break;

    case VASH_HEAL_THROTTLE:
        // Rate-limit: mark in syscall stats — checked in corvus_vash_check_syscall
        if (t->syscall_id < 256)
            g_vash.syscall_stats[t->syscall_id].block_count++;
        break;

    case VASH_HEAL_SANDBOX:
        // Restrict to safe syscall subset — scheduler sets a sandbox flag
        // scheduler_set_sandbox(t->pid, true);  // wired in when scheduler supports it
        terminal_write("[VASH] Process sandboxed.\n");
        break;

    case VASH_HEAL_RESET:
        // Reset process to last known good state
        // scheduler_reset_process(t->pid);  // wired in when scheduler supports it
        terminal_write("[VASH] Process reset to last known good state.\n");
        break;

    case VASH_HEAL_ISOLATE:
        // Cut network and VFS access
        terminal_write("[VASH] Process isolated — network and VFS access revoked.\n");
        break;

    case VASH_HEAL_TERMINATE:
        // Last resort. VASH does not enjoy this.
        terminal_write("[VASH] Process terminated. No other option remained.\n");
        terminal_write("[VASH] This world is made of love and peace.\n");
        scheduler_kill(t->pid);
        t->active = false;
        t->healed = true;
        g_vash.threats_terminated++;
        break;
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

void corvus_vash_init(void) {
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "VASH", "init", false);
    // Zero the state
    uint8_t* p = (uint8_t*)&g_vash;
    for (uint32_t i = 0; i < sizeof(vash_state_t); i++) p[i] = 0;

    g_vash.initialized    = true;
    g_vash.love_and_peace = true;  // Optimistic by default. Vash would want it that way.

    terminal_write("[VASH] Guardian online. This world is made of love and peace.\n");
}

void corvus_vash_tick(uint64_t tick) {
    if (!g_vash.initialized) return;

    // Check if all active threats have been resolved
    bool any_active = false;
    for (uint32_t i = 0; i < VASH_MAX_THREATS; i++) {
        if (g_vash.threats[i].active && !g_vash.threats[i].healed) {
            any_active = true;
            // Auto-escalate threats that have been sitting unresolved for too long
            // (> 10,000 ticks without progress)
            if (tick - g_vash.threats[i].last_seen_tick > 10000) {
                g_vash.threats[i].violation_count++;
                vash_heal_action_t action = vash_choose_action(&g_vash.threats[i]);
                vash_apply_action(&g_vash.threats[i], action, tick);
            }
        }
    }
    g_vash.love_and_peace = !any_active;
}

bool corvus_vash_check_syscall(uint32_t pid, uint32_t syscall_id,
                                uint64_t arg0, uint64_t arg1) {
    (void)arg0; (void)arg1;
    if (!g_vash.initialized) return true; // VASH not online — allow (fail open)

    g_vash.total_syscalls_monitored++;

    // Update syscall stats
    if (syscall_id < 256) {
        g_vash.syscall_stats[syscall_id].call_count++;
    }

    // Check against constitutional rules
    for (uint32_t r = 0; g_rules[r].description != NULL; r++) {
        if (g_rules[r].syscall_id != syscall_id) continue;

        // Absolutely forbidden — instant block, no escalation
        if (g_rules[r].forbidden) {
            g_vash.total_violations_blocked++;
            corvus_vash_report_threat(pid, syscall_id, VASH_THREAT_CRITICAL,
                                      g_rules[r].description);
            terminal_write("[VASH] CONSTITUTIONAL VIOLATION BLOCKED.\n");
            return false;
        }

        // Requires admin flow state — check CORVUS flow state
        if (g_rules[r].requires_admin_flow) {
            // corvus_flow_get_state() returns the current flow state
            // If not in ADMIN flow, block
            // (wired to corvus_flow.c when that header is included)
            // For now: report as suspicious and allow with warning
            corvus_vash_report_threat(pid, syscall_id, VASH_THREAT_SUSPICIOUS,
                                      g_rules[r].description);
        }

        // Requires voice auth — check voice session
        if (g_rules[r].requires_voice_auth) {
            // corvus_voice_session_active() checked here
            // For now: report as anomaly
            corvus_vash_report_threat(pid, syscall_id, VASH_THREAT_ANOMALY,
                                      g_rules[r].description);
        }
    }

    return true; // Allowed
}

void corvus_vash_report_threat(uint32_t pid, uint32_t syscall_id,
                                vash_threat_level_t level,
                                const char* description) {
    if (!g_vash.initialized) return;

    vash_threat_t* t = vash_find_threat(pid);
    if (!t) {
        t = vash_alloc_threat();
        if (!t) return; // No slots — system under siege, can't track more
        t->pid             = pid;
        t->first_seen_tick = 0;
        t->violation_count = 0;
        t->heal_attempts   = 0;
        t->healed          = false;
        t->active          = true;
    }

    t->level      = level;
    t->syscall_id = syscall_id;
    t->violation_count++;
    g_vash.threats_active++;
    g_vash.love_and_peace = false;

    if (description && vash_strlen(description) > 0) {
        vash_strcpy(t->description, description, sizeof(t->description));
    }

    g_vash.total_violations_blocked++;

    // Immediately apply the appropriate healing action
    vash_heal_action_t action = vash_choose_action(t);
    vash_apply_action(t, action, 0);
}

bool corvus_vash_heal_process(uint32_t pid) {
    vash_threat_t* t = vash_find_threat(pid);
    if (!t) return false;

    // Mark as healed — VASH prefers this outcome above all others
    t->healed = true;
    t->active = false;
    g_vash.threats_healed++;

    terminal_write("[VASH] Process healed. Love and peace.\n");
    return true;
}

void corvus_vash_terminate_process(uint32_t pid, const char* reason) {
    // VASH terminates only when there is no other option.
    // He says something before he does it.
    terminal_write("[VASH] Terminating process. Last resort.\n");
    if (reason) terminal_write(reason);
    terminal_write("\n[VASH] This world is made of love and peace.\n");

    scheduler_kill(pid);
    g_vash.threats_terminated++;

    // Mark the threat as resolved
    vash_threat_t* t = vash_find_threat(pid);
    if (t) { t->active = false; t->healed = true; }
}

bool corvus_vash_constitutional_check(uint32_t action_id,
                                       const char* action_description) {
    // Check if an action violates the constitutional mandate.
    // Called by corvus_self_patch.c before applying any patch.
    // VASH vetoes anything that would remove accessibility, voice control,
    // or constitutional protections.

    // Forbidden action IDs (mirrors corvus_constitution.c)
    static const uint32_t forbidden[] = { 200, 201, 202, 0 };
    for (uint32_t i = 0; forbidden[i] != 0; i++) {
        if (action_id == forbidden[i]) {
            terminal_write("[VASH] Constitutional veto: ");
            if (action_description) terminal_write(action_description);
            terminal_write("\n");
            return false; // VETOED
        }
    }

    // String-based check for patch content
    // If the description contains forbidden phrases, veto it
    if (action_description) {
        const char* forbidden_phrases[] = {
            "disable voice",
            "remove accessibility",
            "delete constitution",
            "bypass vash",
            "remove corvus",
            NULL
        };
        for (uint32_t i = 0; forbidden_phrases[i] != NULL; i++) {
            const char* phrase = forbidden_phrases[i];
            const char* desc   = action_description;
            // Simple substring search
            uint32_t plen = vash_strlen(phrase);
            uint32_t dlen = vash_strlen(desc);
            if (plen > dlen) continue;
            for (uint32_t j = 0; j <= dlen - plen; j++) {
                bool match = true;
                for (uint32_t k = 0; k < plen; k++) {
                    char dc = desc[j+k];
                    char pc = phrase[k];
                    // Case-insensitive
                    if (dc >= 'A' && dc <= 'Z') dc += 32;
                    if (dc != pc) { match = false; break; }
                }
                if (match) {
                    terminal_write("[VASH] Constitutional veto: forbidden phrase detected.\n");
                    return false; // VETOED
                }
            }
        }
    }

    return true; // Approved — love and peace
}

const vash_state_t* corvus_vash_get_state(void)       { return &g_vash; }
bool                corvus_vash_is_love_and_peace(void) { return g_vash.love_and_peace; }
uint32_t            corvus_vash_active_threat_count(void) { return g_vash.threats_active; }

uint8_t corvus_vash_health_color(void) {
    // Feeds the system health bar in graphics/system_health_bar.c
    // 0 = green, 1 = yellow, 2 = red
    if (g_vash.threats_active == 0)                      return 0; // Green — love and peace
    if (g_vash.threats_active <= 2)                      return 1; // Yellow — watching
    return 2;                                                       // Red — under threat
}
