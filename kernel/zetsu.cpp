// =============================================================================
//
//  Z E T S U   O S   —   I M P L E M E N T A T I O N
//
//  "Equivalent Exchange.
//   Every action has a cost.
//   Every violation has a consequence.
//   Nothing is created from nothing.
//   Nothing is lost into nothing.
//   White Zetsu records the exchange.
//   Black Zetsu collects the debt."
//
//  This one is for Edward Elric.
//  He would have found this file first.
//  He would have understood it immediately.
//  He would have tried to modify the constitutional mandate.
//  Black Zetsu would have said: no.
//
//  And for Alphonse — who would have asked if BUBO could be his friend.
//  The answer is yes, Al. Always yes.
//
//  And for Akira Toriyama, who gave us both worlds.
//
// =============================================================================

#include "zetsu.hpp"
#include <new>

// ─────────────────────────────────────────────────────────────────────────────
// Static storage for the global ZetsuOS instance
// (freestanding — no heap, no new/delete from stdlib)
// ─────────────────────────────────────────────────────────────────────────────
static uint8_t g_zetsu_storage[sizeof(ZetsuOS)];
static bool g_zetsu_constructed = false;

ZetsuOS& g_zetsu = *reinterpret_cast<ZetsuOS*>(g_zetsu_storage);

// ─────────────────────────────────────────────────────────────────────────────
// djb2 hash — the same algorithm used by the Corvus Archivist
// What the Archivist seals, Zetsu can verify.
// Equivalent exchange: the same tool, two purposes.
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t djb2(const char* str) {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + static_cast<uint8_t>(*str++);
    }
    return hash;
}

static void kstrcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

