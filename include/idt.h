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

#ifndef RAVEN_IDT_H
#define RAVEN_IDT_H

// =============================================================================
// Raven OS — Interrupt Descriptor Table (IDT)
// Handles CPU exceptions, hardware IRQs, and software interrupts
// =============================================================================

#include <stdint.h>

#define IDT_ENTRIES 256

// 64-bit IDT gate types
#define IDT_GATE_INTERRUPT  0x8E  // Present, Ring 0, 64-bit interrupt gate
#define IDT_GATE_TRAP       0x8F  // Present, Ring 0, 64-bit trap gate
#define IDT_GATE_USER       0xEE  // Present, Ring 3, 64-bit interrupt gate (syscalls)

// 64-bit IDT entry (16 bytes)
typedef struct {
    uint16_t offset_low;    // bits 0-15 of handler address
    uint16_t selector;      // kernel code segment selector
    uint8_t  ist;           // interrupt stack table (0 = none)
    uint8_t  type_attr;     // gate type + DPL + present
    uint16_t offset_mid;    // bits 16-31 of handler address
    uint32_t offset_high;   // bits 32-63 of handler address
    uint32_t reserved;      // must be zero
} __attribute__((packed)) idt_entry_t;

// IDT pointer (for LIDT instruction)
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// CPU register state pushed by 64-bit interrupt stubs
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    // CPU auto-pushes on interrupt:
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

// Initialize IDT and install all handlers
void idt_init(void);

// Set a single IDT gate (64-bit handler pointer)
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags);

// Generic interrupt handler (called from ASM stubs)
void isr_handler(registers_t* regs);
void irq_handler(registers_t* regs);

// IRQ registration
typedef void (*irq_handler_t)(registers_t*);
void irq_register(uint8_t irq, irq_handler_t handler);

// PIC remapping
void pic_init(void);
void pic_send_eoi(uint8_t irq);

// I/O port helpers
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif // RAVEN_IDT_H
