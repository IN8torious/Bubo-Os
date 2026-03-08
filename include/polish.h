// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

#pragma once
// =============================================================================
// Deep Flow OS — Visual Polish Layer Header
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

// ── Akatsuki color palette ────────────────────────────────────────────────────
#define POLISH_COLOR_CRIMSON     0xFFCC0020   // Deep Akatsuki crimson
#define POLISH_COLOR_BLOOD       0xFF8B0000   // Dark blood red
#define POLISH_COLOR_VOID        0xFF000000   // Pure void black
#define POLISH_COLOR_DEEP_SPACE  0xFF050008   // Near-black with purple tint
#define POLISH_COLOR_RINNEGAN    0xFF4B0082   // Rinnegan purple
#define POLISH_COLOR_CLOUD_RED   0xFFB22222   // Cloud band red
#define POLISH_COLOR_FROST_DARK  0xCC0A0015   // Frosted glass dark tint
#define POLISH_COLOR_FROST_MID   0xAA100020   // Frosted glass mid tint
#define POLISH_COLOR_WHITE_DIM   0x33FFFFFF   // Subtle white highlight
#define POLISH_COLOR_GOLD        0xFFFFD700   // CORVUS gold accent

// ── Core compositing ──────────────────────────────────────────────────────────
uint32_t polish_alpha_blend(uint32_t dst, uint32_t src, uint8_t alpha);
void     polish_fill_rect_alpha(int32_t x, int32_t y, int32_t w, int32_t h,
                                 uint32_t color, uint8_t alpha);

// ── Surface effects ───────────────────────────────────────────────────────────
void polish_frosted_glass(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t tint, uint8_t tint_alpha, uint8_t blur_passes);
void polish_gradient_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t color_start, uint32_t color_end, bool horizontal);
void polish_drop_shadow(int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t shadow_color, uint8_t radius);
void polish_glow_border(int32_t x, int32_t y, int32_t w, int32_t h,
                         uint32_t glow_color, uint8_t glow_radius);
void polish_bubble(int32_t x, int32_t y, int32_t w, int32_t h,
                    uint32_t color, uint8_t alpha, uint8_t radius);

// ── Desktop components ────────────────────────────────────────────────────────
void polish_titlebar(int32_t x, int32_t y, int32_t w, int32_t h, bool active);
void polish_corvus_bar(int32_t x, int32_t y, int32_t w, int32_t h);
void polish_taskbar_icon(int32_t x, int32_t y, int32_t size,
                          uint32_t icon_color, bool active, bool hovered);
void polish_landon_strip(int32_t x, int32_t y, int32_t w, int32_t h);
void polish_agent_bar(int32_t x, int32_t y, int32_t w, int32_t h,
                       uint8_t load_percent);

// ── Animations ────────────────────────────────────────────────────────────────
void polish_particles_init(void);
void polish_particles_tick(void);
void polish_pulse_trigger(void);
void polish_pulse_tick(int32_t x, int32_t y, int32_t w, int32_t h);
void polish_cursor_trail_update(int32_t cx, int32_t cy);
void polish_cursor_trail_draw(void);

// ── Composite glass panels ──────────────────────────────────────────────────
// Convenience wrapper: frosted glass rect with a single tint color and alpha.
// Used by corvus_home, settings_app, corvus_search, live_mode.
void polish_glass_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                        uint32_t tint, uint8_t alpha);

// Full-screen vignette darkening effect (used by boot_cinematic).
// sw/sh = screen width/height, intensity = 0 (none) to 255 (full black edges).
void polish_vignette(int32_t sw, int32_t sh, uint8_t intensity);

// ── Init ──────────────────────────────────────────────────────────────────────
void polish_init(void);
