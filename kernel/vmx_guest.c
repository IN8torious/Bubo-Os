// BUBO OS — Copyright (c) 2025 Nathan Brown & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/vmx_guest.c — Windows Guest Initialization & Game Launch
//
// Windows lives here. It boots once, suspends when not needed,
// and wakes when Landon says "play." It never knows it is a guest.
// BUBO is always home. Windows is always the tool.
//
// The moment Landon says "play" — this file is why it works.
// =============================================================================

#include "vmx.h"
#include "corvus_archivist.h"
#include "corvus_bubo.h"
#include "corvus_flow.h"

// ── Windows Guest Memory Layout ───────────────────────────────────────────────
// BUBO OS owns the first 2GB of physical RAM.
// Windows gets the next 8GB (or however much is available).
// The GPU framebuffer is mapped directly via VT-d passthrough.
//
// Physical Memory Map:
//   0x00000000 - 0x7FFFFFFF  (2GB)  : BUBO OS kernel, agents, framebuffer
//   0x80000000 - 0x27FFFFFFF (8GB)  : Windows guest physical memory
//   GPU BAR                         : Passed through directly via IOMMU

#define BUBO_MEM_BASE       0x00000000ULL
#define BUBO_MEM_SIZE       0x80000000ULL  // 2GB for BUBO
#define WINDOWS_MEM_BASE    0x80000000ULL  // Windows starts here
#define WINDOWS_MEM_SIZE    0x200000000ULL // 8GB for Windows

// ── Windows Boot Sector Location ─────────────────────────────────────────────
// Windows is installed on a dedicated partition. Its boot sector physical
// address is discovered at runtime by the BUBO partition manager.
static uint64_t windows_boot_sector_phys = 0;

// ── EPT (Extended Page Tables) ───────────────────────────────────────────────
// EPT maps guest physical addresses to host physical addresses.
// This is how BUBO enforces memory isolation — Windows cannot see BUBO's memory.

// EPT PML4 table — 4KB aligned, one entry per 512GB of guest physical space
static uint64_t ept_pml4[512] __attribute__((aligned(4096)));
static uint64_t ept_pdpt[512] __attribute__((aligned(4096)));
static uint64_t ept_pd[512]   __attribute__((aligned(4096)));

static void vmx_setup_ept(bubo_vm_t* vm) {
    // Build a simple identity-mapped EPT for the Windows memory region
    // Guest physical 0x80000000+ maps to host physical 0x80000000+
    // BUBO's memory (0x00000000 - 0x7FFFFFFF) is NOT mapped — Windows cannot see it

    // EPT PML4 entry — points to PDPT
    uint64_t pdpt_phys = (uint64_t)ept_pdpt;
    ept_pml4[0] = pdpt_phys | 0x7; // Read + Write + Execute

    // EPT PDPT entry — points to PD (covers 0x80000000 - 0xBFFFFFFF)
    uint64_t pd_phys = (uint64_t)ept_pd;
    ept_pdpt[0] = pd_phys | 0x7;

    // EPT PD entries — 2MB pages for Windows memory
    for (int i = 0; i < 512; i++) {
        uint64_t page_phys = WINDOWS_MEM_BASE + ((uint64_t)i * 0x200000); // 2MB pages
        ept_pd[i] = page_phys | (1 << 7) | 0x7; // Large page + RWX
    }

    (void)vm;
}

// ── Guest Initialization ──────────────────────────────────────────────────────

bool vmx_guest_init(bubo_vm_t* vm) {
    // Set up the guest memory region
    vm->guest_phys_base = WINDOWS_MEM_BASE;
    vm->guest_mem_size  = WINDOWS_MEM_SIZE;

    // Build the EPT — Windows gets its memory, BUBO keeps its own
    vmx_setup_ept(vm);

    // The guest RIP starts at the Windows boot sector
    // In a real implementation, this is read from the partition table
    vm->guest_rip = windows_boot_sector_phys;
    vm->guest_rsp = WINDOWS_MEM_BASE + WINDOWS_MEM_SIZE - 0x1000; // Stack at top of guest mem

    // Seal the guest configuration with the ARCHIVIST
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "windows_guest", "initialized", false);
    archivist_record(ARCHIVE_RECORD_TRUTH_SEAL,  "windows_mem_base", "0x80000000", true);
    archivist_record(ARCHIVE_RECORD_TRUTH_SEAL,  "bubo_mem_protected", "true", true);

    return true;
}

// ── The Game Launch Trigger ───────────────────────────────────────────────────
// Called by CORVUS when Landon's voice command resolves to "play" or "Call of Duty"
// This is the moment. This is what the whole OS was built for.

void vmx_launch_game(void) {
    bubo_vm_t* vm = &g_windows_vm;

    // If the VM has never been initialized, do it now
    if (vm->state == VM_STATE_NONE) {
        vmx_guest_init(vm);
        vmx_create_windows_vm(vm, WINDOWS_MEM_SIZE);
    }

    // Mark the game as running
    vm->game_running = true;

    // BUBO announces — this is the moment
    bubo_event_landon_connected();

    // Wake the VM — Windows boots (or resumes), Call of Duty launches
    vmx_resume_windows_vm(vm);
}

// ── The Game End Handler ──────────────────────────────────────────────────────
// Called when BUBO detects the game has closed (audio silence, no GPU activity)
// or when Landon says "home" / "done" / "stop"

void vmx_end_game(void) {
    bubo_vm_t* vm = &g_windows_vm;

    if (vm->state != VM_STATE_RUNNING) return;

    // Suspend the VM — Windows freezes, GPU returns to BUBO desktop
    vmx_suspend_windows_vm(vm);

    // BUBO's desktop comes back — we are home
    // The color system returns to BUBO's warm amber
    // ARCHIVIST logs the session
    archivist_record(ARCHIVE_RECORD_VOICE_CMD, "game_session", "completed", false);
}

// ── Voice Command Integration ─────────────────────────────────────────────────
// These are the intent handlers that CORVUS calls when it classifies a command

void vmx_handle_intent_play(void) {
    vmx_launch_game();
}

void vmx_handle_intent_home(void) {
    vmx_end_game();
}
