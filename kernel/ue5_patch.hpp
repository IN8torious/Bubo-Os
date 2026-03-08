// =============================================================================
// BUBO OS — UE5 Performance Co-Pilot
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// BUBO OS patches Unreal Engine 5's known performance problems at the kernel
// level — before they ever reach the game. BUBO becomes a better host for
// UE5 than Windows is.
//
// Problems patched:
//   1. Shader compilation stutters     → Pre-empt shader threads, dedicated CPU time
//   2. Lumen/Nanite VRAM pressure      → VRAM watchdog, LOD signal before stutter
//   3. Frame rate drops                → Render thread gets real-time priority
//   4. High system requirements        → Dynamic feature scaling via CPU/GPU load
//   5. Performance event logging       → RAVEN captures every stutter with full context
//
// How it works:
//   UE5 runs as a process in BUBO OS user space (Ring 3).
//   The UE5 co-pilot runs in kernel space (Ring 0).
//   The co-pilot watches UE5's process via the scheduler and memory manager.
//   When it detects a problem forming, it intervenes BEFORE the stutter hits.
//
// This is the same principle as the healer — proactive, not reactive.
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── UE5 process state ─────────────────────────────────────────────────────────
enum class UE5State : uint8_t {
    NOT_RUNNING   = 0,
    BOOTING       = 1,   // UE5 is starting up, shader pre-compilation active
    RUNNING       = 2,   // Normal gameplay
    SHADER_COMPILING = 3, // Shader compilation in progress — needs pre-emption
    VRAM_PRESSURE = 4,   // VRAM > 85% — LOD scaling needed
    CPU_PRESSURE  = 5,   // CPU > 90% — feature scaling needed
    STUTTERING    = 6,   // Frame time > 33ms (< 30fps) — emergency intervention
    PAUSED        = 7,
};

// ── UE5 performance metrics ───────────────────────────────────────────────────
struct UE5Metrics {
    uint32_t frame_time_ms;       // Last frame time in milliseconds
    uint32_t shader_queue_depth;  // Number of shaders pending compilation
    uint32_t vram_used_mb;        // VRAM in use (estimated from process memory)
    uint32_t vram_total_mb;       // Total VRAM available
    uint32_t cpu_load_pct;        // CPU load percentage (0-100)
    uint32_t gpu_load_pct;        // GPU load percentage (estimated)
    uint32_t stutter_count;       // Total stutters detected this session
    uint32_t interventions;       // Total kernel interventions this session
    uint64_t last_stutter_tick;   // PIT tick of last detected stutter
};

// ── UE5 co-pilot ─────────────────────────────────────────────────────────────
class UE5CoPilot {
public:
    static constexpr uint32_t SHADER_THREAD_PRIORITY = 90;  // Near real-time
    static constexpr uint32_t RENDER_THREAD_PRIORITY = 95;  // Real-time
    static constexpr uint32_t VRAM_WARN_PCT          = 85;  // Warn at 85%
    static constexpr uint32_t VRAM_CRITICAL_PCT      = 92;  // Scale at 92%
    static constexpr uint32_t CPU_WARN_PCT           = 85;
    static constexpr uint32_t TARGET_FRAME_MS        = 16;  // 60fps target
    static constexpr uint32_t STUTTER_FRAME_MS       = 33;  // 30fps threshold

    UE5State  state       = UE5State::NOT_RUNNING;
    UE5Metrics metrics    = {};
    uint32_t  ue5_pid     = 0;    // Process ID of UE5 when running
    bool      active      = false;

    // Called every kernel tick when UE5 is running
    void tick(void);

    // Called when UE5 process is detected
    void on_ue5_start(uint32_t pid);

    // Called when UE5 process exits
    void on_ue5_exit(void);

    // Interventions
    void boost_render_thread(void);
    void boost_shader_threads(void);
    void signal_lod_scale_down(void);
    void signal_lod_scale_up(void);
    void signal_lumen_quality(uint8_t quality);  // 0=off 1=low 2=med 3=high
    void signal_nanite_budget(uint32_t triangles_m); // millions

    // Status
    bool is_stuttering(void) const { return state == UE5State::STUTTERING; }
    bool has_vram_pressure(void) const { return state == UE5State::VRAM_PRESSURE; }
    const char* state_name(void) const;
};

// ── Global UE5 co-pilot instance ──────────────────────────────────────────────
extern UE5CoPilot& g_ue5;

// ── C API ─────────────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

void ue5_patch_init(void);
void ue5_patch_tick(void);
void ue5_patch_on_process_start(uint32_t pid, const char* name);
void ue5_patch_on_process_exit(uint32_t pid);
bool ue5_patch_active(void);
void ue5_patch_status(char* buf, uint32_t max);

#ifdef __cplusplus
}
#endif
