; =============================================================================
; Raven AOS — Interrupt Service Routine Stubs (64-bit long mode)
; Generates ISR stubs for CPU exceptions (0-31) and IRQ handlers (32-47)
; Each stub saves full 64-bit CPU state and calls the C handler
; =============================================================================

bits 64

extern isr_handler
extern irq_handler

; ── Save/restore all GP registers ────────────────────────────────────────────
; Order must match registers_t struct (reversed — last pushed = first in struct)
%macro PUSH_ALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; ── ISR with no error code (push dummy 0) ────────────────────────────────────
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0        ; dummy error code
    push qword %1       ; interrupt number
    jmp isr_common_stub
%endmacro

; ── ISR where CPU auto-pushes error code ─────────────────────────────────────
%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1       ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; ── IRQ stub ─────────────────────────────────────────────────────────────────
%macro IRQ 2
global irq%1
irq%1:
    push qword 0        ; dummy error code
    push qword %2       ; interrupt number
    jmp irq_common_stub
%endmacro

; ── CPU Exception ISRs (0-31) ─────────────────────────────────────────────────
ISR_NOERR  0    ; Divide by zero
ISR_NOERR  1    ; Debug
ISR_NOERR  2    ; Non-maskable interrupt
ISR_NOERR  3    ; Breakpoint
ISR_NOERR  4    ; Overflow
ISR_NOERR  5    ; Bound range exceeded
ISR_NOERR  6    ; Invalid opcode
ISR_NOERR  7    ; Device not available
ISR_ERR    8    ; Double fault
ISR_NOERR  9    ; Coprocessor segment overrun
ISR_ERR   10    ; Invalid TSS
ISR_ERR   11    ; Segment not present
ISR_ERR   12    ; Stack-segment fault
ISR_ERR   13    ; General protection fault
ISR_ERR   14    ; Page fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; x87 FPU error
ISR_ERR   17    ; Alignment check
ISR_NOERR 18    ; Machine check
ISR_NOERR 19    ; SIMD FP exception
ISR_NOERR 20    ; Virtualization exception
ISR_NOERR 21    ; Reserved
ISR_NOERR 22    ; Reserved
ISR_NOERR 23    ; Reserved
ISR_NOERR 24    ; Reserved
ISR_NOERR 25    ; Reserved
ISR_NOERR 26    ; Reserved
ISR_NOERR 27    ; Reserved
ISR_NOERR 28    ; Reserved
ISR_NOERR 29    ; Reserved
ISR_ERR   30    ; Security exception
ISR_NOERR 31    ; Reserved

; ── Hardware IRQs (32-47) ─────────────────────────────────────────────────────
IRQ  0, 32   ; PIT timer
IRQ  1, 33   ; Keyboard
IRQ  2, 34   ; Cascade
IRQ  3, 35   ; COM2
IRQ  4, 36   ; COM1
IRQ  5, 37   ; LPT2
IRQ  6, 38   ; Floppy
IRQ  7, 39   ; LPT1 / spurious
IRQ  8, 40   ; CMOS RTC
IRQ  9, 41   ; Free
IRQ 10, 42   ; Free
IRQ 11, 43   ; Free
IRQ 12, 44   ; PS/2 Mouse
IRQ 13, 45   ; FPU
IRQ 14, 46   ; ATA primary
IRQ 15, 47   ; ATA secondary

; ── Common ISR stub ───────────────────────────────────────────────────────────
isr_common_stub:
    PUSH_ALL
    mov rdi, rsp        ; rdi = pointer to registers_t (System V ABI arg0)
    call isr_handler
    POP_ALL
    add rsp, 16         ; pop int_no + err_code
    iretq               ; 64-bit interrupt return

; ── Common IRQ stub ───────────────────────────────────────────────────────────
irq_common_stub:
    PUSH_ALL
    mov rdi, rsp
    call irq_handler
    POP_ALL
    add rsp, 16
    iretq
