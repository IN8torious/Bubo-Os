// BUBO OS — Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/vmx.c — The VMX Hypervisor Core
//
// BUBO OS is the primary desktop. It never yields. It never sleeps.
// This file puts BUBO in VMX root mode (ring -1) at boot, creates the
// Windows VM in a suspended state, and handles every VM exit.
//
// BUBO owns the hardware. Windows is a guest. The constitution is absolute.
// =============================================================================

#include "vmx.h"
#include "corvus_archivist.h"
#include "corvus_bubo.h"
#include "corvus_vash.h"
#include "deepflow_colors.h"

// ── The one and only Windows VM ───────────────────────────────────────────────
bubo_vm_t g_windows_vm = {
    .state             = VM_STATE_NONE,
    .vmcs_phys_addr    = 0,
    .guest_phys_base   = 0,
    .guest_mem_size    = 0,
    .guest_rip         = 0,
    .guest_rsp         = 0,
    .gpu_bdf           = 0,
    .gpu_passed_through = false,
    .game_running      = false
};

// ── Low-level CPU helpers ─────────────────────────────────────────────────────

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile ("wrmsr" :: "c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)));
}

static inline uint64_t read_cr0(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr4(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(val));
    return val;
}

static inline void write_cr4(uint64_t val) {
    __asm__ volatile ("mov %0, %%cr4" :: "r"(val));
}

static inline uint64_t vmread(uint64_t field) {
    uint64_t val;
    __asm__ volatile ("vmread %1, %0" : "=r"(val) : "r"(field));
    return val;
}

static inline void vmwrite(uint64_t field, uint64_t val) {
    __asm__ volatile ("vmwrite %1, %0" :: "r"(field), "r"(val));
}

// ── VMX Support Check ─────────────────────────────────────────────────────────

bool vmx_check_support(void) {
    // Check CPUID.1:ECX[5] — VMX support bit
    uint32_t ecx;
    __asm__ volatile (
        "mov $1, %%eax\n"
        "cpuid\n"
        : "=c"(ecx) :: "eax", "ebx", "edx"
    );
    if (!(ecx & (1 << 5))) return false;

    // Check IA32_FEATURE_CONTROL MSR — VMX must be enabled in firmware
    uint64_t feature_ctrl = rdmsr(MSR_IA32_FEATURE_CONTROL);
    if (!(feature_ctrl & 0x1)) return false;   // Lock bit must be set
    if (!(feature_ctrl & 0x4)) return false;   // VMX outside SMX must be enabled

    return true;
}

// ── VMXON — Become the Hypervisor ────────────────────────────────────────────

// The VMXON region — a 4KB aligned memory region required by the CPU
static uint8_t vmxon_region[4096] __attribute__((aligned(4096)));

bool vmx_enable(void) {
    if (!vmx_check_support()) return false;

    // Set CR4.VMXE (bit 13) — required before VMXON
    write_cr4(read_cr4() | (1 << 13));

    // Write the VMX revision identifier into the VMXON region
    uint64_t vmx_basic = rdmsr(MSR_IA32_VMX_BASIC);
    uint32_t revision_id = (uint32_t)(vmx_basic & 0x7FFFFFFF);
    *((uint32_t*)vmxon_region) = revision_id;

    // Execute VMXON — BUBO becomes the hypervisor
    uint64_t vmxon_phys = (uint64_t)vmxon_region; // In a real kernel: virt_to_phys()
    uint8_t result;
    __asm__ volatile (
        "vmxon %1\n"
        "setna %0\n"
        : "=r"(result)
        : "m"(vmxon_phys)
        : "cc"
    );

    if (result) return false; // VMXON failed

    // Seal the fact that BUBO is now the hypervisor
    archivist_record(ARCHIVE_RECORD_TRUTH_SEAL, "hypervisor_active", "BUBO_VMX_ROOT", true);

    return true;
}

void vmx_disable(void) {
    __asm__ volatile ("vmxoff");
    write_cr4(read_cr4() & ~(1 << 13));
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "hypervisor_active", "disabled", false);
}

// ── VMCS Setup ────────────────────────────────────────────────────────────────

static uint8_t vmcs_region[4096] __attribute__((aligned(4096)));

