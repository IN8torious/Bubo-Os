; =============================================================================
; Raven OS — Context Switch (Assembly)
; Saves current CPU state into old_ctx, loads new_ctx
; Called by the C scheduler when switching processes
; =============================================================================

bits 32

global scheduler_switch

; void scheduler_switch(cpu_context_t* old_ctx, cpu_context_t* new_ctx)
; [esp+4] = old_ctx pointer
; [esp+8] = new_ctx pointer
scheduler_switch:
    ; Get parameters
    mov eax, [esp+4]    ; old_ctx
    mov ecx, [esp+8]    ; new_ctx

    ; Save current context into old_ctx
    mov [eax+0],  esp
    mov [eax+8],  ebx
    mov [eax+12], ecx
    mov [eax+16], edx   ; Note: ecx is new_ctx, but we save the original
    mov [eax+20], esi
    mov [eax+24], edi
    mov [eax+28], ebp

    ; Save eflags
    pushf
    pop edx
    mov [eax+4], edx    ; eflags

    ; Load new context from new_ctx
    mov esp, [ecx+0]    ; Restore stack pointer
    mov ebx, [ecx+8]
    mov edx, [ecx+16]
    mov esi, [ecx+20]
    mov edi, [ecx+24]
    mov ebp, [ecx+28]

    ; Restore eflags
    mov eax, [ecx+4]
    push eax
    popf

    ; Return — EIP is now the new process's return address
    ret
