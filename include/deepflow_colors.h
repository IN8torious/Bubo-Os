// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// include/deepflow_colors.h — The Semantic Color Language
//
// The Matrix runs green. That's the machine's color.
// Deep Flow OS runs RED. That's Nathan's color.
//
// Every color in Deep Flow OS means something specific and consistent.
// This is a visual programming language. You can look at any element
// on the screen and know its state without reading a single word.
//
// - Red = alive, running, present — the code breathes
// - Dark Red = error — serious, being addressed, not alarming
// - Orange = solution forming — warm, decisive, moving
// - Blue = final solution — resolved, at peace, confirmed
// - Teal = new user welcome — safe, guided, possibility
// - Purple = Landon — the one this was built for. Everything yields.
// - Crimson = VASH — watching, protecting
//
// This is the visual identity of the OS. It cannot be changed without
// a constitutional amendment. This is who we are.
// =============================================================================

#ifndef DEEPFLOW_COLORS_H
#define DEEPFLOW_COLORS_H

#include <stdint.h>

// ── 32-bit ARGB color type ────────────────────────────────────────────────────
// Format: 0xAARRGGBB
// Alpha is ignored in framebuffer mode but used in compositing.
typedef uint32_t df_color_t;

// ── Core Red Palette (Nathan's Signature) ─────────────────────────────────────
// The living code stream — bright, electric red. This is the default.
#define DF_RED_STREAM        ((df_color_t)0xFFFF2020)  // Bright code red
#define DF_RED_BRIGHT        ((df_color_t)0xFFFF4444)  // Highlighted text
#define DF_RED_MID           ((df_color_t)0xFFCC1111)  // Normal text
#define DF_RED_DIM           ((df_color_t)0xFF881111)  // Dimmed / inactive text
#define DF_RED_DARK          ((df_color_t)0xFF440808)  // Background glow

// ── User Journey Colors ───────────────────────────────────────────────────────
// The OS watches you grow and changes color with you.
// New User -> Learning -> Familiar -> Sovereign
#define DF_USER_NEW          ((df_color_t)0xFF00CED1)  // Soft teal — welcome, safe
#define DF_USER_LEARNING     ((df_color_t)0xFF20B2AA)  // Light sea green — growing
#define DF_USER_FAMILIAR     ((df_color_t)0xFFFF6600)  // Orange — comfortable
#define DF_USER_SOVEREIGN    ((df_color_t)0xFFFF2020)  // Bright red — mastery
#define DF_USER_LANDON       ((df_color_t)0xFF9D00FF)  // Deep purple — the VIP

// ── Agent Identity Colors ─────────────────────────────────────────────────────
// Each CORVUS agent has its own visual signature. When they collaborate,
// their colors blend in the particle system.
#define DF_AGENT_CORVUS      ((df_color_t)0xFFFF2020)  // Core: Bright red stream
#define DF_AGENT_VASH        ((df_color_t)0xFF6B0F1A)  // Security: Deep crimson
#define DF_AGENT_JIN         ((df_color_t)0xFF8A2BE2)  // Logic: Blue-violet
#define DF_AGENT_MUGEN       ((df_color_t)0xFFFF4500)  // Chaos: Unpredictable orange
#define DF_AGENT_EDWARD      ((df_color_t)0xFFFFBF00)  // Patcher: Amber/alchemist fire
#define DF_AGENT_YODA        ((df_color_t)0xFFDAA520)  // FCN: Deep gold/ancient
#define DF_AGENT_HEAL        ((df_color_t)0xFF0066CC)  // Healer: Landon blue (tomoe 4)

// ── Error Depth Encoding ──────────────────────────────────────────────────────
// Errors are encoded by severity. Not all errors are equal.
#define DF_ERROR_WARNING     ((df_color_t)0xFF8B0000)  // Dark red — static warning
#define DF_ERROR_STANDARD    ((df_color_t)0xFFCC0000)  // Mid red — slow pulse
#define DF_ERROR_CRITICAL    ((df_color_t)0xFFFF0000)  // Bright red — fast strobe
#define DF_ERROR_VETO        ((df_color_t)0xFFFFFFFF)  // White flash -> VASH crimson