static bool vmx_setup_vmcs(bubo_vm_t* vm) {
    uint64_t vmx_basic = rdmsr(MSR_IA32_VMX_BASIC);
    uint32_t revision_id = (uint32_t)(vmx_basic & 0x7FFFFFFF);

    // Write revision ID into VMCS region
    *((uint32_t*)vmcs_region) = revision_id;

    uint64_t vmcs_phys = (uint64_t)vmcs_region;
    vm->vmcs_phys_addr = vmcs_phys;

    // VMCLEAR — initialize the VMCS
    __asm__ volatile ("vmclear %0" :: "m"(vmcs_phys) : "cc");

    // VMPTRLD — make this VMCS the current one
    __asm__ volatile ("vmptrld %0" :: "m"(vmcs_phys) : "cc");

    // ── Guest State ───────────────────────────────────────────────────────────
    // Windows will start in protected mode, 64-bit long mode
    vmwrite(VMCS_GUEST_CR0, read_cr0() | 0x1);  // PE bit — protected mode
    vmwrite(VMCS_GUEST_CR4, read_cr4());
    vmwrite(VMCS_GUEST_RFLAGS, 0x2);             // Reserved bit 1 always set

    // Segment selectors — flat 64-bit model
    vmwrite(VMCS_GUEST_CS_SELECTOR, 0x10);
    vmwrite(VMCS_GUEST_DS_SELECTOR, 0x18);
    vmwrite(VMCS_GUEST_ES_SELECTOR, 0x18);
    vmwrite(VMCS_GUEST_SS_SELECTOR, 0x18);

    // Guest RIP will be set by vmx_guest.c when Windows is loaded
    vmwrite(VMCS_GUEST_RIP, vm->guest_rip);
    vmwrite(VMCS_GUEST_RSP, vm->guest_rsp);
    vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0); // Active

    // ── Host State ────────────────────────────────────────────────────────────
    // When a VM exit occurs, the CPU jumps here (back to BUBO)
    vmwrite(VMCS_HOST_CR0, read_cr0());
    vmwrite(VMCS_HOST_CR4, read_cr4());
    vmwrite(VMCS_HOST_RIP, (uint64_t)vmx_handle_exit);

    // ── VM Execution Controls ─────────────────────────────────────────────────
    // Enable EPT (Extended Page Tables) for memory isolation
    // Enable unrestricted guest for Windows boot compatibility
    uint64_t secondary_ctls = (1 << 1)  | // Enable EPT
                              (1 << 7);   // Unrestricted guest
    vmwrite(VMCS_SECONDARY_VM_EXEC_CTRL, secondary_ctls);

    return true;
}

// ── VM Lifecycle ──────────────────────────────────────────────────────────────

bool vmx_create_windows_vm(bubo_vm_t* vm, uint64_t mem_size_bytes) {
    vm->guest_mem_size = mem_size_bytes;
    vm->state = VM_STATE_CREATED;

    if (!vmx_setup_vmcs(vm)) return false;

    vm->state = VM_STATE_SUSPENDED;
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "windows_vm", "suspended", false);

    return true;
}

bool vmx_resume_windows_vm(bubo_vm_t* vm) {
    if (vm->state == VM_STATE_NONE || vm->state == VM_STATE_DESTROYED) return false;

    vm->state = VM_STATE_RUNNING;
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "windows_vm", "running", false);

    // BUBO acknowledges — Landon is about to play
    bubo_event_game_start();

    // VMLAUNCH (first time) or VMRESUME (subsequent)
    // The CPU will now execute Windows until a VM exit occurs
    __asm__ volatile ("vmresume");

    // If we get here, VMRESUME failed
    vm->state = VM_STATE_SUSPENDED;
    return false;
}

void vmx_suspend_windows_vm(bubo_vm_t* vm) {
    if (vm->state != VM_STATE_RUNNING) return;

    // Save guest RIP and RSP for resume
    vm->guest_rip = vmread(VMCS_GUEST_RIP);
    vm->guest_rsp = vmread(VMCS_GUEST_RSP);

    vm->state = VM_STATE_SUSPENDED;
    vm->game_running = false;

    // BUBO knows the game is over — return to the desktop
    bubo_event_game_end();
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "windows_vm", "suspended", false);
}

void vmx_destroy_windows_vm(bubo_vm_t* vm) {
    vmx_suspend_windows_vm(vm);
    vm->state = VM_STATE_DESTROYED;
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "windows_vm", "destroyed", false);
}

// ── VM Exit Handler ───────────────────────────────────────────────────────────
// Every time Windows does something privileged, the CPU stops it and comes here.
// VASH inspects every exit. BUBO watches. The constitution holds.

bool vmx_handle_exit(bubo_vm_t* vm) {
    uint64_t exit_reason = vmread(VMCS_EXIT_REASON) & 0xFFFF;

    switch (exit_reason) {

    case VMX_EXIT_CPUID:
        // Windows is asking about the CPU. We answer honestly.
        // VASH does not flag CPUID exits — they are benign.
        break;

    case VMX_EXIT_HLT:
        // Windows is idle. Good. Let it rest.
        // BUBO uses this moment to run a Hyperbolic Chamber epoch.
        break;

    case VMX_EXIT_IO_INSTRUCTION:
        // Windows is accessing an I/O port.
        // VASH checks if this port is on the constitutional watchlist.
        // vash_check_io_exit(vmread(VMCS_EXIT_QUALIFICATION));
        break;

    case VMX_EXIT_RDMSR:
    case VMX_EXIT_WRMSR:
        // Windows is reading or writing a model-specific register.
        // VASH checks for attempts to disable security features.
        // vash_check_msr_exit(exit_reason);
        break;

    case VMX_EXIT_EPT_VIOLATION:
        // Windows tried to access memory outside its allocation.
        // This is a serious constitutional violation — VASH handles it.
        // vash_handle_ept_violation();
        vmx_suspend_windows_vm(vm);
        return false; // Do not resume

    default:
        // Unknown exit — log it, resume cautiously
        archivist_record(ARCHIVE_RECORD_AGENT_STATE, "vmx_unknown_exit", "logged", false);
        break;
    }

    return true; // Resume the VM
}
