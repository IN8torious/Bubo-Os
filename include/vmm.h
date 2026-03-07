// =============================================================================
// Raven AOS — Dedicated to Landon Pankuch
// =============================================================================
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

#ifndef RAVEN_VMM_H
#define RAVEN_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pmm.h"

// Page flags
#define VMM_PRESENT     0x01
#define VMM_WRITABLE    0x02
#define VMM_USER        0x04
#define VMM_WRITE_THRU  0x08
#define VMM_NO_CACHE    0x10
#define VMM_ACCESSED    0x20
#define VMM_DIRTY       0x40
#define VMM_HUGE        0x80

// Initialize VMM and enable paging
void vmm_init(void);

// Map virtual → physical
void vmm_map(uint32_t virt, uint32_t phys, uint32_t flags);

// Unmap a virtual address
void vmm_unmap(uint32_t virt);

// Kernel heap allocator
void* kmalloc(uint32_t size);
void  kfree(void* ptr);

// Convenience: zero-initialized kmalloc
static inline void* kzalloc(uint32_t size) {
    void* p = kmalloc(size);
    if (p) {
        uint8_t* b = (uint8_t*)p;
        for (uint32_t i = 0; i < size; i++) b[i] = 0;
    }
    return p;
}

#endif // RAVEN_VMM_H
