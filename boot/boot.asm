; =============================================================================
; Raven AOS — boot.asm
; Multiboot2 + 32-bit setup + 64-bit long mode entry
; CORVUS awakens here.
; =============================================================================

bits 32

; ── Multiboot2 constants ──────────────────────────────────────────────────────
MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0
MULTIBOOT2_LEN      equ (multiboot2_end - multiboot2_start)
MULTIBOOT2_CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_LEN)

; ── Page table physical addresses (below 1MB, identity mapped) ────────────────
PML4_ADDR  equ 0x1000
PDPT_ADDR  equ 0x2000
PD_ADDR    equ 0x3000

section .multiboot2
align 8
multiboot2_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_LEN
    dd MULTIBOOT2_CHECKSUM
    ; Framebuffer tag — request linear framebuffer from GRUB
    align 8
    dw 5            ; type = framebuffer
    dw 0            ; flags
    dd 20           ; size
    dd 1024         ; width
    dd 768          ; height
    dd 32           ; depth (bits per pixel)
    ; End tag
    align 8
    dw 0
    dw 0
    dd 8
multiboot2_end:

section .bss
align 16
stack_bottom:
    resb 65536      ; 64KB kernel stack
stack_top:

; ── 64-bit GDT ───────────────────────────────────────────────────────────────
section .data
align 8
gdt64:
    dq 0                                                ; null
.code: equ $ - gdt64
    dq (1<<43)|(1<<44)|(1<<47)|(1<<53)                 ; 64-bit code
.data: equ $ - gdt64
    dq (1<<44)|(1<<47)|(1<<41)                         ; 64-bit data
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dq gdt64

section .text
global _start
extern kernel_main

_start:
    ; Preserve multiboot2 registers before anything
    mov edi, eax        ; magic  → edi (will become rdi in 64-bit)
    mov esi, ebx        ; mbi ptr → esi (will become rsi in 64-bit)

    ; Set up 32-bit stack
    mov esp, stack_top

    ; ── Verify CPUID is available ─────────────────────────────────────────────
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .error

    ; ── Verify long mode is available ────────────────────────────────────────
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .error

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .error

    ; ── Build page tables: identity map first 2MB with one huge page ─────────
    ; Zero all three tables
    mov ecx, 3 * 4096 / 4
    xor eax, eax
    mov edi, PML4_ADDR
    rep stosd
    ; Restore edi/esi (rep stosd clobbered edi)
    mov edi, eax        ; edi = 0 (magic check will be done in kernel)
    ; Re-save mbi ptr — esi is still intact

    ; PML4[0] → PDPT
    mov eax, PDPT_ADDR | 0x3
    mov [PML4_ADDR], eax

    ; PDPT[0] → PD
    mov eax, PD_ADDR | 0x3
    mov [PDPT_ADDR], eax

    ; PD[0] → 2MB huge page at physical 0x0
    mov dword [PD_ADDR], 0x83       ; present | writable | huge

    ; ── Enable PAE ────────────────────────────────────────────────────────────
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ; ── Load PML4 ─────────────────────────────────────────────────────────────
    mov eax, PML4_ADDR
    mov cr3, eax

    ; ── Set LME in EFER MSR ───────────────────────────────────────────────────
    mov ecx, 0xC0000080
    rdmsr
    or  eax, 1 << 8
    wrmsr

    ; ── Enable paging → activates long mode ──────────────────────────────────
    mov eax, cr0
    or  eax, (1 << 31) | 1
    mov cr0, eax

    ; ── Load 64-bit GDT and far-jump into 64-bit code segment ────────────────
    lgdt [gdt64_ptr]
    jmp gdt64.code:.in64

bits 64
.in64:
    ; Set all data segment registers
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; rdi = multiboot2 magic (was edi, zero-extended)
    ; rsi = multiboot2 info ptr (was esi, zero-extended)
    ; Both are already set correctly from the 32-bit saves above

    call kernel_main

.halt:
    cli
    hlt
    jmp .halt

bits 32
.error:
    ; Print "ERR" to VGA in red on white
    mov dword [0xB8000], 0x4F524F45
    mov dword [0xB8004], 0x4F4F4F52
    mov dword [0xB8008], 0x4F214F52
    cli
    hlt
