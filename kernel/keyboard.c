// =============================================================================
// Raven OS — Keyboard Driver
// CORVUS Perception Module — Layer 4 Sensory input
// Converts PS/2 scancodes → ASCII → feeds CORVUS intent stream
// =============================================================================

#include "keyboard.h"
#include "idt.h"
#include "corvus.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Keyboard ports ────────────────────────────────────────────────────────────
#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64

// ── Scancode → ASCII tables ───────────────────────────────────────────────────
// US QWERTY layout, set 1 scancodes
static const char scancode_normal[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static const char scancode_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// ── Keyboard state ────────────────────────────────────────────────────────────
static bool shift_pressed   = false;
static bool ctrl_pressed    = false;
static bool caps_lock       = false;

// ── Input ring buffer ─────────────────────────────────────────────────────────
#define KB_BUFFER_SIZE 256
static char    kb_buffer[KB_BUFFER_SIZE];
static uint32_t kb_buf_head = 0;
static uint32_t kb_buf_tail = 0;

static void kb_buffer_push(char c) {
    uint32_t next = (kb_buf_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_buf_tail) {
        kb_buffer[kb_buf_head] = c;
        kb_buf_head = next;
    }
}

char keyboard_getchar(void) {
    if (kb_buf_tail == kb_buf_head) return 0;
    char c = kb_buffer[kb_buf_tail];
    kb_buf_tail = (kb_buf_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

bool keyboard_has_input(void) {
    return kb_buf_tail != kb_buf_head;
}

// ── CORVUS shell input buffer ─────────────────────────────────────────────────
#define SHELL_BUF_SIZE 128
static char    shell_buf[SHELL_BUF_SIZE];
static uint32_t shell_buf_len = 0;

// Forward declaration
extern void corvus_shell_submit(const char* line);

// ── IRQ1 keyboard handler ─────────────────────────────────────────────────────
static void keyboard_irq_handler(registers_t* regs) {
    (void)regs;

    uint8_t scancode = inb(KB_DATA_PORT);
    bool key_release = (scancode & 0x80) != 0;
    uint8_t key = scancode & 0x7F;

    // Handle modifier keys
    if (key == 0x2A || key == 0x36) { shift_pressed = !key_release; return; }
    if (key == 0x1D) { ctrl_pressed = !key_release; return; }
    if (key == 0x3A && !key_release) { caps_lock = !caps_lock; return; }

    if (key_release) return;
    if (key >= 128) return;

    // Determine character
    char c = 0;
    bool use_shift = shift_pressed ^ caps_lock;

    if (use_shift) {
        c = scancode_shift[key];
    } else {
        c = scancode_normal[key];
    }

    if (!c) return;

    // Ctrl+C — interrupt
    if (ctrl_pressed && (c == 'c' || c == 'C')) {
        terminal_writeline("^C");
        shell_buf_len = 0;
        return;
    }

    // Feed to CORVUS shell
    if (c == '\n') {
        // Echo newline
        terminal_putchar('\n');

        // Submit line to CORVUS
        shell_buf[shell_buf_len] = '\0';
        if (shell_buf_len > 0) {
            corvus_shell_submit(shell_buf);
        }
        shell_buf_len = 0;

        // Print new prompt
        uint8_t red = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
        uint8_t white = vga_entry_color(VGA_WHITE, VGA_BLACK);
        terminal_setcolor(red);
        terminal_write("corvus");
        terminal_setcolor(white);
        terminal_write("> ");

    } else if (c == '\b') {
        // Backspace
        if (shell_buf_len > 0) {
            shell_buf_len--;
            // Erase character on screen
            if (terminal_col > 0) {
                terminal_col--;
            } else if (terminal_row > 0) {
                terminal_row--;
                terminal_col = VGA_WIDTH - 1;
            }
            VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_col] =
                vga_entry(' ', terminal_color);
        }
    } else {
        // Regular character — echo and buffer
        if (shell_buf_len < SHELL_BUF_SIZE - 1) {
            shell_buf[shell_buf_len++] = c;
            terminal_putchar(c);
        }
    }

    // Also push to general ring buffer
    kb_buffer_push(c);
}

// ── Initialize keyboard ───────────────────────────────────────────────────────
void keyboard_init(void) {
    // Flush any pending data
    while (inb(KB_STATUS_PORT) & 0x01) {
        inb(KB_DATA_PORT);
    }

    // Register IRQ1
    irq_register(1, keyboard_irq_handler);

    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
    terminal_setcolor(white);
    terminal_write("  [ KBD  ] Keyboard driver initialized — ");
    terminal_setcolor(green);
    terminal_writeline("CORVUS perception module active");
}
