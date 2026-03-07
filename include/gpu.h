// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// gpu.h — NVIDIA GPU Driver Layer Header
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GPU_VENDOR_NONE   = 0,
    GPU_VENDOR_NVIDIA = 1,
    GPU_VENDOR_AMD    = 2,
    GPU_VENDOR_INTEL  = 3,
} gpu_vendor_t;

typedef enum {
    GPU_ARCH_UNKNOWN = 0,
    GPU_ARCH_FERMI   = 1,
    GPU_ARCH_KEPLER  = 2,
    GPU_ARCH_MAXWELL = 3,
    GPU_ARCH_PASCAL  = 4,
    GPU_ARCH_VOLTA   = 5,
    GPU_ARCH_TURING  = 6,   // RTX 20xx — first RTX
    GPU_ARCH_AMPERE  = 7,   // RTX 30xx
    GPU_ARCH_ADA     = 8,   // RTX 40xx
} gpu_arch_t;

typedef struct {
    gpu_vendor_t vendor;
    gpu_arch_t   arch;
    uint16_t     vendor_id;
    uint16_t     device_id;
    uint8_t      pci_bus;
    uint8_t      pci_dev;
    uint8_t      pci_func;
    uint64_t     mmio_base;     // BAR0 — control registers
    uint64_t     vram_base;     // BAR1 — linear framebuffer / VRAM
    uint64_t     mmio_ext;      // BAR5 — extended MMIO (Turing+)
    uint32_t     fb_width;
    uint32_t     fb_height;
    uint32_t     fb_pitch;
    uint64_t     fb_addr;
    bool         has_rtx;
    bool         has_tensor_cores;
} gpu_info_t;

bool         gpu_init(void);
bool         gpu_is_available(void);
gpu_info_t*  gpu_get_info(void);
const char*  gpu_arch_name(void);

void gpu_blit(uint32_t dst_x, uint32_t dst_y,
              uint32_t src_x, uint32_t src_y,
              uint32_t width, uint32_t height);
void gpu_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

// RTX pipeline (v2.0 stubs)
void gpu_rtx_init_pipeline(void);
void gpu_rtx_trace_frame(void);