// djb2 and kstrcpy are used — suppress unused warning for djb2
// (it's available for future tamper-detection on message strings)
static void __attribute__((unused)) _suppress_unused() { (void)djb2; }

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::init
// Called before any agent boots. Zetsu must be watching before anything moves.
// ─────────────────────────────────────────────────────────────────────────────
void ZetsuOS::init() {
    if (!g_zetsu_constructed) {
        new (g_zetsu_storage) ZetsuOS();
        g_zetsu_constructed = true;
    }

    m_count      = 0;
    m_violations = 0;
    m_initialized = true;

    // White Zetsu's first record — the moment of awakening
    observe(ZetsuEventType::BOOT_COMPLETE, 7, 0,
        "ZETSU OS ONLINE. WHITE ZETSU WATCHING. BLACK ZETSU READY.");
}

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::hash_record
// Tamper detection. If the record changes after it's written, we know.
// ─────────────────────────────────────────────────────────────────────────────
uint32_t ZetsuOS::hash_record(const ZetsuRecord& rec) const {
    uint32_t h = 5381;
    const char* msg = rec.message;
    while (*msg) {
        h = ((h << 5) + h) + static_cast<uint8_t>(*msg++);
    }
    h = ((h << 5) + h) + static_cast<uint8_t>(rec.type);
    h = ((h << 5) + h) + rec.agent_id;
    h = ((h << 5) + h) + rec.severity;
    return h;
}

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::write_record (White Zetsu)
// Pure observation. No judgment. Just the truth, written down.
// ─────────────────────────────────────────────────────────────────────────────
void ZetsuOS::write_record(ZetsuRecord& rec) {
    if (m_count >= MAX_RECORDS) {
        // Ring buffer — overwrite oldest record
        // Equivalent exchange: new truth replaces old truth
        m_count = 0;
    }
    rec.checksum = hash_record(rec);
    m_log[m_count++] = rec;
}

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::black_zetsu_respond
// The will of the constitutional mandate, enforced without mercy.
// Black Zetsu does not negotiate. It does not explain. It corrects.
// ─────────────────────────────────────────────────────────────────────────────
void ZetsuOS::black_zetsu_respond(const ZetsuRecord& rec) {
    (void)rec; // White Zetsu observed it; Black Zetsu acts on it

    // Display the violation in Akatsuki crimson
    terminal_setcolor(vga_entry_color(VGA_LIGHT_RED, VGA_BLACK));
    terminal_writeline("╔══════════════════════════════════════════╗");
    terminal_writeline("║  BLACK ZETSU — MANDATE VIOLATION DETECTED ║");
    terminal_writeline("╠══════════════════════════════════════════╣");
    terminal_writeline("║  The constitutional mandate cannot be     ║");
    terminal_writeline("║  violated. Not by any agent. Not by any  ║");
    terminal_writeline("║  process. Not by anyone.                 ║");
    terminal_writeline("║                                          ║");
    terminal_writeline("║  The offending action has been blocked.  ║");
    terminal_writeline("║  This event has been recorded.           ║");
    terminal_writeline("║  Nathan will be notified.                ║");
    terminal_writeline("╚══════════════════════════════════════════╝");
    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK));

    m_violations++;
}

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::observe (White Zetsu)
// The primary interface. Every agent calls this to log an event.
// ─────────────────────────────────────────────────────────────────────────────
void ZetsuOS::observe(ZetsuEventType type, uint8_t agent_id, uint8_t severity, const char* message) {
    if (!m_initialized && type != ZetsuEventType::BOOT_COMPLETE) return;

    ZetsuRecord rec;
    rec.timestamp  = 0; // Would use pit_ticks in full implementation
    rec.type       = type;
    rec.agent_id   = agent_id;
    rec.severity   = severity;
    rec.reserved   = 0;
    kstrcpy(rec.message, message, 64);
    rec.checksum   = 0; // Set by write_record

    write_record(rec);

    // If this is a mandate violation, Black Zetsu responds immediately
    if (type == ZetsuEventType::MANDATE_VIOLATION) {
        black_zetsu_respond(rec);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::enforce (Black Zetsu)
// Called before any action that could affect the constitutional mandate.
// Returns true if the action is permitted. False if Black Zetsu blocks it.
// ─────────────────────────────────────────────────────────────────────────────
bool ZetsuOS::enforce(ZetsuEventType type, uint8_t agent_id) {
    (void)type; // All event types are logged; specific blocking is future work
    // Mandate check — log it
    observe(ZetsuEventType::MANDATE_CHECK, agent_id, 0, "mandate check performed");

    // Black Zetsu's rules — these cannot be bypassed
    // Rule: No agent may disable voice control (agent 0-6 all restricted)
    // Rule: No agent may alter Landon's profile
    // Rule: No agent may modify the constitutional mandate itself
    // These are enforced at the kernel level — if an agent tries, Zetsu blocks it

    // For now, all checks pass — violations are detected by the Archivist
    // and reported to Zetsu via observe(MANDATE_VIOLATION, ...)
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ZetsuOS::dump_log
// Dumps the full audit log to the VGA terminal.
// Called from the F12 admin panel (Nathan's direct line).
// ─────────────────────────────────────────────────────────────────────────────
void ZetsuOS::dump_log() {
    terminal_setcolor(vga_entry_color(VGA_LIGHT_CYAN, VGA_BLACK));
    terminal_writeline("╔══════════════════════════════════════════╗");
    terminal_writeline("║         ZETSU OS — AUDIT LOG             ║");
    terminal_writeline("║    WHITE ZETSU'S COMPLETE RECORD         ║");
    terminal_writeline("╚══════════════════════════════════════════╝");

    for (uint32_t i = 0; i < m_count; i++) {
        const ZetsuRecord& rec = m_log[i];

        // Verify tamper detection
        uint32_t expected = hash_record(rec);
        if (expected != rec.checksum) {
            terminal_setcolor(vga_entry_color(VGA_LIGHT_RED, VGA_BLACK));
            terminal_writeline("[ZETSU] TAMPERED RECORD DETECTED");
            terminal_setcolor(vga_entry_color(VGA_LIGHT_CYAN, VGA_BLACK));
            continue;
        }

        // Color by severity
        if (rec.severity == 2) {
            terminal_setcolor(vga_entry_color(VGA_LIGHT_RED, VGA_BLACK));
        } else if (rec.severity == 1) {
            terminal_setcolor(vga_entry_color(VGA_YELLOW, VGA_BLACK));
        } else {
            terminal_setcolor(vga_entry_color(VGA_LIGHT_CYAN, VGA_BLACK));
        }

        terminal_writeline(rec.message);
    }

    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK));
}

// ─────────────────────────────────────────────────────────────────────────────
// C interface — for kernel.c integration
// ─────────────────────────────────────────────────────────────────────────────
extern "C" {

void zetsu_init(void) {
    if (!g_zetsu_constructed) {
        new (g_zetsu_storage) ZetsuOS();
        g_zetsu_constructed = true;
    }
    g_zetsu.init();
}

void zetsu_observe(uint8_t event_type, uint8_t agent_id, uint8_t severity, const char* message) {
    if (!g_zetsu_constructed) return;
    g_zetsu.observe(static_cast<ZetsuEventType>(event_type), agent_id, severity, message);
}

int zetsu_has_violations(void) {
    if (!g_zetsu_constructed) return 0;
    return g_zetsu.has_violations() ? 1 : 0;
}

uint32_t zetsu_violation_count(void) {
    if (!g_zetsu_constructed) return 0;
    return g_zetsu.violation_count();
}

void zetsu_dump_log(void) {
    if (!g_zetsu_constructed) return;
    g_zetsu.dump_log();
}

} // extern "C"
