; =============================================================================
; Raven AOS — Context Switch (64-bit long mode)
; Saves current CPU state into old_ctx, loads new_ctx
; System V AMD64 ABI: rdi = old_ctx, rsi = new_ctx
; =============================================================================

bits 64

global scheduler_switch

; void scheduler_switch(cpu_context_t* old_ctx, cpu_context_t* new_ctx)
; rdi = old_ctx, rsi = new_ctx
scheduler_switch:
    ; ── Save callee-saved registers into old_ctx ─────────────────────────────
    mov [rdi+0],   rsp
    mov [rdi+8],   rbp
    mov [rdi+16],  rbx
    mov [rdi+24],  r12
    mov [rdi+32],  r13
    mov [rdi+40],  r14
    mov [rdi+48],  r15
    ; Save rflags
    pushfq
    pop rax
    mov [rdi+56],  rax

    ; ── Restore callee-saved registers from new_ctx ──────────────────────────
    mov rsp, [rsi+0]
    mov rbp, [rsi+8]
    mov rbx, [rsi+16]
    mov r12, [rsi+24]
    mov r13, [rsi+32]
    mov r14, [rsi+40]
    mov r15, [rsi+48]
    ; Restore rflags
    mov rax, [rsi+56]
    push rax
    popfq

    ; Return — RIP is the new process's saved return address on its stack
    ret
