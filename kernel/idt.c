// =============================================================================
// Raven OS — IDT Implementation
// Sets up the Interrupt Descriptor Table, remaps the PIC,
// installs all 32 exception handlers + 16 IRQ handlers
// =============================================================================

#include "idt.h"
#include "vga.h"
#include <stdint.h>

// ── IDT storage ───────────────────────────────────────────────────────────────
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

// ── IRQ handler table ─────────────────────────────────────────────────────────
static irq_handler_t irq_handlers[16] = {0};

// ── Exception names ───────────────────────────────────────────────────────────
static const char* exception_names[] = {
    "Division By Zero",         "Debug",
    "Non-Maskable Interrupt",   "Breakpoint",
    "Overflow",                 "Bound Range Exceeded",
    "Invalid Opcode",           "Device Not Available",
    "Double Fault",             "Coprocessor Segment Overrun",
    "Invalid TSS",              "Segment Not Present",
    "Stack-Segment Fault",      "General Protection Fault",
    "Page Fault",               "Reserved",
    "x87 FPU Error",            "Alignment Check",
    "Machine Check",            "SIMD FP Exception",
    "Virtualization Exception", "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Security Exception",       "Reserved"
};

// ── External ISR/IRQ stubs from interrupts.asm ────────────────────────────────
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

// ── PIC constants ─────────────────────────────────────────────────────────────
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1
#define PIC_EOI     0x20
#define ICW1_INIT   0x11
#define ICW4_8086   0x01

// ── PIC initialization ────────────────────────────────────────────────────────
void pic_init(void) {
    // Save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // Start init sequence
    outb(PIC1_CMD,  ICW1_INIT); io_wait();
    outb(PIC2_CMD,  ICW1_INIT); io_wait();

    // Remap: IRQ0-7 → INT 32-39, IRQ8-15 → INT 40-47
    outb(PIC1_DATA, 0x20); io_wait();  // Master offset: 32
    outb(PIC2_DATA, 0x28); io_wait();  // Slave offset: 40

    // Tell master there's a slave at IRQ2
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    // 8086 mode
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

// ── IDT gate setup ────────────────────────────────────────────────────────────
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[num].selector    = selector;
    idt[num].zero        = 0;
    idt[num].type_attr   = flags;
    idt[num].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

// ── ISR handler (called from ASM) ────────────────────────────────────────────
void isr_handler(registers_t* regs) {
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,  VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY,   VGA_BLACK);

    if (regs->int_no == 14) {
        // Page fault — read CR2 for faulting address
        uint32_t fault_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

        terminal_setcolor(red);
        terminal_writeline("\n  !! PAGE FAULT !!");
        terminal_setcolor(white);
        terminal_write("  Fault address: 0x");
        // print hex
        const char* hex = "0123456789ABCDEF";
        char buf[9]; buf[8] = '\0';
        for (int i = 7; i >= 0; i--) buf[7-i] = hex[(fault_addr >> (i*4)) & 0xF];
        terminal_writeline(buf);

        terminal_setcolor(grey);
        terminal_write("  Error code: ");
        uint32_t e = regs->err_code;
        terminal_write((e & 1) ? "Present " : "NotPresent ");
        terminal_write((e & 2) ? "Write "   : "Read ");
        terminal_write((e & 4) ? "User "    : "Kernel ");
        terminal_writeline("");

        // Notify CORVUS healer agent
        terminal_setcolor(red);
        terminal_writeline("  [CORVUS:HEALER] Page fault detected — attempting recovery...");

        // Halt — in future: CORVUS healer patches this
        __asm__ volatile("cli; hlt");
    }
    else if (regs->int_no == 13) {
        // General Protection Fault
        terminal_setcolor(red);
        terminal_writeline("\n  !! GENERAL PROTECTION FAULT !!");
        terminal_setcolor(white);
        terminal_write("  Error code: 0x");
        char buf[9]; buf[8] = '\0';
        const char* hex = "0123456789ABCDEF";
        uint32_t e = regs->err_code;
        for (int i = 7; i >= 0; i--) buf[7-i] = hex[(e >> (i*4)) & 0xF];
        terminal_writeline(buf);
        terminal_setcolor(red);
        terminal_writeline("  [CORVUS:CROW] GPF detected — system halted");
        __asm__ volatile("cli; hlt");
    }
    else if (regs->int_no < 32) {
        // Other CPU exception
        terminal_setcolor(red);
        terminal_write("\n  !! EXCEPTION: ");
        if (regs->int_no < 32) {
            terminal_write(exception_names[regs->int_no]);
        }
        terminal_writeline(" !!");
        terminal_setcolor(white);
        terminal_write("  INT: ");
        char n = '0' + (char)(regs->int_no % 10);
        char buf[3] = {(regs->int_no >= 10) ? ('0' + regs->int_no/10) : ' ', n, '\0'};
        terminal_write(buf);
        terminal_write("  EIP: 0x");
        const char* hex = "0123456789ABCDEF";
        char hbuf[9]; hbuf[8] = '\0';
        for (int i = 7; i >= 0; i--) hbuf[7-i] = hex[(regs->eip >> (i*4)) & 0xF];
        terminal_writeline(hbuf);
        terminal_setcolor(red);
        terminal_writeline("  [CORVUS] Unhandled exception — system halted");
        __asm__ volatile("cli; hlt");
    }
}

