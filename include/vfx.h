// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/vfx.h — Visual effects definitions and functions
// =============================================================================

#pragma once

#include <stdint.h>

// ARGB color structure
typedef struct {
    uint8_t a; // Alpha component (0 = transparent, 255 = opaque)
    uint8_t r; // Red component
    uint8_t g; // Green component
    uint8_t b; // Blue component
} vfx_color_t;

// Rectangle structure
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} vfx_rect_t;

// Blends a foreground color onto a background color using alpha blending.
// Resulting color is (fg * alpha + bg * (255 - alpha)) / 255 for each component.
static inline vfx_color_t vfx_blend_alpha(vfx_color_t fg, vfx_color_t bg) {
    vfx_color_t result;
    uint16_t alpha_fg = fg.a;
    uint16_t alpha_bg = bg.a;

    // Calculate new alpha
    result.a = (uint8_t)(((alpha_fg * 255) + (alpha_bg * (255 - alpha_fg))) / 255);
    if (result.a == 0) return (vfx_color_t){0,0,0,0}; // Fully transparent

    // Blend color components
    result.r = (uint8_t)(((fg.r * alpha_fg) + (bg.r * (255 - alpha_fg))) / 255);
    result.g = (uint8_t)(((fg.g * alpha_fg) + (bg.g * (255 - alpha_fg))) / 255);
    result.b = (uint8_t)(((fg.b * alpha_fg) + (bg.b * (255 - alpha_fg))) / 255);

    return result;
}

// Applies a simple glow effect to a color. This is a placeholder for a more complex shader.
// For bare-metal, this might just lighten the color or add a slight tint.
// A real glow would involve blurring and blending with surrounding pixels.
static inline vfx_color_t vfx_apply_glow(vfx_color_t color, uint8_t intensity) {
    // Simple glow: increase brightness based on intensity
    // Max out each component at 255
    uint16_t r = color.r + intensity;
    uint16_t g = color.g + intensity;
    uint16_t b = color.b + intensity;

    color.r = (uint8_t)(r > 255 ? 255 : r);
    color.g = (uint8_t)(g > 255 ? 255 : g);
    color.b = (uint8_t)(b > 255 ? 255 : b);

    return color;
}

// Placeholder for a function to draw a filled rectangle to the framebuffer.
// Actual implementation would depend on the framebuffer interface (e.g., framebuffer.h).
// This declaration assumes a global framebuffer context or one passed as an argument.
// For a header, we only define the interface.
// void vfx_draw_filled_rect(vfx_rect_t rect, vfx_color_t color);

// Example predefined colors
#define VFX_COLOR_BLACK     ((vfx_color_t){255, 0, 0, 0})
#define VFX_COLOR_WHITE     ((vfx_color_t){255, 255, 255, 255})
#define VFX_COLOR_RED       ((vfx_color_t){255, 255, 0, 0})
#define VFX_COLOR_GREEN     ((vfx_color_t){255, 0, 255, 0})
#define VFX_COLOR_BLUE      ((vfx_color_t){255, 0, 0, 255})
#define VFX_COLOR_TRANSPARENT ((vfx_color_t){0, 0, 0, 0})
