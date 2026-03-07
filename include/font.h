// =============================================================================
// Raven AOS — Dedicated to Landon Pankuch
// =============================================================================
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

#ifndef RAVEN_FONT_H
#define RAVEN_FONT_H

#include <stdint.h>
#include <stdbool.h>
#include "framebuffer.h"

#define FONT_WIDTH        8
#define FONT_HEIGHT       16
#define FONT_CHAR_SPACING 1
#define FONT_LINE_SPACING 2

void    font_draw_char(int32_t x, int32_t y, char c,
                       uint32_t fg, uint32_t bg, bool transparent_bg);
void    font_draw_string(int32_t x, int32_t y, const char* str,
                         uint32_t fg, uint32_t bg, bool transparent_bg);
void    font_draw_string_scaled(int32_t x, int32_t y, const char* str,
                                uint32_t fg, uint32_t bg, bool transparent_bg,
                                int scale);
void    font_draw_centered(int32_t cx, int32_t y, const char* str,
                           uint32_t fg, uint32_t bg, bool transparent_bg);
int32_t font_measure_width(const char* str);

#endif // RAVEN_FONT_H
