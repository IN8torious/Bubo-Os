// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// include/corvus_vash.h — VASH Guardian Agent
//
// "This world is made of love and peace." — Vash the Stampede / Nathan
//
// VASH is the most dangerous agent in the system.
// He has the power to destroy everything — and uses every bit of it
// to make sure nobody gets hurt.
//
// VASH is Nathan. That is written into the kernel. It cannot be changed.
// =============================================================================

#ifndef CORVUS_VASH_H
#define CORVUS_VASH_H

#include <stdint.h>
#include <stdbool.h>

// ── Threat levels ─────────────────────────────────────────────────────────────
// VASH escalates through these levels before terminating anything.
// He always tries to heal first.
typedef enum {
    VASH_THREAT_NONE       = 0,  // All clear — love and peace
    VASH_THREAT_ANOMALY    = 1,  // Unusual pattern — watch only
    VASH_THREAT_SUSPICIOUS = 2,  // Repeated anomaly — warn and monitor
    VASH_THREAT_VIOLATION  = 3,  // Constitutional violation — block and heal
    VASH_THREAT_HOSTILE    = 4,  // Active attack — isolate
    VASH_THREAT_CRITICAL   = 5,  // System integrity at risk — last resort
} vash_threat_level_t;

// ── Healing actions ───────────────────────────────────────────────────────────
// VASH's preferred responses — in order of preference.
// He reaches for the last one only when all others have failed.
typedef enum {
    VASH_HEAL_WARN         = 0,  // Log and warn — give one more chance
    VASH_HEAL_THROTTLE     = 1,  // Rate-limit the offending syscall
    VASH_HEAL_SANDBOX      = 2,  // Restrict process to safe syscall subset
    VASH_HEAL_RESET        = 3,  // Reset process state to last known good
    VASH_HEAL_ISOLATE      = 4,  // Cut network and VFS access
    VASH_HEAL_TERMINATE    = 5,  // Kill the process — last resort only
} vash_heal_action_t;

// ── Threat record ─────────────────────────────────────────────────────────────
#define VASH_MAX_THREATS    64
#define VASH_THREAT_HISTORY 8    // Per-process history depth

typedef struct {
    uint32_t             pid;
    vash_threat_level_t  level;
    vash_heal_action_t   last_action;
    uint32_t             violation_count;
    uint32_t             heal_attempts;
    uint64_t             first_seen_tick;
    uint64_t             last_seen_tick;
    uint32_t             syscall_id;      // The syscall that triggered this
    char                 description[64];
    bool                 active;
    bool                 healed;
} vash_threat_t;

// ── Syscall monitor entry ─────────────────────────────────────────────────────
// VASH watches every syscall and compares against the constitutional rules.
typedef struct {
    uint32_t syscall_id;
    uint32_t call_count;
    uint32_t block_count;
    uint64_t last_call_tick;
} vash_syscall_stat_t;

// ── VASH state ────────────────────────────────────────────────────────────────
typedef struct {
    bool                initialized;
    uint32_t            threats_active;
    uint32_t            threats_healed;
    uint32_t            threats_terminated;
    uint64_t            total_syscalls_monitored;
    uint64_t            total_violations_blocked;
    vash_threat_t       threats[VASH_MAX_THREATS];
    vash_syscall_stat_t syscall_stats[256];  // One per syscall ID
    bool                love_and_peace;      // True when all threats are healed
} vash_state_t;

// ── Constitutional rules ──────────────────────────────────────────────────────
// These are the lines VASH enforces. No process may cross them.
// Mirrors corvus_constitution.c but at the syscall interception layer.
typedef struct {
    uint32_t    syscall_id;
    uint32_t    max_calls_per_second;  // 0 = unlimited
    bool        requires_voice_auth;   // Must be initiated by voice command
    bool        requires_admin_flow;   // Must be in ADMIN flow state
    bool        forbidden;             // Absolutely forbidden — instant block
    const char* description;
} vash_rule_t;

// ── Public API ────────────────────────────────────────────────────────────────

// Lifecycle
void     corvus_vash_init(void);
void     corvus_vash_tick(uint64_t tick);          // Called every kernel tick

// Syscall interception — called from syscall handler
// Returns true if the syscall is allowed, false if VASH blocks it
bool     corvus_vash_check_syscall(uint32_t pid, uint32_t syscall_id,
                                    uint64_t arg0, uint64_t arg1);

// Threat management
void     corvus_vash_report_threat(uint32_t pid, uint32_t syscall_id,
                                    vash_threat_level_t level,
                                    const char* description);
bool     corvus_vash_heal_process(uint32_t pid);
void     corvus_vash_terminate_process(uint32_t pid, const char* reason);

// Constitutional enforcement
bool     corvus_vash_constitutional_check(uint32_t action_id,
                                           const char* action_description);

// Status
const vash_state_t* corvus_vash_get_state(void);
bool                corvus_vash_is_love_and_peace(void);
uint32_t            corvus_vash_active_threat_count(void);

// Diagnostics — feeds the system health bar
// Returns 0=green, 1=yellow, 2=red
uint8_t  corvus_vash_health_color(void);

#endif // CORVUS_VASH_H