// ── IRQ handler (called from ASM) ────────────────────────────────────────────
void irq_handler(registers_t* regs) {
    uint8_t irq = (uint8_t)(regs->int_no - 32);

    // Dispatch to registered handler
    if (irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](regs);
    }

    pic_send_eoi(irq);
}

// ── Register IRQ handler ──────────────────────────────────────────────────────
void irq_register(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) irq_handlers[irq] = handler;
}

// ── IDT initialization ────────────────────────────────────────────────────────
void idt_init(void) {
    idt_ptr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_ENTRIES - 1);
    idt_ptr.base  = (uint32_t)&idt;

    // Zero out IDT
    uint8_t* p = (uint8_t*)idt;
    for (uint32_t i = 0; i < sizeof(idt); i++) p[i] = 0;

    // Remap PIC before installing IRQ handlers
    pic_init();

    // Install CPU exception handlers (ISRs 0-31)
    idt_set_gate( 0, (uint32_t)isr0,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 1, (uint32_t)isr1,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 2, (uint32_t)isr2,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 3, (uint32_t)isr3,  0x08, IDT_GATE_TRAP);
    idt_set_gate( 4, (uint32_t)isr4,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 5, (uint32_t)isr5,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 6, (uint32_t)isr6,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 7, (uint32_t)isr7,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 8, (uint32_t)isr8,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate( 9, (uint32_t)isr9,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_GATE_INTERRUPT);

    // Install hardware IRQ handlers (IRQs 0-15 → INTs 32-47)
    idt_set_gate(32, (uint32_t)irq0,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(33, (uint32_t)irq1,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(34, (uint32_t)irq2,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(35, (uint32_t)irq3,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(36, (uint32_t)irq4,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(37, (uint32_t)irq5,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(38, (uint32_t)irq6,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(39, (uint32_t)irq7,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(40, (uint32_t)irq8,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(41, (uint32_t)irq9,  0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_GATE_INTERRUPT);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_GATE_INTERRUPT);

    // Load the IDT
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    // Report
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
    terminal_setcolor(white);
    terminal_write("  [ IDT  ] Interrupt Descriptor Table loaded — ");
    terminal_setcolor(green);
    terminal_writeline("48 gates installed");
    terminal_setcolor(white);
    terminal_writeline("  [ PIC  ] 8259 PIC remapped — IRQ0-15 \xbb INT 32-47");
}
