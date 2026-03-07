#ifndef RAVEN_IDT_H
#define RAVEN_IDT_H

// =============================================================================
// Raven OS — Interrupt Descriptor Table (IDT)
// Handles CPU exceptions, hardware IRQs, and software interrupts
// =============================================================================

#include <stdint.h>

#define IDT_ENTRIES 256

// IDT gate types
#define IDT_GATE_INTERRUPT  0x8E  // Present, Ring 0, 32-bit interrupt gate
#define IDT_GATE_TRAP       0x8F  // Present, Ring 0, 32-bit trap gate
#define IDT_GATE_USER       0xEE  // Present, Ring 3, 32-bit interrupt gate (syscalls)

// IDT entry (gate descriptor)
typedef struct {
    uint16_t offset_low;    // Lower 16 bits of handler address
    uint16_t selector;      // Kernel code segment selector
    uint8_t  zero;          // Always 0
    uint8_t  type_attr;     // Gate type + DPL + Present bit
    uint16_t offset_high;   // Upper 16 bits of handler address
} __attribute__((packed)) idt_entry_t;

// IDT pointer (for LIDT instruction)
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// CPU register state pushed by interrupt stubs
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;            // pushed by CPU
} __attribute__((packed)) registers_t;

// Initialize IDT and install all handlers
void idt_init(void);

// Set a single IDT gate
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);

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
