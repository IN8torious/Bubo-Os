// =============================================================================
// BUBO OS — Adaptive Quality Layer Implementation
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================

#include <new>           // freestanding placement new (bare-metal)
#include "lowspec.hpp"
#include "hwdetect.hpp"
#include "vga.h"
#include <stdint.h>

// ── Global quality settings ───────────────────────────────────────────────────
// g_quality is declared as 'extern QualitySettings g_quality' in the header.
// We define it here as a plain object (not a reference) to match that declaration.
static uint8_t quality_storage[sizeof(QualitySettings)];
QualitySettings& g_quality = *reinterpret_cast<QualitySettings*>(quality_storage);

// ── Preset builders ───────────────────────────────────────────────────────────

static void preset_potato(QualitySettings& q) {
    // Absolute minimum — text mode, no animations, no particles
    // Target: < 512MB RAM, no SSE2, software GPU
    q.use_framebuffer      = false;   // VGA text mode only
    q.use_truecolor        = false;
    q.use_antialiasing     = false;
    q.render_scale         = 4;

    q.use_rain_animation   = false;
    q.use_cloud_animation  = false;
    q.use_bike_animation   = false;
    q.animation_fps        = 0;

    q.use_particles        = false;
    q.max_rain_drops       = 0;
    q.max_cloud_count      = 0;

    q.font_cache_kb        = 32;
    q.icon_cache_kb        = 64;
    q.kernel_heap_kb       = 512;

    q.use_simd             = false;
    q.use_multithreading   = false;
    q.scheduler_quantum_ms = 20;

    q.use_lumen            = false;
    q.use_nanite           = false;
    q.lumen_quality        = 0;
    q.nanite_budget_m      = 0;

    // Accessibility ALWAYS on
    q.use_voice_output     = true;
    q.use_eye_tracking     = true;
    q.use_controller       = true;
}

static void preset_low(QualitySettings& q) {
    // Low spec — basic framebuffer, minimal animations
    // Target: 1-2GB RAM, SSE2, Intel HD Graphics
    // This is Landon's minimum guaranteed experience.
    q.use_framebuffer      = true;
    q.use_truecolor        = true;
    q.use_antialiasing     = false;
    q.render_scale         = 2;       // Half resolution rendering

    q.use_rain_animation   = true;    // Rain is on — it's part of the identity
    q.use_cloud_animation  = true;    // Clouds too — they're slow, cheap
    q.use_bike_animation   = false;   // Bike arrival too expensive on low-spec
    q.animation_fps        = 15;      // 15fps animations

    q.use_particles        = true;
    q.max_rain_drops       = 50;      // Minimal rain drops
    q.max_cloud_count      = 2;

    q.font_cache_kb        = 128;
    q.icon_cache_kb        = 256;
    q.kernel_heap_kb       = 2048;

    q.use_simd             = true;    // SSE2 is available on LOW tier
    q.use_multithreading   = false;   // Single-core safe
    q.scheduler_quantum_ms = 15;

    q.use_lumen            = false;
    q.use_nanite           = true;    // Nanite works on integrated GPU
    q.lumen_quality        = 0;
    q.nanite_budget_m      = 4;       // Very conservative Nanite budget

    q.use_voice_output     = true;
    q.use_eye_tracking     = true;
    q.use_controller       = true;
}

static void preset_mid(QualitySettings& q) {
    // Mid spec — full framebuffer, good animations, reduced particles
    // Target: 2-4GB RAM, SSE4, integrated/low discrete GPU
    q.use_framebuffer      = true;
    q.use_truecolor        = true;
    q.use_antialiasing     = false;
    q.render_scale         = 1;       // Native resolution

    q.use_rain_animation   = true;
    q.use_cloud_animation  = true;
    q.use_bike_animation   = true;    // Bike arrival enabled
    q.animation_fps        = 28;

    q.use_particles        = true;
    q.max_rain_drops       = 150;
    q.max_cloud_count      = 4;

    q.font_cache_kb        = 256;
    q.icon_cache_kb        = 512;
    q.kernel_heap_kb       = 4096;

    q.use_simd             = true;
    q.use_multithreading   = true;
    q.scheduler_quantum_ms = 10;

    q.use_lumen            = false;
    q.use_nanite           = true;
    q.lumen_quality        = 0;
    q.nanite_budget_m      = 8;

    q.use_voice_output     = true;
    q.use_eye_tracking     = true;
    q.use_controller       = true;
}

static void preset_high(QualitySettings& q) {
    // High spec — everything on, full quality
    // Target: 4-8GB RAM, AVX, mid discrete GPU
    q.use_framebuffer      = true;
    q.use_truecolor        = true;
    q.use_antialiasing     = true;
    q.render_scale         = 1;

    q.use_rain_animation   = true;
    q.use_cloud_animation  = true;
    q.use_bike_animation   = true;
    q.animation_fps        = 60;

    q.use_particles        = true;
    q.max_rain_drops       = 250;
    q.max_cloud_count      = 6;

    q.font_cache_kb        = 512;
    q.icon_cache_kb        = 1024;
    q.kernel_heap_kb       = 8192;

    q.use_simd             = true;
    q.use_multithreading   = true;
    q.scheduler_quantum_ms = 8;

    q.use_lumen            = true;
    q.use_nanite           = true;
    q.lumen_quality        = 2;       // Medium Lumen
    q.nanite_budget_m      = 16;

    q.use_voice_output     = true;
    q.use_eye_tracking     = true;
    q.use_controller       = true;
}

