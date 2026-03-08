// =============================================================================
// BUBO OS — Self-Healing Layer
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// Bare-metal C++ — freestanding, no stdlib, no exceptions, no RTTI.
// Every subsystem is wrapped. Nothing fails silently.
// If it breaks, it heals. If it can't heal, it isolates and continues.
//
// Inspired by Toshiki Kamishima — healed, scarred, still here.
// =============================================================================
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <new>   // BUBO OS freestanding placement new (include/new)

// ── Error codes ───────────────────────────────────────────────────────────────
enum class HealError : uint32_t {
    OK              = 0,
    NOT_FOUND       = 1,   // Hardware not detected
    INIT_FAILED     = 2,   // Initialization failed
    TIMEOUT         = 3,   // Operation timed out
    BAD_ADDRESS     = 4,   // Invalid memory address
    NO_MULTIBOOT    = 5,   // Multiboot2 tag missing
    FALLBACK_ACTIVE = 6,   // Running in fallback mode (degraded but alive)
    FATAL           = 7,   // Cannot recover — subsystem isolated
    ALREADY_HEALED  = 8,   // Heal was called but system was already OK
};

// ── Result<T> — every subsystem call returns this ─────────────────────────────
// No exceptions. No silent failures. Either you have a value or you have an error.
template<typename T>
class Result {
public:
    T        value;
    HealError error;
    bool     ok;

    // Success
    static Result<T> success(T v) {
        Result<T> r;
        r.value = v;
        r.error = HealError::OK;
        r.ok    = true;
        return r;
    }

    // Failure
    static Result<T> fail(HealError e) {
        Result<T> r;
        r.error = e;
        r.ok    = false;
        return r;
    }

    // Implicit bool — if (result) { ... }
    explicit operator bool() const { return ok; }
};

// Specialization for void results (just success/fail, no value)
template<>
class Result<void> {
public:
    HealError error;
    bool      ok;

    static Result<void> success() {
        Result<void> r;
        r.error = HealError::OK;
        r.ok    = true;
        return r;
    }

    static Result<void> fail(HealError e) {
        Result<void> r;
        r.error = e;
        r.ok    = false;
        return r;
    }

    explicit operator bool() const { return ok; }
};

// ── Healing strategy ──────────────────────────────────────────────────────────
enum class HealStrategy {
    RETRY,      // Try again up to N times
    FALLBACK,   // Switch to a degraded fallback mode
    ISOLATE,    // Disable the subsystem, keep everything else running
    REBOOT,     // Last resort — trigger a controlled reboot
};

// ── Subsystem state ───────────────────────────────────────────────────────────
enum class SubsystemState {
    UNINITIALIZED,
    HEALTHY,
    DEGRADED,   // Running in fallback mode
    ISOLATED,   // Disabled — too broken to run
    HEALING,    // Currently attempting recovery
};

// ── SubsystemBase — every kernel subsystem inherits from this ─────────────────
class SubsystemBase {
public:
    const char*    name;
    SubsystemState state;
    uint32_t       fail_count;
    uint32_t       heal_count;
    HealError      last_error;

    SubsystemBase(const char* n)
        : name(n)
        , state(SubsystemState::UNINITIALIZED)
        , fail_count(0)
        , heal_count(0)
        , last_error(HealError::OK)
    {}

    // Override these in each subsystem
    virtual Result<void> init()    = 0;  // Primary initialization
    virtual Result<void> fallback() = 0; // Degraded mode initialization
    virtual Result<void> check()   = 0;  // Health check — called periodically
    virtual void         isolate() = 0;  // Called when subsystem is disabled

    bool is_healthy()  const { return state == SubsystemState::HEALTHY; }
    bool is_degraded() const { return state == SubsystemState::DEGRADED; }
    bool is_alive()    const { return state == SubsystemState::HEALTHY ||
                                      state == SubsystemState::DEGRADED; }
    bool is_isolated() const { return state == SubsystemState::ISOLATED; }
};

