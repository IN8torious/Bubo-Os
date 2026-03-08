// BUBO OS — Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/vt_d.c — Intel VT-d IOMMU & GPU Passthrough
//
// The GPU is the bridge between BUBO OS and Call of Duty.
// When Landon is on the BUBO desktop, the GPU renders BUBO's color language.
// When Landon says "play," the GPU is handed to the Windows VM.
// When the game ends, the GPU comes home to BUBO.
//
// VT-d (Virtualization Technology for Directed I/O) makes this safe.
// The IOMMU ensures the GPU can only access memory it is supposed to.
// Windows gets the GPU. Windows does not get BUBO's memory.
// =============================================================================

#include "vmx.h"
#include "corvus_archivist.h"
#include "deepflow_colors.h"

// ── IOMMU Register Base ───────────────────────────────────────────────────────
// The IOMMU MMIO base address is found in the ACPI DMAR table at boot.
// For now we use a placeholder — the real address is discovered at runtime.
static volatile uint64_t* iommu_base = (volatile uint64_t*)0xFED90000ULL;

// IOMMU Register Offsets (Intel VT-d Spec)
#define IOMMU_VER_REG       0x000  // Version register
#define IOMMU_CAP_REG       0x008  // Capability register
#define IOMMU_GCMD_REG      0x018  // Global command register
#define IOMMU_GSTS_REG      0x01C  // Global status register
#define IOMMU_RTADDR_REG    0x020  // Root table address register
#define IOMMU_CCMD_REG      0x028  // Context command register
#define IOMMU_FSTS_REG      0x034  // Fault status register

// GCMD bits
#define IOMMU_GCMD_TE       (1U << 31)  // Translation enable
#define IOMMU_GCMD_SRTP     (1U << 30)  // Set root table pointer

// ── Root and Context Tables ───────────────────────────────────────────────────
// The IOMMU uses a two-level table structure to map DMA transactions.
// Root table: 256 entries (one per PCI bus)
// Context table: 256 entries per bus (one per device/function)

typedef struct {
    uint64_t lo;
    uint64_t hi;
} iommu_entry_t;

static iommu_entry_t root_table[256]      __attribute__((aligned(4096)));
static iommu_entry_t context_table[256]   __attribute__((aligned(4096)));

// ── GPU State ─────────────────────────────────────────────────────────────────
typedef enum {
    GPU_OWNER_BUBO    = 0,  // GPU renders BUBO desktop
    GPU_OWNER_WINDOWS = 1   // GPU is passed through to Windows VM
} gpu_owner_t;

static gpu_owner_t current_gpu_owner = GPU_OWNER_BUBO;
static uint32_t    gpu_bdf = 0; // Bus:Device:Function — discovered at boot

// ── IOMMU Initialization ──────────────────────────────────────────────────────

bool vtd_init(void) {
    // Verify IOMMU is present by reading version register
    uint64_t ver = iommu_base[IOMMU_VER_REG / 8];
    if ((ver & 0xF) == 0) return false; // No IOMMU

    // Set up root table
    uint64_t root_phys = (uint64_t)root_table;
    iommu_base[IOMMU_RTADDR_REG / 8] = root_phys | 0x1; // Present bit

    // Issue Set Root Table Pointer command
    iommu_base[IOMMU_GCMD_REG / 8] = IOMMU_GCMD_SRTP;

    // Wait for acknowledgment
    while (!(iommu_base[IOMMU_GSTS_REG / 8] & IOMMU_GCMD_SRTP));

    // Enable translation
    iommu_base[IOMMU_GCMD_REG / 8] = IOMMU_GCMD_TE;
    while (!(iommu_base[IOMMU_GSTS_REG / 8] & IOMMU_GCMD_TE));

    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "iommu", "enabled", false);
    return true;
}

// ── GPU Discovery ─────────────────────────────────────────────────────────────
// Scans the PCI bus for a GPU (class 0x03, subclass 0x00 or 0x02)

