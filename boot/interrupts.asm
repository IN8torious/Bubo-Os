; =============================================================================
; Raven OS — Interrupt Service Routine Stubs
; Generates ISR stubs for CPU exceptions (0-31) and IRQ handlers (32-47)
; Each stub saves CPU state and calls the C handler
; =============================================================================

bits 32

; ── Macros ────────────────────────────────────────────────────────────────────

; ISR with no error code pushed by CPU (we push a dummy 0)
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push byte 0         ; dummy error code
    push byte %1        ; interrupt number
    jmp isr_common_stub
%endmacro

; ISR where CPU pushes an error code automatically
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push byte %1        ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; IRQ stub — maps IRQ N to interrupt (N+32)
%macro IRQ 2
global irq%1
irq%1:
    cli
    push byte 0         ; dummy error code
    push byte %2        ; interrupt number
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
ISR_ERR    8    ; Double fault (has error code)
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
IRQ  0, 32    ; PIT Timer
IRQ  1, 33    ; Keyboard
IRQ  2, 34    ; Cascade (used internally)
IRQ  3, 35    ; COM2
IRQ  4, 36    ; COM1
IRQ  5, 37    ; LPT2
IRQ  6, 38    ; Floppy
IRQ  7, 39    ; LPT1 / Spurious
IRQ  8, 40    ; CMOS RTC
IRQ  9, 41    ; Free / ACPI
IRQ 10, 42    ; Free
IRQ 11, 43    ; Free
IRQ 12, 44    ; PS/2 Mouse
IRQ 13, 45    ; FPU / Coprocessor
IRQ 14, 46    ; Primary ATA
IRQ 15, 47    ; Secondary ATA

; ── Common ISR stub ───────────────────────────────────────────────────────────
extern isr_handler

isr_common_stub:
    pusha               ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    mov ax, ds
    push eax            ; Save data segment

    mov ax, 0x10        ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pass pointer to registers struct
    call isr_handler
    add esp, 4          ; Clean up pointer

    pop eax             ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Restore general registers
    add esp, 8          ; Clean up error code + interrupt number
    sti
    iret

; ── Common IRQ stub ───────────────────────────────────────────────────────────
extern irq_handler

irq_common_stub:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    sti
    iret