// ── HealerCore — the self-healing orchestrator ────────────────────────────────
// Manages up to 16 subsystems. Wraps init, monitors health, triggers recovery.
class HealerCore {
public:
    static constexpr uint32_t MAX_SUBSYSTEMS = 16;
    static constexpr uint32_t MAX_RETRIES    = 3;

    // Ring buffer for heal events (no dynamic allocation)
    struct HealEvent {
        const char* subsystem;
        HealError   error;
        HealStrategy strategy;
        bool        recovered;
        uint32_t    timestamp_ms;  // from PIT tick counter
    };

    static constexpr uint32_t EVENT_RING_SIZE = 64;
    HealEvent   events[EVENT_RING_SIZE];
    uint32_t    event_head = 0;
    uint32_t    event_count = 0;

    SubsystemBase* subsystems[MAX_SUBSYSTEMS];
    uint32_t       subsystem_count = 0;

    // ── Register a subsystem ──────────────────────────────────────────────────
    void register_subsystem(SubsystemBase* s) {
        if (subsystem_count < MAX_SUBSYSTEMS) {
            subsystems[subsystem_count++] = s;
        }
    }

    // ── Boot a subsystem with automatic healing ───────────────────────────────
    // Returns true if the subsystem is alive (healthy or degraded)
    // Returns false only if it had to be isolated
    bool boot(SubsystemBase* s) {
        s->state = SubsystemState::HEALING;

        // Try primary init up to MAX_RETRIES times
        for (uint32_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
            auto r = s->init();
            if (r) {
                s->state = SubsystemState::HEALTHY;
                return true;
            }
            s->fail_count++;
            s->last_error = r.error;
        }

        // Primary failed — try fallback
        auto fb_r = s->fallback();
        if (fb_r) {
            s->state = SubsystemState::DEGRADED;
            s->heal_count++;
            log_event(s->name, s->last_error, HealStrategy::FALLBACK, true);
            return true;
        }

        // Both failed — isolate
        s->state = SubsystemState::ISOLATED;
        s->isolate();
        log_event(s->name, s->last_error, HealStrategy::ISOLATE, false);
        return false;
    }

    // ── Heal a specific subsystem ─────────────────────────────────────────────
    bool heal(SubsystemBase* s) {
        if (s->state == SubsystemState::ISOLATED) return false;
        if (s->state == SubsystemState::HEALTHY)  return true;

        s->state = SubsystemState::HEALING;
        s->heal_count++;

        auto r = s->init();
        if (r) {
            s->state = SubsystemState::HEALTHY;
            log_event(s->name, HealError::OK, HealStrategy::RETRY, true);
            return true;
        }

        auto fb_r = s->fallback();
        if (fb_r) {
            s->state = SubsystemState::DEGRADED;
            log_event(s->name, s->last_error, HealStrategy::FALLBACK, true);
            return true;
        }

        s->state = SubsystemState::ISOLATED;
        s->isolate();
        log_event(s->name, s->last_error, HealStrategy::ISOLATE, false);
        return false;
    }

    // ── Periodic health check — call from the PIT timer interrupt ─────────────
    void tick_check() {
        for (uint32_t i = 0; i < subsystem_count; i++) {
            SubsystemBase* s = subsystems[i];
            if (!s->is_alive()) continue;

            auto r = s->check();
            if (!r) {
                s->fail_count++;
                s->last_error = r.error;
                // Auto-heal if it's gone unhealthy
                if (s->state == SubsystemState::HEALTHY) {
                    s->state = SubsystemState::DEGRADED;
                    heal(s);
                }
            }
        }
    }

    // ── Status report — written to VGA text for boot screen ──────────────────
    uint32_t healthy_count()  const {
        uint32_t n = 0;
        for (uint32_t i = 0; i < subsystem_count; i++)
            if (subsystems[i]->is_healthy()) n++;
        return n;
    }

