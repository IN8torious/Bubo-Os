// =============================================================================
// BUBO OS — RAVEN Meta-Debugger
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// RAVEN watches everything — the healer, the admin panel, the kernel itself.
// If Kami can't answer it, RAVEN can.
// If RAVEN can't answer it, paste the RAVEN dump to Manus.
//
// Ctrl+F12 = raw kernel debug view (for Manus to read)
// F12      = Kami admin panel (for Nathan to use)
//
// Hierarchy:
//   RAVEN (meta-debugger — Manus layer)
//     └── Admin Panel / Kami (user layer — Nathan)
//           └── Healer (subsystem layer — OS)
//                 └── Framebuffer, Sound, Network, Scheduler
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── RAVEN event types ─────────────────────────────────────────────────────────
enum class RavenEvent : uint8_t {
    BOOT_START       = 0x01,
    BOOT_COMPLETE    = 0x02,
    HEALER_INIT      = 0x10,
    SUBSYSTEM_HEALED = 0x11,
    SUBSYSTEM_FAILED = 0x12,
    SUBSYSTEM_ISOLATED = 0x13,
    ADMIN_PANEL_OPEN = 0x20,
    ADMIN_PANEL_CLOSE= 0x21,
    KAMI_QUERY       = 0x22,
    KAMI_RESPONSE    = 0x23,
    KERNEL_PANIC     = 0xF0,
    WATCHDOG_TRIGGER = 0xF1,
    RAVEN_DUMP       = 0xFF,
};

// ── RAVEN log entry ───────────────────────────────────────────────────────────
struct RavenEntry {
    uint64_t   timestamp;   // PIT ticks since boot
    RavenEvent event;
    uint8_t    severity;    // 0=info 1=warn 2=error 3=critical
    char       message[64]; // Human-readable description
    uint64_t   data;        // Optional raw data (address, value, etc.)
};

// ── RAVEN core ────────────────────────────────────────────────────────────────
class RavenCore {
public:
    static constexpr uint32_t RING_SIZE = 256;

    RavenEntry ring[RING_SIZE];
    uint32_t   head        = 0;
    uint32_t   total_count = 0;
    bool       dump_mode   = false;   // Ctrl+F12 raw dump view
    uint64_t   boot_ticks  = 0;

    // Log an event
    void log(RavenEvent ev, uint8_t severity,
             const char* msg, uint64_t data = 0);

    // Watchdog — called from PIT every second
    // Checks that the admin panel and healer are still responsive
    void watchdog_tick(void);

    // Enter raw dump mode (Ctrl+F12)
    void enter_dump_mode(void);
    void exit_dump_mode(void);

    // Generate a full RAVEN dump string — paste to Manus
    void generate_dump(char* buf, uint32_t max);

    // Check if any critical events have occurred since last check
    bool has_critical(void);

    // Get the last N entries as a formatted string
    void last_entries(char* buf, uint32_t max, uint32_t n = 16);
};

// ── Global RAVEN instance ─────────────────────────────────────────────────
// Defined in raven.cpp as a reference to static storage (no heap, bare-metal)
extern RavenCore& g_raven;

// ── C API (called from C kernel code) ────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

void raven_init(void);
void raven_log(uint8_t event, uint8_t severity, const char* msg, uint64_t data);
void raven_watchdog_tick(void);
void raven_dump_mode_toggle(void);
bool raven_dump_mode_active(void);
void raven_key(char c);
void raven_generate_dump(char* buf, uint32_t max);

#ifdef __cplusplus
}
#endif