// ── Solution Confidence Gradient (The Blue Shift) ─────────────────────────────
// As CORVUS works toward a solution, the color shifts from orange to blue.
#define DF_SOL_THINKING      ((df_color_t)0xFFFF6600)  // Orange — working
#define DF_SOL_FORMING       ((df_color_t)0xFFFFBF00)  // Amber — forming
#define DF_SOL_TESTING       ((df_color_t)0xFFFFD700)  // Gold — testing
#define DF_SOL_VERIFYING     ((df_color_t)0xFF87CEFA)  // Pale blue — verifying
#define DF_SOL_FINAL         ((df_color_t)0xFF1E90FF)  // Deep blue — confirmed/peace

// ── VASH Colors — Deep Crimson ────────────────────────────────────────────────
// VASH pulses in deep crimson when he's watching. Calm but present.
#define DF_VASH_IDLE         ((df_color_t)0xFF6B0F1A)  // Deep crimson — watching
#define DF_VASH_ALERT        ((df_color_t)0xFFAA1122)  // Alert — threat detected
#define DF_VASH_ACTIVE       ((df_color_t)0xFFDD1133)  // Active — healing
#define DF_VASH_CRITICAL     ((df_color_t)0xFFFF1144)  // Critical — last resort
#define DF_VASH_PEACE        ((df_color_t)0xFF3D0010)  // Love and peace — quiet

// ── Ultra Instinct Colors — Orange-Gold ──────────────────────────────────────
// When CORVUS enters DEEP_FLOW and pre-loads a command, it burns orange-gold.
#define DF_ULTRA_PRELOAD     ((df_color_t)0xFFFFAA00)  // Pre-loading — warm gold
#define DF_ULTRA_FIRE        ((df_color_t)0xFFFFCC00)  // Firing — bright gold
#define DF_ULTRA_PULSE       ((df_color_t)0xFFFF8800)  // Pulse — orange-gold
#define DF_ULTRA_TRAIL       ((df_color_t)0xFFFFDD88)  // Trail — pale gold

// ── Flow State Colors ─────────────────────────────────────────────────────────
#define DF_FLOW_HELLO        ((df_color_t)0xFF441111)  // Idle — dark, quiet red
#define DF_FLOW_LISTENING    ((df_color_t)0xFF882222)  // Listening — warming up
#define DF_FLOW_THINKING     ((df_color_t)0xFFCC3333)  // Thinking — FCN active
#define DF_FLOW_ACTING       ((df_color_t)0xFFFF4444)  // Acting — full red
#define DF_FLOW_DEEP         ((df_color_t)0xFFFFAA00)  // Deep Flow — orange-gold
#define DF_FLOW_REPORTING    ((df_color_t)0xFFFF6600)  // Reporting — orange burst

// ── System Health Bar Colors ──────────────────────────────────────────────────
// These are the only non-red/blue/orange colors — they need instant readability.
#define DF_HEALTH_GREEN      ((df_color_t)0xFF00FF88)  // All clear
#define DF_HEALTH_YELLOW     ((df_color_t)0xFFFFD700)  // Tasks running
#define DF_HEALTH_RED        ((df_color_t)0xFFFF3333)  // Threat / error

// ── Background Colors ─────────────────────────────────────────────────────────
#define DF_BG_DEEP           ((df_color_t)0xFF0A0000)  // Near-black with red tint
#define DF_BG_MID            ((df_color_t)0xFF150000)  // Dark background
#define DF_BG_PANEL          ((df_color_t)0xFF200000)  // Panel / card background
#define DF_BG_GLASS          ((df_color_t)0x40200000)  // Frosted glass (semi-transparent)

// ── Terminal / VGA Color Indices ──────────────────────────────────────────────
// Maps the 16 VGA palette entries to the Deep Flow semantic palette.
// Format: { R, G, B } — 6-bit DAC values (0-63)
#define DF_VGA_PALETTE { \
    { 0,  0,  0  }, /* 0: Black background          */ \
    { 34, 2,  2  }, /* 1: Dark red — error           */ \
    { 10, 0,  0  }, /* 2: Deep red — dim text        */ \
    { 44, 8,  8  }, /* 3: Mid red — normal text      */ \
    { 0,  50, 50 }, /* 4: Teal — new user welcome    */ \
    { 27, 1,  0  }, /* 5: VASH crimson               */ \
    { 50, 30, 0  }, /* 6: Orange — solution          */ \
    { 55, 55, 55 }, /* 7: Light gray — UI chrome     */ \
    { 25, 0,  0  }, /* 8: Dark gray with red tint    */ \
    { 63, 8,  8  }, /* 9: Bright red — stream        */ \
    { 38, 0,  63 }, /* A: Purple — Landon            */ \
    { 12, 35, 63 }, /* B: Blue — final solution      */ \
    { 63, 42, 0  }, /* C: Orange-gold — Ultra        */ \
    { 63, 50, 0  }, /* D: Bright gold — UI fire      */ \
    { 63, 63, 63 }, /* E: White — critical alerts    */ \
    { 63, 26, 0  }, /* F: Orange — solution bright   */ \
}

