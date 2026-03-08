// =============================================================================
// BUBO OS — Adaptive Quality Layer (Low-Spec Optimization)
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// Same ISO. Same kernel. Any machine.
//
// This layer reads the HardwareProfile from hwdetect and sets quality flags
// that every other subsystem checks before doing expensive work.
//
// The principle: degrade gracefully, never fail hard.
// A POTATO machine gets a working OS. A LOW machine gets a good OS.
// A HIGH machine gets the full experience.
//
// Quality tiers:
//   POTATO  — text mode only, no animations, minimal memory use
//   LOW     — basic framebuffer, simple UI, no particles, reduced colors
//   MID     — full framebuffer, basic animations, reduced particle count
//   HIGH    — full UI, all animations, full particle system
//   ULTRA   — everything, RTX voxel effects, maximum quality
// =============================================================================
#pragma once
#include "hwdetect.hpp"
#include <stdint.h>
#include <stdbool.h>

// ── Quality settings ──────────────────────────────────────────────────────────
struct QualitySettings {
    // Rendering
    bool   use_framebuffer;      // false = VGA text mode only
    bool   use_truecolor;        // false = 256-color palette
    bool   use_antialiasing;     // false = no AA
    uint8_t render_scale;        // 1=native, 2=half, 4=quarter resolution

    // Animations
    bool   use_rain_animation;   // AKIRA rain effect
    bool   use_cloud_animation;  // Floating Akatsuki clouds
    bool   use_bike_animation;   // Kaneda bike arrival
    uint8_t animation_fps;       // Target FPS for animations (8/15/28/60)

    // Particles
    bool     use_particles;      // Particle system enabled
    uint16_t max_rain_drops;     // Rain drop count (0/50/150/250/500)
    uint16_t max_cloud_count;    // Cloud count (0/2/4/8)

    // Memory
    uint32_t font_cache_kb;      // Font cache size in KB
    uint32_t icon_cache_kb;      // Icon cache size in KB
    uint32_t kernel_heap_kb;     // Kernel heap size in KB

    // CPU
    bool   use_simd;             // Use SSE/AVX for rendering
    bool   use_multithreading;   // Use multiple cores
    uint8_t scheduler_quantum_ms; // Scheduler time slice in ms

    // Features
    bool   use_lumen;            // UE5 Lumen (requires HIGH+)
    bool   use_nanite;           // UE5 Nanite (requires MID+)
    uint8_t lumen_quality;       // 0=off 1=low 2=med 3=high
    uint32_t nanite_budget_m;    // Nanite triangle budget in millions

    // Accessibility (always on regardless of tier)
    bool   use_voice_output;     // Dysarthria voice system
    bool   use_eye_tracking;     // Rinnegan eye tracking
    bool   use_controller;       // Adaptive controller
};

// ── Global quality settings ───────────────────────────────────────────────────
// Declared as a reference to a static storage buffer in lowspec.cpp
extern QualitySettings& g_quality;

// ── C API ─────────────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

// Initialize quality settings from hardware profile
// Call after hwdetect_init()
void lowspec_init(void);

// Dynamic adjustment — call when load changes
void lowspec_adjust_for_load(uint32_t cpu_pct, uint32_t gpu_pct);

// Queries
bool lowspec_use_rain(void);
bool lowspec_use_clouds(void);
bool lowspec_use_particles(void);
bool lowspec_use_framebuffer(void);
uint16_t lowspec_rain_drop_count(void);
uint8_t  lowspec_animation_fps(void);

// Print quality settings to terminal
void lowspec_print_settings(void);

#ifdef __cplusplus
}
#endif
