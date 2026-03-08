// =============================================================================
// BUBO OS — Hardware Detection Layer
// Copyright (c) 2025 N8torious AI. MIT License.
// Built by Nathan Brown for Landon Pankuch.
// NO MAS DISADVANTAGED.
//
// BUBO OS detects the hardware it's running on at boot and adapts.
// No machine gets left behind. This is a standard, not a feature.
//
// Detects:
//   - CPU: vendor, model, core count, clock speed, SIMD capabilities
//   - RAM: total available memory from multiboot2 memory map
//   - GPU: PCI scan for known GPU vendors, VRAM estimate
//   - Storage: USB/HDD/SSD via AHCI/IDE detection
//   - Display: framebuffer resolution from GRUB/multiboot2
//
// Outputs a HardwareProfile that the rest of the OS uses to scale features.
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── CPU feature flags ─────────────────────────────────────────────────────────
struct CPUFeatures {
    bool has_sse;       // SSE — Pentium III+
    bool has_sse2;      // SSE2 — Pentium 4+
    bool has_sse3;      // SSE3 — Core 2+
    bool has_ssse3;     // SSSE3 — Core 2 Penryn+
    bool has_sse4_1;    // SSE4.1 — Core 2 Penryn+
    bool has_sse4_2;    // SSE4.2 — Nehalem+
    bool has_avx;       // AVX — Sandy Bridge+
    bool has_avx2;      // AVX2 — Haswell+
    bool has_avx512;    // AVX-512 — Skylake-X+
    bool has_aes;       // AES-NI — Westmere+
    bool has_rdrand;    // RDRAND — Ivy Bridge+
    bool has_nx;        // NX/XD bit
    bool has_x2apic;    // x2APIC — multi-core
    bool has_hypervisor; // Running in a VM
};

// ── CPU info ──────────────────────────────────────────────────────────────────
struct CPUInfo {
    char     vendor[13];     // "GenuineIntel" or "AuthenticAMD"
    char     brand[49];      // Full CPU brand string
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint32_t logical_cores;  // Logical core count
    uint32_t physical_cores; // Physical core count
    uint32_t base_mhz;       // Estimated base clock (MHz)
    CPUFeatures features;
};

// ── GPU tier ──────────────────────────────────────────────────────────────────
enum class GPUTier : uint8_t {
    UNKNOWN        = 0,
    SOFTWARE_ONLY  = 1,  // No GPU — pure software renderer
    INTEGRATED_OLD = 2,  // Intel GMA, old integrated — very limited
    INTEGRATED     = 3,  // Intel HD/UHD, AMD Vega iGPU — decent
    DISCRETE_LOW   = 4,  // GTX 750, RX 560 — low-end discrete
    DISCRETE_MID   = 5,  // GTX 1060, RX 580 — mid-range
    DISCRETE_HIGH  = 6,  // RTX 3070+, RX 6800+ — high-end
    DISCRETE_ULTRA = 7,  // RTX 4090, RX 7900 XTX — ultra
};

// ── GPU info ──────────────────────────────────────────────────────────────────
struct GPUInfo {
    GPUTier  tier;
    char     name[32];
    uint32_t vram_mb;       // Estimated VRAM in MB
    uint16_t pci_vendor;    // PCI vendor ID
    uint16_t pci_device;    // PCI device ID
    bool     has_vulkan;    // Vulkan capable (estimated from GPU tier)
    bool     has_opengl;    // OpenGL capable
};

// ── RAM info ──────────────────────────────────────────────────────────────────
struct RAMInfo {
    uint64_t total_bytes;   // Total RAM
    uint64_t usable_bytes;  // Usable RAM (excluding reserved regions)
    uint32_t total_mb;      // Total RAM in MB (convenient)
};

// ── Hardware tier — overall system classification ─────────────────────────────
enum class HWTier : uint8_t {
    POTATO    = 0,  // < 1GB RAM, no SSE2, software GPU — absolute minimum
    LOW       = 1,  // 1-2GB RAM, SSE2, integrated GPU — Landon's minimum target
    MID       = 2,  // 2-4GB RAM, SSE4, integrated/low discrete
    HIGH      = 3,  // 4-8GB RAM, AVX, mid discrete
    ULTRA     = 4,  // 8GB+ RAM, AVX2, high discrete
};

// ── Full hardware profile ─────────────────────────────────────────────────────
struct HardwareProfile {
    CPUInfo  cpu;
    GPUInfo  gpu;
    RAMInfo  ram;
    HWTier   tier;          // Overall system tier
    uint32_t fb_width;      // Framebuffer width
    uint32_t fb_height;     // Framebuffer height
    uint32_t fb_bpp;        // Bits per pixel
    bool     is_vm;         // Running in a virtual machine
    bool     is_low_power;  // Laptop/embedded — power-conscious mode
    char     summary[128];  // Human-readable summary for boot screen
};

// ── Global hardware profile ───────────────────────────────────────────────────
extern HardwareProfile& g_hw;

// ── C API ─────────────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

// Call once at boot with the multiboot2 info address
void hwdetect_init(uint32_t multiboot_info_addr);

// Get tier name as string
const char* hwdetect_tier_name(void);
const char* hwdetect_gpu_tier_name(void);

// Feature queries
bool hwdetect_has_sse2(void);
bool hwdetect_has_avx(void);
bool hwdetect_is_low_spec(void);   // true if tier <= LOW
bool hwdetect_is_vm(void);

// Print hardware summary to terminal
void hwdetect_print_summary(void);

#ifdef __cplusplus
}
#endif