// ── Text colors ───────────────────────────────────────────────────────────────────────────────
#define DF_TEXT_PRIMARY      ((df_color_t)0xFFF0F0F0)  // Cloud white — primary UI text
#define DF_TEXT_SECONDARY    ((df_color_t)0xFFAAAAAA)  // Dim grey — secondary text
#define DF_TEXT_ACCENT       ((df_color_t)0xFFCC2222)  // Akatsuki red — accent text

// ── Vera face constants ────────────────────────────────────────────────────────────────────────────
#define VERA_FACE_BOOT       0
#define VERA_FACE_DESKTOP    1
#define VERA_FACE_FOCUS      2
#define VERA_FACE_ALERT      3
#define VERA_FACE_LANDON     4

// ── Helper macros ────────────────────────────────────────────────────────────────────────────────
#define DF_COLOR_R(c)   (((c) >> 16) & 0xFF)
#define DF_COLOR_G(c)   (((c) >>  8) & 0xFF)
#define DF_COLOR_B(c)   (((c)      ) & 0xFF)
#define DF_COLOR_A(c)   (((c) >> 24) & 0xFF)

// Blend two colors at 50/50 (Used for Agent Collaboration particles)
#define DF_COLOR_BLEND(a, b) ( \
    (((DF_COLOR_R(a) + DF_COLOR_R(b)) / 2) << 16) | \
    (((DF_COLOR_G(a) + DF_COLOR_G(b)) / 2) <<  8) | \
    (((DF_COLOR_B(a) + DF_COLOR_B(b)) / 2)      ) | \
    0xFF000000 \
)

// Calculate color brightness based on CPU load (0.0 to 1.0)
// Base color gets multiplied by a load factor, keeping it within bounds
#define DF_COLOR_APPLY_LOAD(c, load) ( \
    ((uint32_t)(DF_COLOR_R(c) * (0.5f + (load * 0.5f))) << 16) | \
    ((uint32_t)(DF_COLOR_G(c) * (0.5f + (load * 0.5f))) <<  8) | \
    ((uint32_t)(DF_COLOR_B(c) * (0.5f + (load * 0.5f)))      ) | \
    0xFF000000 \
)

// Dim a color by 50%
#define DF_COLOR_DIM(c) ( \
    ((DF_COLOR_R(c) / 2) << 16) | \
    ((DF_COLOR_G(c) / 2) <<  8) | \
    ((DF_COLOR_B(c) / 2)      ) | \
    0xFF000000 \
)

// Get the color for the current flow state
static inline df_color_t df_flow_color(uint32_t flow_state) {
    switch (flow_state) {
    case 0: return DF_FLOW_HELLO;
    case 1: return DF_FLOW_LISTENING;
    case 2: return DF_FLOW_THINKING;
    case 3: return DF_FLOW_ACTING;
    case 4: return DF_FLOW_DEEP;
    case 5: return DF_FLOW_REPORTING;
    default: return DF_RED_MID;
    }
}

// Get the color for the current user journey stage
static inline df_color_t df_user_journey_color(uint32_t stage, int is_landon) {
    if (is_landon) return DF_USER_LANDON;
    switch (stage) {
    case 0: return DF_USER_NEW;        // First boot / Onboarding
    case 1: return DF_USER_LEARNING;   // Basic usage
    case 2: return DF_USER_FAMILIAR;   // Standard usage
    case 3: return DF_USER_SOVEREIGN;  // Mastery / Full Red
    default: return DF_USER_SOVEREIGN;
    }
}

#endif // DEEPFLOW_COLORS_H