    uint32_t degraded_count() const {
        uint32_t n = 0;
        for (uint32_t i = 0; i < subsystem_count; i++)
            if (subsystems[i]->is_degraded()) n++;
        return n;
    }

    uint32_t isolated_count() const {
        uint32_t n = 0;
        for (uint32_t i = 0; i < subsystem_count; i++)
            if (subsystems[i]->is_isolated()) n++;
        return n;
    }

private:
    void log_event(const char* name, HealError err, HealStrategy strat, bool recovered) {
        HealEvent& e = events[event_head % EVENT_RING_SIZE];
        e.subsystem  = name;
        e.error      = err;
        e.strategy   = strat;
        e.recovered  = recovered;
        e.timestamp_ms = 0;  // TODO: wire to PIT tick counter
        event_head++;
        if (event_count < EVENT_RING_SIZE) event_count++;
    }
};

// ── Global healer instance ────────────────────────────────────────────────────
// Declared here, defined in healer.cpp
extern HealerCore g_healer;

// ── Forward declarations for framebuffer/sound/net APIs ──────────────────────
// These are implemented in their respective .c files
extern "C" bool fb_init_from_multiboot(uint32_t mb2_info_addr);
extern "C" void fb_init_fallback(void);
extern "C" bool fb_is_ready(void);
extern "C" void terminal_writeline(const char* s);
extern "C" bool sound_init(void);
extern "C" bool net_init(void);

// ── Concrete subsystem classes ────────────────────────────────────────────────
// Defined here (not in healer.cpp) so admin_panel.cpp and raven.cpp can use
// the full type (not just a forward declaration).

class FramebufferSubsystem : public SubsystemBase {
public:
    uint32_t mb2_info_addr;
    FramebufferSubsystem() : SubsystemBase("Framebuffer"), mb2_info_addr(0) {}
    Result<void> init() override {
        if (!mb2_info_addr)
            return Result<void>::fail(HealError::NO_MULTIBOOT);
        bool ok = fb_init_from_multiboot(mb2_info_addr);
        if (!ok)
            return Result<void>::fail(HealError::NOT_FOUND);
        return Result<void>::success();
    }
    Result<void> fallback() override {
        fb_init_fallback();
        return Result<void>::success();
    }
    Result<void> check() override {
        if (!fb_is_ready())
            return Result<void>::fail(HealError::INIT_FAILED);
        return Result<void>::success();
    }
    void isolate() override {
        terminal_writeline("  [HEAL ] Framebuffer isolated — VGA text mode active");
    }
};

class SoundSubsystem : public SubsystemBase {
public:
    SoundSubsystem() : SubsystemBase("Sound") {}
    Result<void> init() override {
        bool ok = sound_init();
        if (!ok)
            return Result<void>::fail(HealError::NOT_FOUND);
        return Result<void>::success();
    }
    Result<void> fallback() override {
        terminal_writeline("  [HEAL ] Sound: PC speaker fallback active");
        return Result<void>::success();
    }
    Result<void> check() override { return Result<void>::success(); }
    void isolate() override {
        terminal_writeline("  [HEAL ] Sound isolated — silent mode active");
    }
};

class NetworkSubsystem : public SubsystemBase {
public:
    NetworkSubsystem() : SubsystemBase("Network") {}
    Result<void> init() override {
        bool ok = net_init();
        if (!ok)
            return Result<void>::fail(HealError::NOT_FOUND);
        return Result<void>::success();
    }
    Result<void> fallback() override {
        terminal_writeline("  [HEAL ] Network: offline mode active");
        return Result<void>::success();
    }
    Result<void> check() override { return Result<void>::success(); }
    void isolate() override {
        terminal_writeline("  [HEAL ] Network isolated — offline mode");
    }
};

// ── Global subsystem pointers ─────────────────────────────────────────────────
// Defined in healer.cpp, used by admin_panel.cpp and raven.cpp
extern FramebufferSubsystem* g_fb_subsystem;
extern SoundSubsystem*       g_snd_subsystem;
extern NetworkSubsystem*     g_net_subsystem;
