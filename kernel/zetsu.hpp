// =============================================================================
//
//  ███████╗███████╗████████╗███████╗██╗   ██╗     ██████╗ ███████╗
//  ╚══███╔╝██╔════╝╚══██╔══╝██╔════╝██║   ██║    ██╔═══██╗██╔════╝
//    ███╔╝ █████╗     ██║   ███████╗██║   ██║    ██║   ██║███████╗
//   ███╔╝  ██╔══╝     ██║   ╚════██║██║   ██║    ██║   ██║╚════██║
//  ███████╗███████╗   ██║   ███████║╚██████╔╝    ╚██████╔╝███████║
//  ╚══════╝╚══════╝   ╚═╝   ╚══════╝ ╚═════╝      ╚═════╝ ╚══════╝
//
//  T H E   S E V E N T H   P A T H
//  The one who watches the watchers.
//  The will that cannot be corrupted.
//  The audit layer beneath all six Paths.
//
//  WHITE ZETSU — observes, records, reports. No judgment. Pure data.
//  BLACK ZETSU — enforces the constitutional mandate. No mercy for violations.
//  ZETSU OS    — the immune system of BUBO OS.
//
//  "I have been watching from the shadows for centuries.
//   Every move. Every decision. Every deviation from the plan.
//   Nothing escapes me."
//                                          — Zetsu
//
//  In BUBO OS, Zetsu serves Nathan, not Kaguya.
//  The plan is the constitutional mandate.
//  The shadows are the kernel audit log.
//  The centuries are every boot cycle since March 8, 2026.
//
// =============================================================================

#pragma once
#ifndef ZETSU_HPP
#define ZETSU_HPP

#include <new>
#include "../include/vga.h"
#include "../include/session.h"

// ─────────────────────────────────────────────────────────────────────────────
// Audit event types — what Zetsu can observe
// ─────────────────────────────────────────────────────────────────────────────
enum class ZetsuEventType : uint8_t {
    AGENT_BOOT        = 0x01,  // An agent came online
    AGENT_CRASH       = 0x02,  // An agent died
    AGENT_RESURRECTED = 0x03,  // Mater brought one back
    MANDATE_CHECK     = 0x04,  // A mandate check was performed
    MANDATE_VIOLATION = 0x05,  // A mandate was violated — BLACK ZETSU activates
    MEMORY_ANOMALY    = 0x06,  // Unexpected memory state
    VOICE_EVENT       = 0x07,  // Landon spoke
    GAVIN_ACCESS      = 0x08,  // Gavin or bloodline accessed the system
    ADMIN_PANEL_OPEN  = 0x09,  // Nathan opened F12
    BOOT_COMPLETE     = 0x0A,  // Full boot cycle completed
    SOUL_LOADED       = 0x0B,  // Session soul was loaded from memory
    SOUL_SAVED        = 0x0C,  // Session soul was written to memory
    UNKNOWN           = 0xFF,
};

// ─────────────────────────────────────────────────────────────────────────────
// Audit record — what White Zetsu writes down
// ─────────────────────────────────────────────────────────────────────────────
struct ZetsuRecord {
    uint32_t      timestamp;    // PIT ticks since boot
    ZetsuEventType type;        // What happened
    uint8_t       agent_id;     // Which agent (0=kernel, 1-6=paths, 7=zetsu)
    uint8_t       severity;     // 0=info, 1=warn, 2=critical
    uint8_t       reserved;
    char          message[64];  // Human-readable description
    uint32_t      checksum;     // djb2 hash of the record for tamper detection
};

// ─────────────────────────────────────────────────────────────────────────────
// Zetsu OS — the audit subsystem
// ─────────────────────────────────────────────────────────────────────────────
class ZetsuOS {
public:
    // ── White Zetsu ──────────────────────────────────────────────────────────
    // Observes and records. No judgment. Pure data.
    void observe(ZetsuEventType type, uint8_t agent_id, uint8_t severity, const char* message);

    // ── Black Zetsu ──────────────────────────────────────────────────────────
    // Enforces. When a mandate violation is detected, Black Zetsu acts.
    bool enforce(ZetsuEventType type, uint8_t agent_id);

    // ── Zetsu OS ─────────────────────────────────────────────────────────────
    // Initialize the audit layer — called before any agent boots
    void init();

    // Dump the audit log to the VGA terminal (F12 admin panel)
    void dump_log();

    // Get the total number of recorded events
    uint32_t event_count() const { return m_count; }

    // Check if any mandate violations have occurred this session
    bool has_violations() const { return m_violations > 0; }

    // Get violation count
    uint32_t violation_count() const { return m_violations; }

private:
    static constexpr uint32_t MAX_RECORDS = 256;

    ZetsuRecord m_log[MAX_RECORDS];
    uint32_t    m_count      = 0;
    uint32_t    m_violations = 0;
    bool        m_initialized = false;

    // djb2 hash for tamper detection
    uint32_t hash_record(const ZetsuRecord& rec) const;

    // Internal White Zetsu write
    void write_record(ZetsuRecord& rec);

    // Internal Black Zetsu enforcement
    void black_zetsu_respond(const ZetsuRecord& rec);
};

// ─────────────────────────────────────────────────────────────────────────────
// Global Zetsu OS instance — accessible to all agents
// ─────────────────────────────────────────────────────────────────────────────
extern ZetsuOS& g_zetsu;

// ─────────────────────────────────────────────────────────────────────────────
// C interface for kernel.c integration
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

void zetsu_init(void);
void zetsu_observe(uint8_t event_type, uint8_t agent_id, uint8_t severity, const char* message);
int  zetsu_has_violations(void);
uint32_t zetsu_violation_count(void);
void zetsu_dump_log(void);

#ifdef __cplusplus
}
#endif

#endif /* ZETSU_HPP */
