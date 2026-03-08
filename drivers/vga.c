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

// =============================================================================
// Raven OS — VGA Text Mode Driver
// Provides terminal output via the VGA text buffer at 0xB8000
// =============================================================================

#include "vga.h"
#include <stddef.h>
#include <stdint.h>

// ── Deep Flow Red Palette — VGA DAC reprogramming ────────────────────────────
// The Matrix runs green. Deep Flow OS runs RED. That's Nathan's color.
// We reprogram the VGA DAC at boot so the hardware palette reflects our identity.
static inline void vga_out8(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void vga_set_dac_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    vga_out8(0x3C8, index);
    vga_out8(0x3C9, r);
    vga_out8(0x3C9, g);
    vga_out8(0x3C9, b);
}

static void vga_apply_deepflow_palette(void) {
    // 16-entry red spectrum palette — 6-bit DAC values (0-63)
    // { index, R, G, B }
    static const uint8_t pal[16][4] = {
        {  0,  0,  0,  0 }, // 0: Black
        {  1, 34,  2,  2 }, // 1: Dark red — errors
        {  2, 10,  0,  0 }, // 2: Deep red — dim text
        {  3, 44,  8,  8 }, // 3: Mid red — normal text
        {  4, 20,  0,  0 }, // 4: Dark background
        {  5, 27,  1,  0 }, // 5: VASH crimson
        {  6, 50, 30,  0 }, // 6: Orange — solutions
        {  7, 55, 55, 55 }, // 7: Light gray — UI chrome
        {  8, 25,  0,  0 }, // 8: Dark gray with red tint
        {  9, 63,  8,  8 }, // 9: Bright red — code stream
        { 10, 20,  0,  0 }, // A: Dark red
        { 11, 63, 17, 17 }, // B: Bright red highlight
        { 12, 63, 42,  0 }, // C: Orange-gold — Ultra Instinct
        { 13, 63, 50,  0 }, // D: Bright gold
        { 14, 63, 63, 63 }, // E: White — critical alerts
        { 15, 63, 26,  0 }, // F: Orange — solution bright
    };
    for (int i = 0; i < 16; i++) {
        vga_set_dac_color(pal[i][0], pal[i][1], pal[i][2], pal[i][3]);
    }
}

size_t terminal_row = 0;
size_t terminal_col = 0;
uint8_t terminal_color;

void terminal_init(void) {
    // Deep Flow red palette — Nathan's color. The Matrix runs green. We run red.
    vga_apply_deepflow_palette();
    // Bright red on black — the living code stream
    terminal_color = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
    terminal_clear();
}

void terminal_clear(void) {
    terminal_row = 0;
    terminal_col = 0;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

static void terminal_scroll(void) {
    // Shift all rows up by one
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[(y - 1) * VGA_WIDTH + x] = VGA_BUFFER[y * VGA_WIDTH + x];
        }
    }
    // Clear the last row
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
    terminal_row = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_col = 0;
        if (++terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
        return;
    }
    if (c == '\r') {
        terminal_col = 0;
        return;
    }
    if (c == '\t') {
        // 4-space tab
        for (int i = 0; i < 4; i++) terminal_putchar(' ');
        return;
    }

    VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_col] = vga_entry((unsigned char)c, terminal_color);

    if (++terminal_col >= VGA_WIDTH) {
        terminal_col = 0;
        if (++terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
    }
}

void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_writeline(const char* str) {
    terminal_write(str);
    terminal_putchar('\n');
}
