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

#ifndef RAVEN_PMM_H
#define RAVEN_PMM_H

// =============================================================================
// Raven OS — Physical Memory Manager (PMM)
// Bitmap allocator: 1 bit per 4KiB page frame
// =============================================================================

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE       4096        // 4 KiB
#define PMM_BITMAP_ADDR 0x100000    // Bitmap lives at 1MB (before kernel at 1MB+)

// Multiboot2 memory map entry types
#define MULTIBOOT2_MEMORY_AVAILABLE  1
#define MULTIBOOT2_MEMORY_RESERVED   2
#define MULTIBOOT2_MEMORY_ACPI       3
#define MULTIBOOT2_MEMORY_NVS        4
#define MULTIBOOT2_MEMORY_BADRAM     5

// Multiboot2 structures (simplified)
typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) mb2_tag_t;

typedef struct {
    uint32_t type;      // = 6 for memory map
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed)) mb2_tag_mmap_t;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed)) mb2_mmap_entry_t;

// PMM statistics
typedef struct {
    uint32_t total_pages;
    uint32_t free_pages;
    uint32_t used_pages;
    uint32_t reserved_pages;
} pmm_stats_t;

// Initialize PMM from multiboot2 memory map
void pmm_init(uint32_t multiboot_info_addr);

// Allocate one 4KiB physical page — returns physical address or NULL
void* pmm_alloc_page(void);

// Free a previously allocated page
void pmm_free_page(void* addr);

// Check if a page is free
bool pmm_is_page_free(uint32_t page_index);

// Get PMM statistics
pmm_stats_t pmm_get_stats(void);

// Mark a region as used/free
void pmm_mark_region_used(uint32_t base, uint32_t size);
void pmm_mark_region_free(uint32_t base, uint32_t size);

#endif // RAVEN_PMM_H
