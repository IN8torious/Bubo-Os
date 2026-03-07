// =============================================================================
// Raven OS — VGA Text Mode Driver
// Provides terminal output via the VGA text buffer at 0xB8000
// =============================================================================

#include "vga.h"
#include <stddef.h>
#include <stdint.h>

size_t terminal_row = 0;
size_t terminal_col = 0;
uint8_t terminal_color;

void terminal_init(void) {
    // Akatsuki theme: dark red on black
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
