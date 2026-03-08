/*
 * BUBO OS — Display Abstraction Layer
 * include/bubo_display.h
 *
 * Thin wrapper that pulls together framebuffer init and the GPU
 * context so kernel.c has a single include for all display setup.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#ifndef BUBO_DISPLAY_H
#define BUBO_DISPLAY_H

#include "framebuffer.h"
#include "gpu.h"

/* ── Display initialisation ─────────────────────────────────────────────── */

/*
 * bubo_display_init()
 *
 * Called from kernel_main after multiboot info is available.
 * Attempts VESA framebuffer init; falls back to VGA text mode on failure.
 * Returns true if a pixel framebuffer is available, false for text-only.
 */
static inline int bubo_display_init(uint32_t mb2_info_addr)
{
    if (fb_init_from_multiboot(mb2_info_addr)) return 1;
    fb_init_fallback();
    return 0;
}

#endif /* BUBO_DISPLAY_H */