uint32_t vtd_find_gpu(void) {
    // PCI config space access via I/O ports 0xCF8 / 0xCFC
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t dev = 0; dev < 32; dev++) {
            for (uint32_t fn = 0; fn < 8; fn++) {
                uint32_t addr = 0x80000000 |
                                (bus << 16) | (dev << 11) | (fn << 8) | 0x08;

                // Write address to CONFIG_ADDRESS
                __asm__ volatile ("outl %0, %1" :: "a"(addr), "Nd"((uint16_t)0xCF8));

                uint32_t class_rev;
                __asm__ volatile ("inl %1, %0" : "=a"(class_rev) : "Nd"((uint16_t)0xCFC));

                uint8_t class_code = (class_rev >> 24) & 0xFF;
                if (class_code == 0x03) { // Display controller
                    gpu_bdf = (bus << 16) | (dev << 8) | fn;
                    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "gpu_found", "true", false);
                    return gpu_bdf;
                }
            }
        }
    }
    return 0; // No GPU found
}

// ── GPU Passthrough ───────────────────────────────────────────────────────────
// Assigns the GPU to the Windows VM via IOMMU context table manipulation.
// After this call, Windows has direct hardware access to the GPU.
// BUBO loses GPU access until vtd_reclaim_gpu() is called.

bool vtd_passthrough_gpu_to_vm(bubo_vm_t* vm) {
    if (current_gpu_owner == GPU_OWNER_WINDOWS) return true; // Already passed through

    uint32_t bus = (gpu_bdf >> 16) & 0xFF;
    uint32_t dev = (gpu_bdf >> 8)  & 0xFF;
    uint32_t fn  =  gpu_bdf        & 0xFF;

    // Set up context table entry for the GPU's BDF
    // This tells the IOMMU: DMA from this device goes to the guest's EPT
    uint64_t ctx_phys = (uint64_t)context_table;
    root_table[bus].lo = ctx_phys | 0x1; // Present

    // Context entry: map GPU DMA to Windows VM's physical memory space
    uint32_t ctx_idx = (dev << 3) | fn;
    context_table[ctx_idx].lo = vm->guest_phys_base | 0x1; // Present + guest base
    context_table[ctx_idx].hi = 0x1; // Domain ID 1 = Windows VM

    // Invalidate IOMMU context cache
    iommu_base[IOMMU_CCMD_REG / 8] = (1ULL << 63) | (1ULL << 61); // Global invalidate

    current_gpu_owner = GPU_OWNER_WINDOWS;
    vm->gpu_passed_through = true;
    vm->gpu_bdf = gpu_bdf;

    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "gpu_owner", "windows_vm", false);
    return true;
}

// ── GPU Reclaim ───────────────────────────────────────────────────────────────
// Takes the GPU back from the Windows VM and returns it to BUBO desktop.
// Called when the game ends and Landon returns home.

void vtd_reclaim_gpu(bubo_vm_t* vm) {
    if (current_gpu_owner == GPU_OWNER_BUBO) return; // Already ours

    uint32_t bus = (gpu_bdf >> 16) & 0xFF;
    uint32_t dev = (gpu_bdf >> 8)  & 0xFF;
    uint32_t fn  =  gpu_bdf        & 0xFF;

    // Clear the context table entry — GPU DMA is no longer mapped to the VM
    uint32_t ctx_idx = (dev << 3) | fn;
    context_table[ctx_idx].lo = 0;
    context_table[ctx_idx].hi = 0;
    root_table[bus].lo = 0;

    // Invalidate IOMMU context cache
    iommu_base[IOMMU_CCMD_REG / 8] = (1ULL << 63) | (1ULL << 61);

    current_gpu_owner = GPU_OWNER_BUBO;
    vm->gpu_passed_through = false;

    // BUBO desktop reinitializes the GPU for its own rendering
    // The color language comes back — warm amber, deep red, Landon's purple
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "gpu_owner", "bubo_desktop", false);
}

// ── Accessors ─────────────────────────────────────────────────────────────────

gpu_owner_t vtd_get_gpu_owner(void) {
    return current_gpu_owner;
}

bool vtd_is_gpu_available(void) {
    return current_gpu_owner == GPU_OWNER_BUBO;
}
