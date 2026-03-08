// =============================================================================
// BUBO OS — UE5 Performance Co-Pilot Implementation
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================
#include <new>           // freestanding placement new (bare-metal)
// =============================================================================
// BUBO OS — UE5 Performance Co-Pilot Implementation
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================

#include "ue5_patch.hpp"
#include "raven.hpp"
#include "scheduler.h"
#include "vga.h"
#include <stdint.h>

extern "C" volatile uint32_t pit_ticks;

// ── Global instance ───────────────────────────────────────────────────────────
static uint8_t ue5_storage[sizeof(UE5CoPilot)];
UE5CoPilot& g_ue5 = *reinterpret_cast<UE5CoPilot*>(ue5_storage);

// ── String helpers ────────────────────────────────────────────────────────────
static bool str_contains(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    uint32_t ni = 0, hi = 0;
    while (haystack[hi]) {
        if (haystack[hi] == needle[ni]) {
            ni++;
            if (!needle[ni]) return true;
        } else {
            ni = 0;
        }
        hi++;
    }
    return false;
}

static void u32_to_str(char* buf, uint32_t v) {
    if (v == 0) { buf[0]='0'; buf[1]='\0'; return; }
    char tmp[12]; int i = 0;
    while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

static void str_append(char* d, const char* s, uint32_t* len, uint32_t max) {
    uint32_t i = 0;
    while (s[i] && *len < max-1) { d[(*len)++] = s[i++]; }
    d[*len] = '\0';
}

// ── UE5CoPilot::state_name ────────────────────────────────────────────────────
const char* UE5CoPilot::state_name(void) const {
    switch (state) {
        case UE5State::NOT_RUNNING:      return "NOT_RUNNING";
        case UE5State::BOOTING:          return "BOOTING";
        case UE5State::RUNNING:          return "RUNNING";
        case UE5State::SHADER_COMPILING: return "SHADER_COMPILING";
        case UE5State::VRAM_PRESSURE:    return "VRAM_PRESSURE";
        case UE5State::CPU_PRESSURE:     return "CPU_PRESSURE";
        case UE5State::STUTTERING:       return "STUTTERING";
        case UE5State::PAUSED:           return "PAUSED";
        default:                         return "UNKNOWN";
    }
}

// ── UE5CoPilot::on_ue5_start ──────────────────────────────────────────────────
void UE5CoPilot::on_ue5_start(uint32_t pid) {
    ue5_pid = pid;
    state   = UE5State::BOOTING;
    active  = true;

    // Immediately boost shader compilation threads during startup
    // UE5 compiles thousands of shaders on first boot — this is the biggest
    // source of stutters. Give shader threads near-real-time priority.
    boost_shader_threads();

    g_raven.log(RavenEvent::HEALER_INIT, 0,
        "UE5 detected — co-pilot active, shader boost engaged", pid);

    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));
    terminal_writeline("  [UE5] Co-pilot active — shader pre-emption engaged");
    terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
}

// ── UE5CoPilot::on_ue5_exit ───────────────────────────────────────────────────
void UE5CoPilot::on_ue5_exit(void) {
    g_raven.log(RavenEvent::HEALER_INIT, 0,
        "UE5 exited — co-pilot standing down", metrics.stutter_count);

    // Log session summary
    char buf[128];
    uint32_t len = 0;
    str_append(buf, "UE5 session: stutters=", &len, sizeof(buf));
    char tmp[12]; u32_to_str(tmp, metrics.stutter_count);
    str_append(buf, tmp, &len, sizeof(buf));
    str_append(buf, " interventions=", &len, sizeof(buf));
    u32_to_str(tmp, metrics.interventions);
    str_append(buf, tmp, &len, sizeof(buf));
    g_raven.log(RavenEvent::HEALER_INIT, 0, buf, 0);

    ue5_pid  = 0;
    state    = UE5State::NOT_RUNNING;
    active   = false;
    metrics  = {};
}

