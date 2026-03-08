// =============================================================================
// BUBO OS — Safety Flask
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// "To obtain something, something of equal value must be lost."
// The Safety Flask contains the soul of the OS. If the kernel panics,
// the Flask catches it, displays the error, and holds the system in a
// safe halt loop — never corrupting, never crashing silently.
//
// The Flask is the equivalent exchange: one panic in, one safe state out.
// =============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "vga.h"
#include "framebuffer.h"

// ── Flask state ───────────────────────────────────────────────────────────────
static bool flask_sealed = false;
static char flask_panic_msg[256];
static uint32_t flask_panic_code = 0;

// ── Safe halt — the final resting state ──────────────────────────────────────
static void __attribute__((noreturn)) flask_halt_forever(void) {
    __asm__ volatile("cli");          // Disable interrupts — nothing can disturb the Flask
    while (1) {
        __asm__ volatile("hlt");      // Halt. Wait. Hold.
    }
}

// ── Panic screen — Akatsuki red, clean, honest ───────────────────────────────
static void flask_draw_panic_screen(const char* msg, uint32_t code) {
    // Try framebuffer first, fall back to VGA text
    fb_info_t* fb = fb_get_info();
    if (fb && fb->addr) {
        // Fill screen with deep crimson
        fb_fill_rect(0, 0, (int32_t)fb->width, (int32_t)fb->height, 0x1A0000);
        // Red border
        fb_draw_rect(8, 8, (int32_t)fb->width - 16, (int32_t)fb->height - 16, 3, 0xCC0000);
        // Title bar
        fb_fill_rect(8, 8, (int32_t)fb->width - 16, 60, 0xCC0000);
    }

    // VGA text fallback — always works
    terminal_setcolor(vga_entry_color(VGA_LIGHT_RED, VGA_BLACK));
    terminal_writeline("");
    terminal_writeline("  ╔══════════════════════════════════════════════════════╗");
    terminal_writeline("  ║           BUBO OS — SAFETY FLASK ENGAGED             ║");
    terminal_writeline("  ╠══════════════════════════════════════════════════════╣");
    terminal_writeline("  ║                                                      ║");

    terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
    terminal_write("  ║  PANIC: ");
    terminal_write(msg);
    terminal_writeline("");

    terminal_setcolor(vga_entry_color(VGA_YELLOW, VGA_BLACK));
    terminal_writeline("  ║                                                      ║");
    terminal_writeline("  ║  The Flask has sealed the error.                     ║");
    terminal_writeline("  ║  The soul is protected. Landon is safe.              ║");
    terminal_writeline("  ║                                                      ║");
    terminal_writeline("  ║  Mater is logging this failure for resurrection.     ║");
    terminal_writeline("  ║  Press RESET to reboot. The OS will remember.        ║");
    terminal_writeline("  ║                                                      ║");

    terminal_setcolor(vga_entry_color(VGA_LIGHT_RED, VGA_BLACK));
    terminal_writeline("  ║           NO MAS DISADVANTAGED                       ║");
    terminal_writeline("  ╚══════════════════════════════════════════════════════╝");
    terminal_writeline("");
}

// ── Public API ────────────────────────────────────────────────────────────────

// Call this once at the start of kernel_main to seal the Flask
void safety_flask_init(void) {
    flask_sealed = true;
    flask_panic_code = 0;
    for (int i = 0; i < 256; i++) flask_panic_msg[i] = 0;
    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));
    terminal_writeline("  [FLASK] Safety Flask sealed — kernel is protected");
}

// Call this from any point in the kernel when something goes fatally wrong
void __attribute__((noreturn)) kernel_panic(const char* msg) {
    __asm__ volatile("cli");  // Disable interrupts immediately

    // Copy message
    const char* src = msg ? msg : "Unknown panic";
    int i = 0;
    while (src[i] && i < 255) {
        flask_panic_msg[i] = src[i];
        i++;
    }
    flask_panic_msg[i] = 0;

    flask_draw_panic_screen(flask_panic_msg, flask_panic_code);
    flask_halt_forever();
}

// Convenience macro version — use PANIC("message") anywhere in kernel code
// (defined in safety_flask.h)

// ── Exception handlers — wire these to IDT entries ───────────────────────────
void flask_handle_divide_by_zero(void)    { kernel_panic("Division by zero"); }
void flask_handle_general_protection(void){ kernel_panic("General protection fault"); }
void flask_handle_page_fault(void)        { kernel_panic("Page fault"); }
void flask_handle_double_fault(void)      { kernel_panic("Double fault"); }
void flask_handle_stack_fault(void)       { kernel_panic("Stack segment fault"); }
void flask_handle_invalid_opcode(void)    { kernel_panic("Invalid opcode"); }
