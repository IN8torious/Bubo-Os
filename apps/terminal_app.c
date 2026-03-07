// =============================================================================
// Instinct OS — Dedicated to Landon Pankuch
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

// =============================================================================
// Instinct OS — Terminal App
// A simple in-desktop terminal emulator backed by the kernel VFS and shell.
// =============================================================================

#include "terminal_app.h"
#include "framebuffer.h"
#include "font.h"
#include "vfs.h"
#include "vga.h"

#define TERM_COLS    80
#define TERM_ROWS    24
#define TERM_BG      0xFF0D0D0D
#define TERM_FG      0xFF00FF41   // Matrix green
#define TERM_PROMPT  0xFFCC0000   // Crimson prompt
#define TERM_CURSOR  0xFF00FF41

static char term_lines[TERM_ROWS][TERM_COLS + 1];
static int  term_row = 0;
static int  term_col = 0;
static char input_buf[256];
static int  input_len = 0;

static void term_scroll(void) {
    for (int r = 0; r < TERM_ROWS - 1; r++) {
        for (int c = 0; c <= TERM_COLS; c++) {
            term_lines[r][c] = term_lines[r+1][c];
        }
    }
    for (int c = 0; c <= TERM_COLS; c++) term_lines[TERM_ROWS-1][c] = 0;
    if (term_row > 0) term_row--;
}

static void term_putchar(char ch) {
    if (ch == '\n') {
        term_row++;
        term_col = 0;
        if (term_row >= TERM_ROWS) term_scroll();
        return;
    }
    if (term_col >= TERM_COLS) {
        term_row++;
        term_col = 0;
        if (term_row >= TERM_ROWS) term_scroll();
    }
    term_lines[term_row][term_col++] = ch;
}

static void term_puts(const char* s) {
    while (*s) term_putchar(*s++);
}

void terminal_app_init(void) {
    for (int r = 0; r < TERM_ROWS; r++)
        for (int c = 0; c <= TERM_COLS; c++)
            term_lines[r][c] = 0;
    term_row = 0; term_col = 0;
    input_len = 0; input_buf[0] = 0;

    term_puts("Instinct OS v0.7 — CORVUS Terminal\n");
    term_puts("MAS Sovereign Intelligence | NO MAS DISADVANTAGED\n");
    term_puts("Type 'help' for commands.\n");
    term_puts("\n");
}

void terminal_app_render(uint32_t wx, uint32_t wy, uint32_t ww, uint32_t wh) {
    uint32_t cx = wx + 1;
    uint32_t cy = wy + 24;
    uint32_t cw = ww - 2;
    uint32_t ch = wh - 25;

    fb_fill_rect(cx, cy, cw, ch, TERM_BG);

    // Draw text lines
    uint32_t char_h = 14;
    for (int r = 0; r < TERM_ROWS && r < (int)(ch / char_h); r++) {
        if (term_lines[r][0] == 0) continue;
        font_draw_string(cx + 4, cy + 4 + r * char_h,
                         term_lines[r], TERM_FG, TERM_BG, true);
    }

    // Draw input line with prompt
    uint32_t input_y = cy + ch - char_h - 6;
    fb_fill_rect(cx, input_y - 2, cw, char_h + 4, 0xFF111111);
    font_draw_string(cx + 4, input_y, "raven> ", TERM_PROMPT, TERM_BG, true);
    font_draw_string(cx + 60, input_y, input_buf, TERM_FG, TERM_BG, true);

    // Blinking cursor
    static uint32_t blink = 0;
    blink++;
    if ((blink / 30) % 2 == 0) {
        uint32_t cur_x = cx + 60 + input_len * 8;
        fb_fill_rect(cur_x, input_y, 2, char_h, TERM_CURSOR);
    }
}

static void term_exec(const char* cmd) {
    term_puts("raven> ");
    term_puts(cmd);
    term_puts("\n");

    // Built-in commands
    if (cmd[0]=='h'&&cmd[1]=='e'&&cmd[2]=='l'&&cmd[3]=='p'&&cmd[4]==0) {
        term_puts("Commands: help  ls  pwd  clear  corvus  mandate  version\n");
    } else if (cmd[0]=='l'&&cmd[1]=='s'&&cmd[2]==0) {
        term_puts("/\n  boot/\n  kernel/\n  apps/\n  home/\n  user/\n");
    } else if (cmd[0]=='p'&&cmd[1]=='w'&&cmd[2]=='d'&&cmd[3]==0) {
        term_puts("/home/raven\n");
    } else if (cmd[0]=='c'&&cmd[1]=='l'&&cmd[2]=='e'&&cmd[3]=='a'&&cmd[4]=='r'&&cmd[5]==0) {
        for (int r=0;r<TERM_ROWS;r++) for(int c=0;c<=TERM_COLS;c++) term_lines[r][c]=0;
        term_row=0; term_col=0;
    } else if (cmd[0]=='c'&&cmd[1]=='o'&&cmd[2]=='r'&&cmd[3]=='v'&&cmd[4]=='u'&&cmd[5]=='s'&&cmd[6]==0) {
        term_puts("CORVUS MAS — 10 agents active\n");
        term_puts("Mandate: NO MAS DISADVANTAGED\n");
    } else if (cmd[0]=='m'&&cmd[1]=='a'&&cmd[2]=='n'&&cmd[3]=='d'&&cmd[4]=='a'&&cmd[5]=='t'&&cmd[6]=='e'&&cmd[7]==0) {
        term_puts("NO MAS DISADVANTAGED\n");
        term_puts("MAS = Multi-Agentic Systems\n");
        term_puts("Sovereign Intelligence. Not corporate AI.\n");
    } else if (cmd[0]=='v'&&cmd[1]=='e'&&cmd[2]=='r'&&cmd[3]=='s'&&cmd[4]=='i'&&cmd[5]=='o'&&cmd[6]=='n'&&cmd[7]==0) {
        term_puts("Instinct OS v0.7 | CORVUS MAS | Akatsuki Theme\n");
        term_puts("Built for Landon Pankuch — NO MAS DISADVANTAGED\n");
    } else {
        term_puts("Unknown command: ");
        term_puts(cmd);
        term_puts("\n");
    }
}

void terminal_app_key(char key) {
    if (key == '\n' || key == '\r') {
        if (input_len > 0) {
            input_buf[input_len] = 0;
            term_exec(input_buf);
            input_len = 0;
            input_buf[0] = 0;
        }
    } else if (key == '\b') {
        if (input_len > 0) input_buf[--input_len] = 0;
    } else if (input_len < 254) {
        input_buf[input_len++] = key;
        input_buf[input_len] = 0;
    }
}
