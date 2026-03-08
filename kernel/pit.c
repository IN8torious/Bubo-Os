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
// Raven OS — PIT Timer Driver
// Programs the 8253/8254 PIT to fire at 100Hz
// Each tick drives the CORVUS cognitive loop
// =============================================================================

#include "pit.h"
#include "idt.h"
#include "corvus.h"
#include "vga.h"
#include <stdint.h>

// ── PIT ports ─────────────────────────────────────────────────────────────────
#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43
#define PIT_BASE_FREQ   1193182   // Hz — PIT oscillator frequency

// ── Tick counter ──────────────────────────────────────────────────────────────
static volatile uint32_t pit_ticks = 0;
static uint32_t pit_frequency = 0;

// ── Scheduler forward declaration ────────────────────────────────────────────
extern void scheduler_tick(void);

// ── Timer IRQ handler (IRQ0) ──────────────────────────────────────────────────
static void pit_irq_handler(registers_t* regs) {
    (void)regs;
    pit_ticks++;

    // Drive CORVUS cognitive loop every tick
    corvus_tick();

    // Drive scheduler every tick (round-robin)
    // scheduler_tick();  // Uncomment when scheduler is ready
}

// ── Sleep (busy-wait, tick-based) ────────────────────────────────────────────
void pit_sleep(uint32_t ms) {
    uint32_t target = pit_ticks + (ms * pit_frequency / 1000);
    while (pit_ticks < target) {
        __asm__ volatile("hlt");
    }
}

// ── Get current tick count ────────────────────────────────────────────────────
uint32_t pit_get_ticks(void) {
    return pit_ticks;
}

// ── Initialize PIT at given frequency ────────────────────────────────────────
void pit_init(uint32_t frequency) {
    pit_frequency = frequency;

    // Calculate divisor
    uint32_t divisor = PIT_BASE_FREQ / frequency;

    // Command: channel 0, lobyte/hibyte, square wave mode
    outb(PIT_COMMAND, 0x36);

    // Send divisor low byte then high byte
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    // Register IRQ0 handler
    irq_register(0, pit_irq_handler);

    // Enable interrupts
    __asm__ volatile("sti");

    // Report
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
    terminal_setcolor(white);
    terminal_write("  [ PIT  ] Timer initialized at ");
    // print frequency
    char buf[12];
    uint32_t n = frequency;
    if (!n) { buf[0]='0'; buf[1]='\0'; }
    else { char t[12]; int i=0; while(n){t[i++]='0'+(n%10);n/=10;} int j=0; while(i>0)buf[j++]=t[--i]; buf[j]='\0'; }
    terminal_write(buf);
    terminal_setcolor(green);
    terminal_writeline("Hz — CORVUS cognitive loop armed");
}
