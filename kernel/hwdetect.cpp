// =============================================================================
// BUBO OS — Hardware Detection Implementation
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================

#include <new>           // freestanding placement new (bare-metal)
#include "hwdetect.hpp"
#include "vga.h"
#include <stdint.h>

// ── Global hardware profile ───────────────────────────────────────────────────
static uint8_t hw_storage[sizeof(HardwareProfile)];
HardwareProfile& g_hw = *reinterpret_cast<HardwareProfile*>(hw_storage);

// ── Port I/O ──────────────────────────────────────────────────────────────────
static inline uint32_t inl(uint16_t port) {
    uint32_t v;
    __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outl(uint16_t port, uint32_t v) {
    __asm__ volatile("outl %0, %1" : : "a"(v), "Nd"(port));
}

// ── CPUID helper ─────────────────────────────────────────────────────────────
static void cpuid(uint32_t leaf, uint32_t subleaf,
                  uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

// ── String helpers ────────────────────────────────────────────────────────────
static void memzero(void* p, uint32_t n) {
    uint8_t* b = (uint8_t*)p;
    for (uint32_t i = 0; i < n; i++) b[i] = 0;
}

static void memcopy(void* dst, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}

static uint32_t str_len(const char* s) {
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

static void str_copy(char* d, const char* s, uint32_t max) {
    uint32_t i = 0;
    while (s[i] && i < max-1) { d[i] = s[i]; i++; }
    d[i] = '\0';
}

static void str_append(char* d, const char* s, uint32_t max) {
    uint32_t dlen = str_len(d);
    uint32_t i = 0;
    while (s[i] && dlen + i < max-1) { d[dlen+i] = s[i]; i++; }
    d[dlen+i] = '\0';
}

static void u32_to_str(char* buf, uint32_t v) {
    if (v == 0) { buf[0]='0'; buf[1]='\0'; return; }
    char tmp[12]; int i = 0;
    while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

// ── CPU detection ─────────────────────────────────────────────────────────────
static void detect_cpu(void) {
    CPUInfo& cpu = g_hw.cpu;
    uint32_t eax, ebx, ecx, edx;

    // Vendor string
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    memcopy(cpu.vendor,     &ebx, 4);
    memcopy(cpu.vendor + 4, &edx, 4);
    memcopy(cpu.vendor + 8, &ecx, 4);
    cpu.vendor[12] = '\0';

    // Family/model/stepping
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    cpu.family   = ((eax >> 8) & 0xF) + ((eax >> 20) & 0xFF);
    cpu.model    = ((eax >> 4) & 0xF) | (((eax >> 16) & 0xF) << 4);
    cpu.stepping = eax & 0xF;

    // Logical core count (from APIC info)
    cpu.logical_cores = (ebx >> 16) & 0xFF;
    if (cpu.logical_cores == 0) cpu.logical_cores = 1;
    cpu.physical_cores = cpu.logical_cores; // Simplified

    // Feature flags from CPUID leaf 1
    cpu.features.has_sse      = (edx >> 25) & 1;
    cpu.features.has_sse2     = (edx >> 26) & 1;
    cpu.features.has_sse3     = (ecx >>  0) & 1;
    cpu.features.has_ssse3    = (ecx >>  9) & 1;
    cpu.features.has_sse4_1   = (ecx >> 19) & 1;
    cpu.features.has_sse4_2   = (ecx >> 20) & 1;
    cpu.features.has_aes      = (ecx >> 25) & 1;
    cpu.features.has_avx      = (ecx >> 28) & 1;
    cpu.features.has_rdrand   = (ecx >> 30) & 1;
    cpu.features.has_hypervisor = (ecx >> 31) & 1;
    cpu.features.has_nx       = (edx >> 20) & 1;
    cpu.features.has_x2apic   = (ecx >> 21) & 1;

    // AVX2 from leaf 7
    cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    cpu.features.has_avx2   = (ebx >> 5) & 1;
    cpu.features.has_avx512 = (ebx >> 16) & 1;

    // Brand string (leaves 0x80000002-0x80000004)
    uint32_t max_ext;
    cpuid(0x80000000, 0, &max_ext, &ebx, &ecx, &edx);
    if (max_ext >= 0x80000004) {
        uint32_t* brand = (uint32_t*)cpu.brand;
        cpuid(0x80000002, 0, brand+0, brand+1, brand+2, brand+3);
        cpuid(0x80000003, 0, brand+4, brand+5, brand+6, brand+7);
        cpuid(0x80000004, 0, brand+8, brand+9, brand+10, brand+11);
        cpu.brand[48] = '\0';
    } else {
        str_copy(cpu.brand, cpu.vendor, sizeof(cpu.brand));
    }

    // VM detection
    g_hw.is_vm = cpu.features.has_hypervisor;
}

// ── PCI GPU detection ─────────────────────────────────────────────────────────
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
                    (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}

static void detect_gpu(void) {
    GPUInfo& gpu = g_hw.gpu;
    gpu.tier = GPUTier::SOFTWARE_ONLY;
    str_copy(gpu.name, "Unknown", sizeof(gpu.name));

    // Scan PCI bus for display controllers (class 0x03)
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0x00);
            if (id == 0xFFFFFFFF) continue;

            uint32_t class_rev = pci_read(bus, dev, 0, 0x08);
            uint8_t class_code = (class_rev >> 24) & 0xFF;
            if (class_code != 0x03) continue; // Not a display controller

            uint16_t vendor = id & 0xFFFF;
            uint16_t device = (id >> 16) & 0xFFFF;
            gpu.pci_vendor = vendor;
            gpu.pci_device = device;

            // Classify by vendor and device ID
            if (vendor == 0x8086) { // Intel
                if (device >= 0x4C00) {
                    gpu.tier = GPUTier::INTEGRATED;
                    str_copy(gpu.name, "Intel Xe / UHD", sizeof(gpu.name));
                    gpu.vram_mb = 512; // Shared RAM
                } else if (device >= 0x0100) {
                    gpu.tier = GPUTier::INTEGRATED;
                    str_copy(gpu.name, "Intel HD Graphics", sizeof(gpu.name));
                    gpu.vram_mb = 256;
                } else {
                    gpu.tier = GPUTier::INTEGRATED_OLD;
                    str_copy(gpu.name, "Intel GMA", sizeof(gpu.name));
                    gpu.vram_mb = 128;
                }
            } else if (vendor == 0x10DE) { // NVIDIA
                if (device >= 0x2200) {
                    gpu.tier = GPUTier::DISCRETE_ULTRA;
                    str_copy(gpu.name, "NVIDIA RTX 30/40 Series", sizeof(gpu.name));
                    gpu.vram_mb = 8192;
                } else if (device >= 0x1E00) {
                    gpu.tier = GPUTier::DISCRETE_HIGH;
                    str_copy(gpu.name, "NVIDIA RTX 20 Series", sizeof(gpu.name));
                    gpu.vram_mb = 8192;
                } else if (device >= 0x1B00) {
                    gpu.tier = GPUTier::DISCRETE_MID;
                    str_copy(gpu.name, "NVIDIA GTX 10 Series", sizeof(gpu.name));
                    gpu.vram_mb = 4096;
                } else if (device >= 0x1200) {
                    gpu.tier = GPUTier::DISCRETE_LOW;
                    str_copy(gpu.name, "NVIDIA GTX 700/900", sizeof(gpu.name));
                    gpu.vram_mb = 2048;
                } else {
                    gpu.tier = GPUTier::DISCRETE_LOW;
                    str_copy(gpu.name, "NVIDIA GPU (legacy)", sizeof(gpu.name));
                    gpu.vram_mb = 1024;
                }
            } else if (vendor == 0x1002) { // AMD
                if (device >= 0x7400) {
                    gpu.tier = GPUTier::DISCRETE_ULTRA;
                    str_copy(gpu.name, "AMD RX 7000 Series", sizeof(gpu.name));
                    gpu.vram_mb = 16384;
                } else if (device >= 0x6FDF) {
                    gpu.tier = GPUTier::DISCRETE_HIGH;
                    str_copy(gpu.name, "AMD RX 6000 Series", sizeof(gpu.name));
                    gpu.vram_mb = 8192;
                } else if (device >= 0x687F) {
                    gpu.tier = GPUTier::DISCRETE_MID;
                    str_copy(gpu.name, "AMD RX 500 Series", sizeof(gpu.name));
                    gpu.vram_mb = 4096;
                } else if (device >= 0x1500) {
                    gpu.tier = GPUTier::INTEGRATED;
                    str_copy(gpu.name, "AMD Vega iGPU", sizeof(gpu.name));
                    gpu.vram_mb = 512;
                } else {
                    gpu.tier = GPUTier::DISCRETE_LOW;
                    str_copy(gpu.name, "AMD GPU (legacy)", sizeof(gpu.name));
                    gpu.vram_mb = 1024;
                }
            }

            gpu.has_vulkan  = (uint8_t)gpu.tier >= (uint8_t)GPUTier::INTEGRATED;
            gpu.has_opengl  = (uint8_t)gpu.tier >= (uint8_t)GPUTier::INTEGRATED_OLD;
            return; // Found primary GPU
        }
    }
}

// ── RAM detection from multiboot2 memory map ──────────────────────────────────
static void detect_ram(uint32_t mb_addr) {
    RAMInfo& ram = g_hw.ram;
    ram.total_bytes  = 0;
    ram.usable_bytes = 0;

    if (!mb_addr) {
        ram.total_bytes  = 512ULL * 1024 * 1024; // Assume 512MB minimum
        ram.usable_bytes = ram.total_bytes;
        ram.total_mb     = 512;
        return;
    }

    // Walk multiboot2 tags
    uint8_t* p = (uint8_t*)(mb_addr + 8);
    uint8_t* end = (uint8_t*)mb_addr + *(uint32_t*)mb_addr;

    while (p < end) {
        uint32_t type = *(uint32_t*)p;
        uint32_t size = *(uint32_t*)(p + 4);
        if (type == 0) break; // End tag

        if (type == 6) { // Memory map tag
            uint32_t entry_size = *(uint32_t*)(p + 8);
            uint8_t* entry = p + 16;
            while (entry < p + size) {
                uint64_t base = *(uint64_t*)entry;
                uint64_t len  = *(uint64_t*)(entry + 8);
                uint32_t etype = *(uint32_t*)(entry + 16);
                ram.total_bytes += len;
                if (etype == 1) ram.usable_bytes += len; // Available RAM
                (void)base;
                entry += entry_size;
            }
        }

        // Align to 8 bytes
        p += (size + 7) & ~7u;
    }

    if (ram.total_bytes == 0) {
        ram.total_bytes  = 512ULL * 1024 * 1024;
        ram.usable_bytes = ram.total_bytes;
    }
    ram.total_mb = (uint32_t)(ram.total_bytes / (1024 * 1024));
}

// ── Overall tier calculation ──────────────────────────────────────────────────
static HWTier calculate_tier(void) {
    uint32_t ram_mb = g_hw.ram.total_mb;
    GPUTier  gpu    = g_hw.gpu.tier;
    bool     sse2   = g_hw.cpu.features.has_sse2;
    bool     avx    = g_hw.cpu.features.has_avx;
    bool     avx2   = g_hw.cpu.features.has_avx2;

    if (!sse2 || ram_mb < 512 || gpu <= GPUTier::SOFTWARE_ONLY)
        return HWTier::POTATO;
    if (ram_mb < 2048 || gpu <= GPUTier::INTEGRATED_OLD)
        return HWTier::LOW;
    if (!avx || ram_mb < 4096 || gpu <= GPUTier::INTEGRATED)
        return HWTier::MID;
    if (!avx2 || ram_mb < 8192 || gpu <= GPUTier::DISCRETE_LOW)
        return HWTier::HIGH;
    return HWTier::ULTRA;
}

// ── Build summary string ──────────────────────────────────────────────────────
static void build_summary(void) {
    char* s = g_hw.summary;
    s[0] = '\0';
    str_append(s, g_hw.cpu.brand[0] ? g_hw.cpu.brand : g_hw.cpu.vendor, 128);
    str_append(s, " | ", 128);
    char tmp[16]; u32_to_str(tmp, g_hw.ram.total_mb);
    str_append(s, tmp, 128);
    str_append(s, "MB RAM | ", 128);
    str_append(s, g_hw.gpu.name, 128);
    str_append(s, " | Tier: ", 128);
    str_append(s, hwdetect_tier_name(), 128);
}

// ── Public API ────────────────────────────────────────────────────────────────
extern "C" void hwdetect_init(uint32_t multiboot_info_addr) {
    new(hw_storage) HardwareProfile();
    memzero(&g_hw, sizeof(HardwareProfile));

    detect_cpu();
    detect_gpu();
    detect_ram(multiboot_info_addr);

    g_hw.tier = calculate_tier();
    g_hw.is_low_power = false; // TODO: ACPI battery detection

    build_summary();
}

extern "C" const char* hwdetect_tier_name(void) {
    switch (g_hw.tier) {
        case HWTier::POTATO: return "POTATO";
        case HWTier::LOW:    return "LOW";
        case HWTier::MID:    return "MID";
        case HWTier::HIGH:   return "HIGH";
        case HWTier::ULTRA:  return "ULTRA";
        default:             return "UNKNOWN";
    }
}

extern "C" const char* hwdetect_gpu_tier_name(void) {
    switch (g_hw.gpu.tier) {
        case GPUTier::SOFTWARE_ONLY:  return "Software";
        case GPUTier::INTEGRATED_OLD: return "Integrated (old)";
        case GPUTier::INTEGRATED:     return "Integrated";
        case GPUTier::DISCRETE_LOW:   return "Discrete (low)";
        case GPUTier::DISCRETE_MID:   return "Discrete (mid)";
        case GPUTier::DISCRETE_HIGH:  return "Discrete (high)";
        case GPUTier::DISCRETE_ULTRA: return "Discrete (ultra)";
        default:                      return "Unknown";
    }
}

extern "C" bool hwdetect_has_sse2(void)     { return g_hw.cpu.features.has_sse2; }
extern "C" bool hwdetect_has_avx(void)      { return g_hw.cpu.features.has_avx; }
extern "C" bool hwdetect_is_low_spec(void)  { return (uint8_t)g_hw.tier <= (uint8_t)HWTier::LOW; }
extern "C" bool hwdetect_is_vm(void)        { return g_hw.is_vm; }

extern "C" void hwdetect_print_summary(void) {
    terminal_setcolor(vga_entry_color(VGA_CYAN, VGA_BLACK));
    terminal_writeline("  [HW] Hardware Detection:");
    terminal_setcolor(vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK));
    terminal_writeline(g_hw.summary);

    // Print CPU features
    char feat[64] = "  [CPU] ";
    if (g_hw.cpu.features.has_avx2)    str_append(feat, "AVX2 ", 64);
    else if (g_hw.cpu.features.has_avx) str_append(feat, "AVX ", 64);
    else if (g_hw.cpu.features.has_sse4_2) str_append(feat, "SSE4.2 ", 64);
    else if (g_hw.cpu.features.has_sse2)   str_append(feat, "SSE2 ", 64);
    if (g_hw.cpu.features.has_aes)     str_append(feat, "AES ", 64);
    if (g_hw.is_vm)                    str_append(feat, "VM ", 64);
    terminal_writeline(feat);

    terminal_setcolor(vga_entry_color(VGA_WHITE, VGA_BLACK));
}