static void preset_ultra(QualitySettings& q) {
    // Ultra — maximum everything
    // Target: 8GB+ RAM, AVX2, high discrete GPU
    q.use_framebuffer      = true;
    q.use_truecolor        = true;
    q.use_antialiasing     = true;
    q.render_scale         = 1;

    q.use_rain_animation   = true;
    q.use_cloud_animation  = true;
    q.use_bike_animation   = true;
    q.animation_fps        = 60;

    q.use_particles        = true;
    q.max_rain_drops       = 500;
    q.max_cloud_count      = 8;

    q.font_cache_kb        = 1024;
    q.icon_cache_kb        = 2048;
    q.kernel_heap_kb       = 16384;

    q.use_simd             = true;
    q.use_multithreading   = true;
    q.scheduler_quantum_ms = 5;

    q.use_lumen            = true;
    q.use_nanite           = true;
    q.lumen_quality        = 3;       // Full Lumen
    q.nanite_budget_m      = 32;

    q.use_voice_output     = true;
    q.use_eye_tracking     = true;
    q.use_controller       = true;
}

// ── lowspec_init ──────────────────────────────────────────────────────────────
extern "C" void lowspec_init(void) {
    new(quality_storage) QualitySettings();

    switch (g_hw.tier) {
        case HWTier::POTATO: preset_potato(g_quality); break;
        case HWTier::LOW:    preset_low(g_quality);    break;
        case HWTier::MID:    preset_mid(g_quality);    break;
        case HWTier::HIGH:   preset_high(g_quality);   break;
        case HWTier::ULTRA:  preset_ultra(g_quality);  break;
        default:             preset_low(g_quality);    break;
    }

    // VM override — reduce animations in virtual machines
    if (g_hw.is_vm) {
        g_quality.use_bike_animation = false;
        if (g_quality.max_rain_drops > 100) g_quality.max_rain_drops = 100;
        if (g_quality.animation_fps > 28)   g_quality.animation_fps  = 28;
    }
}

// ── lowspec_adjust_for_load ───────────────────────────────────────────────────
extern "C" void lowspec_adjust_for_load(uint32_t cpu_pct, uint32_t gpu_pct) {
    // Dynamic quality reduction under load
    if (cpu_pct > 90 || gpu_pct > 90) {
        // Emergency — cut particles in half
        g_quality.max_rain_drops /= 2;
        if (g_quality.animation_fps > 15) g_quality.animation_fps = 15;
    } else if (cpu_pct > 75 || gpu_pct > 75) {
        // Moderate load — reduce rain
        if (g_quality.max_rain_drops > 100) g_quality.max_rain_drops = 100;
    } else if (cpu_pct < 50 && gpu_pct < 50) {
        // Light load — restore to tier defaults
        lowspec_init();
    }
}

// ── Queries ───────────────────────────────────────────────────────────────────
extern "C" bool     lowspec_use_rain(void)        { return g_quality.use_rain_animation; }
extern "C" bool     lowspec_use_clouds(void)      { return g_quality.use_cloud_animation; }
extern "C" bool     lowspec_use_particles(void)   { return g_quality.use_particles; }
extern "C" bool     lowspec_use_framebuffer(void) { return g_quality.use_framebuffer; }
extern "C" uint16_t lowspec_rain_drop_count(void) { return g_quality.max_rain_drops; }
extern "C" uint8_t  lowspec_animation_fps(void)   { return g_quality.animation_fps; }

// ── Print settings ────────────────────────────────────────────────────────────
extern "C" void lowspec_print_settings(void) {
    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));
    terminal_writeline("  [QUALITY] Adaptive settings loaded:");

    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK));

    if (g_quality.use_framebuffer) {
        terminal_writeline("    Framebuffer: ON");
    } else {
        terminal_writeline("    Framebuffer: OFF (VGA text mode)");
    }

    if (g_quality.use_rain_animation) {
        terminal_writeline("    Rain: ON");
    } else {
        terminal_writeline("    Rain: OFF");
    }

    if (g_quality.use_lumen) {
        terminal_writeline("    Lumen: ON");
    } else {
        terminal_writeline("    Lumen: OFF (low-spec)");
    }

    if (g_quality.use_nanite) {
        terminal_writeline("    Nanite: ON");
    } else {
        terminal_writeline("    Nanite: OFF");
    }

    // Accessibility always on message
    terminal_setcolor(vga_entry_color(VGA_YELLOW, VGA_BLACK));
    terminal_writeline("    Accessibility: ALWAYS ON — voice, eye tracking, controller");
    terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
}
