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
