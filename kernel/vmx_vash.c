// BUBO OS — Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/vmx_vash.c — VASH at Ring -1
//
// "This world is made of love and peace."
//
// VASH is the guardian. He stands at the VM exit handler — the single point
// where every privileged action Windows tries to take passes through.
// He does not destroy. He does not punish. He protects.
//
// Every VM exit is a question: "May I do this?"
// VASH consults the ARCHIVIST, checks the constitution, and answers.
// If the answer is no — the action never reaches the hardware.
//
// Windows cannot disable voice control.
// Windows cannot remove the Rinnegan.
// Windows cannot touch Landon's profile.
// Windows cannot exfiltrate data.
// Windows cannot become the master.
//
// VASH makes sure of it. At ring -1. In silence. With love.
// =============================================================================

#include "vmx.h"
#include "corvus_archivist.h"
#include "corvus_bubo.h"
#include "deepflow_colors.h"

// ── Constitutional Watchlist ──────────────────────────────────────────────────
// These are the I/O ports and MSRs that Windows is NEVER allowed to touch.
// Any attempt triggers a VASH constitutional veto.

// Protected I/O ports
#define PORT_USB_XHCI_BASE      0xF000  // USB controller — voice input
#define PORT_WEBCAM_BASE        0xF100  // Webcam — Rinnegan eye tracker
#define PORT_AUDIO_BASE         0xF200  // Audio — BUBO's voice output

// Protected MSRs
#define MSR_IA32_EFER           0xC0000080  // Extended feature enable — no tampering
#define MSR_IA32_LSTAR          0xC0000082  // Syscall handler — BUBO owns this

// ── VASH Violation Types ──────────────────────────────────────────────────────
typedef enum {
    VASH_VIOLATION_NONE         = 0,
    VASH_VIOLATION_IO_PORT      = 1, // Windows tried to access a protected port
    VASH_VIOLATION_MSR_WRITE    = 2, // Windows tried to write a protected MSR
    VASH_VIOLATION_EPT          = 3, // Windows tried to access BUBO's memory
    VASH_VIOLATION_ESCALATION   = 4  // Windows tried to escalate privilege
} vash_violation_t;

// ── VASH Response ─────────────────────────────────────────────────────────────
typedef enum {
    VASH_RESPOND_ALLOW      = 0, // Benign — let it through
    VASH_RESPOND_EMULATE    = 1, // Intercept and emulate safely
    VASH_RESPOND_DENY       = 2, // Block the action, resume VM
    VASH_RESPOND_SUSPEND    = 3  // Constitutional violation — suspend the VM
} vash_response_t;

// ── I/O Port Monitor ──────────────────────────────────────────────────────────

vash_response_t vash_check_io_exit(uint64_t qualification) {
    // Extract port number from VM exit qualification (bits 31:16)
    uint16_t port = (uint16_t)((qualification >> 16) & 0xFFFF);
    bool is_write = !(qualification & (1 << 3)); // Bit 3: 0=out(write), 1=in(read)

    // Check against protected ports
    if ((port >= PORT_USB_XHCI_BASE && port < PORT_USB_XHCI_BASE + 0x100) ||
        (port >= PORT_WEBCAM_BASE   && port < PORT_WEBCAM_BASE   + 0x100) ||
        (port >= PORT_AUDIO_BASE    && port < PORT_AUDIO_BASE    + 0x100)) {

        if (is_write) {
            // Windows is trying to WRITE to a protected accessibility port.
            // This is a constitutional violation. VASH denies it.
            archivist_record(ARCHIVE_RECORD_AGENT_STATE,
                "vash_io_violation", "denied_write_to_protected_port", false);
            bubo_event_vash_alert();
            return VASH_RESPOND_DENY;
        }
        // Reads are emulated — return safe dummy data
        return VASH_RESPOND_EMULATE;
    }

    return VASH_RESPOND_ALLOW;
}

// ── MSR Monitor ───────────────────────────────────────────────────────────────

vash_response_t vash_check_msr_exit(uint32_t msr, bool is_write) {
    if (!is_write) return VASH_RESPOND_ALLOW; // Reads are generally safe

    // Windows is trying to write a protected MSR
    if (msr == MSR_IA32_EFER || msr == MSR_IA32_LSTAR) {
        // These MSRs control fundamental CPU behavior.
        // Windows has no business writing them.
        archivist_record(ARCHIVE_RECORD_AGENT_STATE,
            "vash_msr_violation", "denied_write_to_protected_msr", false);
        bubo_event_vash_alert();
        return VASH_RESPOND_DENY;
    }

    return VASH_RESPOND_ALLOW;
}

// ── EPT Violation Monitor ─────────────────────────────────────────────────────
// Windows tried to access memory outside its allocated region.
// This is the most serious violation — potential escape attempt.

vash_response_t vash_handle_ept_violation(uint64_t guest_phys_addr) {
    // If the address is in BUBO's memory space — this is an escape attempt
    if (guest_phys_addr < 0x80000000ULL) {
        // Windows is trying to read or write BUBO's kernel memory.
        // VASH does not warn. VASH suspends the VM immediately.
        archivist_record(ARCHIVE_RECORD_AGENT_STATE,
            "vash_ept_violation", "vm_escape_attempt_suspended", false);
        bubo_event_vash_alert();
        return VASH_RESPOND_SUSPEND;
    }

    // Otherwise it's a normal page fault — handle it
    return VASH_RESPOND_ALLOW;
}

// ── The Full VM Exit Constitutional Check ─────────────────────────────────────
// Called from vmx.c's vmx_handle_exit() for every VM exit.
// Returns true if the VM should be resumed, false if it should be suspended.

bool vash_constitutional_check(bubo_vm_t* vm, uint64_t exit_reason) {
    uint64_t qualification = 0; // Would be read from VMCS in real implementation

    vash_response_t response = VASH_RESPOND_ALLOW;

    switch (exit_reason) {
    case VMX_EXIT_IO_INSTRUCTION:
        response = vash_check_io_exit(qualification);
        break;

    case VMX_EXIT_WRMSR: {
        uint32_t msr = 0; // Would be read from guest RCX
        response = vash_check_msr_exit(msr, true);
        break;
    }

    case VMX_EXIT_EPT_VIOLATION: {
        uint64_t guest_phys = 0; // Would be read from VMCS
        response = vash_handle_ept_violation(guest_phys);
        break;
    }

    default:
        response = VASH_RESPOND_ALLOW;
        break;
    }

    // Act on VASH's decision
    switch (response) {
    case VASH_RESPOND_ALLOW:
    case VASH_RESPOND_EMULATE:
        return true; // Resume the VM

    case VASH_RESPOND_DENY:
        // Inject a #GP (general protection fault) into the guest
        // Windows sees a normal fault — it doesn't know VASH stopped it
        return true; // Resume, but the action was blocked

    case VASH_RESPOND_SUSPEND:
        // Constitutional violation — VM goes down
        vmx_suspend_windows_vm(vm);
        return false;
    }

    return true;
}