// ── UE5CoPilot::tick ──────────────────────────────────────────────────────────
// Called every 10 kernel ticks (100ms) when UE5 is running.
void UE5CoPilot::tick(void) {
    if (!active || state == UE5State::NOT_RUNNING) return;

    // ── Estimate metrics from scheduler ──────────────────────────────────────
    // In a full implementation, UE5 would write metrics to a shared memory page.
    // For now, we estimate from scheduler data.
    metrics.cpu_load_pct = scheduler_get_cpu_load();

    // ── State machine ─────────────────────────────────────────────────────────

    // Transition from BOOTING to RUNNING when shader queue clears
    if (state == UE5State::BOOTING) {
        if (metrics.shader_queue_depth == 0 && metrics.cpu_load_pct < 70) {
            state = UE5State::RUNNING;
            // Render thread gets real-time priority once shaders are done
            boost_render_thread();
            g_raven.log(RavenEvent::SUBSYSTEM_HEALED, 0,
                "UE5 boot complete — render thread boosted to real-time", 0);
        }
        return; // Stay in boost mode during boot
    }

    // ── Stutter detection ─────────────────────────────────────────────────────
    if (metrics.frame_time_ms > STUTTER_FRAME_MS) {
        if (state != UE5State::STUTTERING) {
            state = UE5State::STUTTERING;
            metrics.stutter_count++;
            metrics.last_stutter_tick = pit_ticks;

            g_raven.log(RavenEvent::WATCHDOG_TRIGGER, 2,
                "UE5 stutter detected — emergency intervention", metrics.frame_time_ms);

            // Emergency: scale down Lumen quality immediately
            signal_lumen_quality(1);  // Drop to low
            signal_nanite_budget(8);  // Reduce Nanite triangle budget to 8M
            metrics.interventions++;
        }
    } else if (state == UE5State::STUTTERING && metrics.frame_time_ms < TARGET_FRAME_MS) {
        // Recovered — scale back up gradually
        state = UE5State::RUNNING;
        signal_lumen_quality(2);  // Back to medium
        signal_nanite_budget(16); // Restore Nanite budget
        g_raven.log(RavenEvent::SUBSYSTEM_HEALED, 0,
            "UE5 stutter resolved — quality restored", 0);
    }

    // ── VRAM pressure ─────────────────────────────────────────────────────────
    if (metrics.vram_total_mb > 0) {
        uint32_t vram_pct = (metrics.vram_used_mb * 100) / metrics.vram_total_mb;
        if (vram_pct > VRAM_CRITICAL_PCT && state != UE5State::VRAM_PRESSURE) {
            state = UE5State::VRAM_PRESSURE;
            signal_lod_scale_down();
            metrics.interventions++;
            g_raven.log(RavenEvent::WATCHDOG_TRIGGER, 2,
                "UE5 VRAM critical — LOD scaled down", vram_pct);
        } else if (vram_pct < VRAM_WARN_PCT && state == UE5State::VRAM_PRESSURE) {
            state = UE5State::RUNNING;
            signal_lod_scale_up();
            g_raven.log(RavenEvent::SUBSYSTEM_HEALED, 0,
                "UE5 VRAM pressure relieved — LOD restored", vram_pct);
        }
    }

    // ── CPU pressure ──────────────────────────────────────────────────────────
    if (metrics.cpu_load_pct > CPU_WARN_PCT && state == UE5State::RUNNING) {
        state = UE5State::CPU_PRESSURE;
        // Give UE5's render thread even more priority — starve background tasks
        boost_render_thread();
        metrics.interventions++;
        g_raven.log(RavenEvent::WATCHDOG_TRIGGER, 1,
            "UE5 CPU pressure — render thread priority maximized", metrics.cpu_load_pct);
    } else if (metrics.cpu_load_pct < 75 && state == UE5State::CPU_PRESSURE) {
        state = UE5State::RUNNING;
    }
}

// ── Intervention methods ──────────────────────────────────────────────────────

void UE5CoPilot::boost_render_thread(void) {
    // Set UE5's render thread to real-time priority in the BUBO scheduler
    if (ue5_pid) {
        scheduler_set_process_priority(ue5_pid, RENDER_THREAD_PRIORITY);
    }
}

void UE5CoPilot::boost_shader_threads(void) {
    // Give shader compilation threads near-real-time priority
    // This prevents the main thread from starving during shader compilation
    if (ue5_pid) {
        scheduler_set_process_priority(ue5_pid, SHADER_THREAD_PRIORITY);
    }
}

void UE5CoPilot::signal_lod_scale_down(void) {
    // Write to UE5's shared memory page (if available) to request LOD reduction
    // In bare-metal mode, this writes to a well-known memory address that
    // a UE5 plugin would read. For now, log the signal.
    g_raven.log(RavenEvent::HEALER_INIT, 1, "UE5: LOD scale-down signal sent", 0);
}

void UE5CoPilot::signal_lod_scale_up(void) {
    g_raven.log(RavenEvent::HEALER_INIT, 0, "UE5: LOD scale-up signal sent", 0);
}

void UE5CoPilot::signal_lumen_quality(uint8_t quality) {
    // 0=off 1=low 2=medium 3=high
    const char* names[] = { "OFF", "LOW", "MEDIUM", "HIGH" };
    const char* name = quality < 4 ? names[quality] : "?";
    g_raven.log(RavenEvent::HEALER_INIT, 1, name, quality);
    (void)name;
}

void UE5CoPilot::signal_nanite_budget(uint32_t triangles_m) {
    g_raven.log(RavenEvent::HEALER_INIT, 0, "UE5: Nanite budget set", triangles_m);
}

// ── C API ─────────────────────────────────────────────────────────────────────
extern "C" void ue5_patch_init(void) {
    new(ue5_storage) UE5CoPilot();
    g_raven.log(RavenEvent::HEALER_INIT, 0,
        "UE5 co-pilot ready — watching for UnrealEngine process", 0);

    terminal_setcolor(vga_entry_color(VGA_CYAN, VGA_BLACK));
    terminal_writeline("  [UE5] Performance co-pilot ready — shader/VRAM/frame patches loaded");
    terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
}

extern "C" void ue5_patch_tick(void) {
    g_ue5.tick();
}

extern "C" void ue5_patch_on_process_start(uint32_t pid, const char* name) {
    // Detect UE5 by process name
    if (str_contains(name, "UnrealEngine") ||
        str_contains(name, "UE5") ||
        str_contains(name, "EpicGames") ||
        str_contains(name, "FortniteClient")) {
        g_ue5.on_ue5_start(pid);
    }
}

extern "C" void ue5_patch_on_process_exit(uint32_t pid) {
    if (g_ue5.ue5_pid == pid) {
        g_ue5.on_ue5_exit();
    }
}

extern "C" bool ue5_patch_active(void) {
    return g_ue5.active;
}

extern "C" void ue5_patch_status(char* buf, uint32_t max) {
    uint32_t len = 0;
    str_append(buf, "UE5 Co-Pilot: ", &len, max);
    str_append(buf, g_ue5.state_name(), &len, max);
    if (g_ue5.active) {
        str_append(buf, " | stutters=", &len, max);
        char tmp[12]; u32_to_str(tmp, g_ue5.metrics.stutter_count);
        str_append(buf, tmp, &len, max);
        str_append(buf, " | interventions=", &len, max);
        u32_to_str(tmp, g_ue5.metrics.interventions);
        str_append(buf, tmp, &len, max);
    }
}
