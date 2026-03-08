// BUBO OS — Copyright (c) 2025 Nathan Brown & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// include/vmx.h — The VMX Hypervisor Layer
//
// BUBO OS is the primary desktop. It never yields. It never sleeps.
// Windows is a guest — a tool that wakes when Landon wants to play,
// and sleeps when the game is done. BUBO is always home.
//
// Ring -1 (VMX root) : BUBO hypervisor — owns the hardware absolutely
// Ring  0            : BUBO OS kernel  — the desktop, the agents, the soul
// Ring  3 (VM guest) : Windows 11      — wakes for Call of Duty, sleeps after
//
// "BUBO online. I'm here." — always the first and last voice.
// =============================================================================

#ifndef VMX_H
#define VMX_H

#include <stdint.h>
#include <stdbool.h>

// ── Intel VMX MSR Addresses ───────────────────────────────────────────────────
#define MSR_IA32_VMX_BASIC              0x480
#define MSR_IA32_VMX_PINBASED_CTLS     0x481
#define MSR_IA32_VMX_PROCBASED_CTLS    0x482
#define MSR_IA32_VMX_EXIT_CTLS         0x483
#define MSR_IA32_VMX_ENTRY_CTLS        0x484
#define MSR_IA32_VMX_CR0_FIXED0        0x486
#define MSR_IA32_VMX_CR0_FIXED1        0x487
#define MSR_IA32_VMX_CR4_FIXED0        0x488
#define MSR_IA32_VMX_CR4_FIXED1        0x489
#define MSR_IA32_FEATURE_CONTROL        0x03A

// ── VMCS Field Encodings (Intel SDM Vol 3C) ───────────────────────────────────
// Guest State
#define VMCS_GUEST_CR0                  0x6800
#define VMCS_GUEST_CR3                  0x6802
#define VMCS_GUEST_CR4                  0x6804
#define VMCS_GUEST_ES_SELECTOR          0x0800
#define VMCS_GUEST_CS_SELECTOR          0x0802
#define VMCS_GUEST_SS_SELECTOR          0x0804
#define VMCS_GUEST_DS_SELECTOR          0x0806
#define VMCS_GUEST_RIP                  0x681E
#define VMCS_GUEST_RSP                  0x681C
#define VMCS_GUEST_RFLAGS               0x6820
#define VMCS_GUEST_ACTIVITY_STATE       0x4826

// Host State
#define VMCS_HOST_CR0                   0x6C00
#define VMCS_HOST_CR3                   0x6C02
#define VMCS_HOST_CR4                   0x6C04
#define VMCS_HOST_RIP                   0x6C16
#define VMCS_HOST_RSP                   0x6C14

// Control Fields
#define VMCS_PIN_BASED_VM_EXEC_CTRL     0x4000
#define VMCS_CPU_BASED_VM_EXEC_CTRL     0x4002
#define VMCS_EXCEPTION_BITMAP           0x4004
#define VMCS_VM_EXIT_CONTROLS           0x400C
#define VMCS_VM_ENTRY_CONTROLS          0x4012
#define VMCS_SECONDARY_VM_EXEC_CTRL     0x401E

// Exit Information
#define VMCS_EXIT_REASON                0x4402
#define VMCS_EXIT_QUALIFICATION         0x6400
#define VMCS_GUEST_LINEAR_ADDR          0x640A

// ── VM Exit Reasons (Intel SDM Vol 3C Table 24-1) ─────────────────────────────
#define VMX_EXIT_CPUID                  10
#define VMX_EXIT_HLT                    12
#define VMX_EXIT_RDMSR                  31
#define VMX_EXIT_WRMSR                  32
#define VMX_EXIT_IO_INSTRUCTION         30
#define VMX_EXIT_EPT_VIOLATION          48
#define VMX_EXIT_XSETBV                 55

// ── VM Lifecycle States ───────────────────────────────────────────────────────
typedef enum {
    VM_STATE_NONE       = 0, // VM does not exist yet
    VM_STATE_CREATED    = 1, // VMCS allocated, not yet running
    VM_STATE_RUNNING    = 2, // Windows is active, game is running
    VM_STATE_SUSPENDED  = 3, // Game paused or closed, VM sleeping
    VM_STATE_DESTROYED  = 4  // VM torn down, memory returned to BUBO
} vm_state_t;

// ── The Windows Guest VM Descriptor ──────────────────────────────────────────
typedef struct {
    vm_state_t  state;
    uint64_t    vmcs_phys_addr;      // Physical address of the VMCS region
    uint64_t    guest_phys_base;     // Start of guest physical memory
    uint64_t    guest_mem_size;      // How much RAM Windows gets (e.g., 8GB)
    uint64_t    guest_rip;           // Guest instruction pointer (resume point)
    uint64_t    guest_rsp;           // Guest stack pointer (resume point)
    uint32_t    gpu_bdf;             // PCI Bus:Device:Function of the passed-through GPU
    bool        gpu_passed_through;  // Is the GPU assigned directly to this VM?
    bool        game_running;        // Is Call of Duty currently active?
} bubo_vm_t;

// ── Core Hypervisor API ───────────────────────────────────────────────────────

// Check if this CPU supports VMX. Returns false if hardware doesn't support it.
bool vmx_check_support(void);

// Enable VMX operation (VMXON). BUBO becomes the hypervisor.
bool vmx_enable(void);

// Disable VMX operation (VMXOFF). Returns hardware to normal mode.
void vmx_disable(void);

// ── Windows VM Lifecycle ──────────────────────────────────────────────────────

// Create the Windows VM — allocate VMCS, set up memory, configure guest state.
// Called once at boot. The VM starts in SUSPENDED state.
bool vmx_create_windows_vm(bubo_vm_t* vm, uint64_t mem_size_bytes);

// Wake the Windows VM — resume from suspended state.
// Called when Landon says "play" or launches Call of Duty.
bool vmx_resume_windows_vm(bubo_vm_t* vm);

// Suspend the Windows VM — freeze state, return GPU to BUBO desktop.
// Called when the game ends or Landon returns to BUBO desktop.
void vmx_suspend_windows_vm(bubo_vm_t* vm);

// Destroy the Windows VM — free all memory, return to BUBO.
void vmx_destroy_windows_vm(bubo_vm_t* vm);

// ── VM Exit Handler ───────────────────────────────────────────────────────────
// Called automatically by the CPU on every VM exit.
// VASH inspects every exit for constitutional violations.
// Returns true if the VM should be resumed, false if it should be suspended.
bool vmx_handle_exit(bubo_vm_t* vm);

// ── Global VM Instance ────────────────────────────────────────────────────────
// There is only one Windows VM in BUBO OS. One guest. One game.
extern bubo_vm_t g_windows_vm;

#endif // VMX_H
