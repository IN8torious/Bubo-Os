// =============================================================================
// Instinct OS — Dedicated to Landon Pankuch
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

// =============================================================================
// Raven OS — Physical Memory Manager (PMM)
// Bitmap allocator: 1 bit per 4KiB page frame
// Layer 2 of the 7-Layer Agentic OS Architecture
// Telemetry hooks feed Layer 4 (Sensory) ring buffer
// =============================================================================

#include "pmm.h"
#include "vga.h"
#include <stdint.h>
#include <stddef.h>

// ── Bitmap storage ────────────────────────────────────────────────────────────
// Each bit = 1 page (4KiB). 1 = used, 0 = free.
// 1GB of RAM = 262144 pages = 32768 bytes of bitmap
#define PMM_MAX_PAGES   (1024 * 1024)       // Support up to 4GB (4G/4K pages)
#define BITMAP_SIZE     (PMM_MAX_PAGES / 8) // In bytes

static uint8_t  pmm_bitmap[BITMAP_SIZE];
static uint32_t pmm_total_pages  = 0;
static uint32_t pmm_free_pages   = 0;
static uint32_t pmm_last_alloc   = 0;   // Next-fit hint for speed

// ── Telemetry ring buffer (Layer 4 feed) ─────────────────────────────────────
#define TELEM_RING_SIZE 256

typedef struct {
    uint8_t  event;     // 0=alloc, 1=free, 2=oom, 3=init
    uint32_t addr;
    uint32_t timestamp; // tick counter (set by PIT later)
} pmm_telem_t;

static pmm_telem_t telem_ring[TELEM_RING_SIZE];
static uint32_t    telem_head = 0;
static uint32_t    telem_count = 0;

static void telem_push(uint8_t event, uint32_t addr) {
    telem_ring[telem_head % TELEM_RING_SIZE].event = event;
    telem_ring[telem_head % TELEM_RING_SIZE].addr  = addr;
    telem_ring[telem_head % TELEM_RING_SIZE].timestamp = telem_count++;
    telem_head = (telem_head + 1) % TELEM_RING_SIZE;
}

// ── Bitmap helpers ────────────────────────────────────────────────────────────
static inline void bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (uint8_t)(1 << (bit % 8));
}

static inline void bitmap_clear(uint32_t bit) {
    pmm_bitmap[bit / 8] &= (uint8_t)~(1 << (bit % 8));
}

static inline bool bitmap_test(uint32_t bit) {
    return (pmm_bitmap[bit / 8] >> (bit % 8)) & 1;
}

// ── Mark regions ─────────────────────────────────────────────────────────────
void pmm_mark_region_used(uint32_t base, uint32_t size) {
    uint32_t start_page = base / PAGE_SIZE;
    uint32_t page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < page_count; i++) {
        if (start_page + i < PMM_MAX_PAGES) {
            if (!bitmap_test(start_page + i)) {
                bitmap_set(start_page + i);
                if (pmm_free_pages > 0) pmm_free_pages--;
            }
        }
    }
}

void pmm_mark_region_free(uint32_t base, uint32_t size) {
    uint32_t start_page = base / PAGE_SIZE;
    uint32_t page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < page_count; i++) {
        if (start_page + i < PMM_MAX_PAGES) {
            if (bitmap_test(start_page + i)) {
                bitmap_clear(start_page + i);
                pmm_free_pages++;
            }
        }
    }
}

// ── Init from multiboot2 memory map ──────────────────────────────────────────
void pmm_init(uint32_t multiboot_info_addr) {
    // Mark everything as used first
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFF;
    }
    pmm_free_pages = 0;

    // Parse multiboot2 tags
    uint32_t addr = multiboot_info_addr + 8; // Skip total_size and reserved
    uint32_t total_size = *((uint32_t*)multiboot_info_addr);
    uint32_t end = multiboot_info_addr + total_size;

    uint32_t detected_ram = 0;

    while (addr < end) {
        mb2_tag_t* tag = (mb2_tag_t*)addr;
        if (tag->type == 0) break; // End tag

        if (tag->type == 6) { // Memory map tag
            mb2_tag_mmap_t* mmap_tag = (mb2_tag_mmap_t*)tag;
            uint32_t entry_addr = addr + sizeof(mb2_tag_mmap_t);
            uint32_t mmap_end   = addr + tag->size;

            while (entry_addr < mmap_end) {
                mb2_mmap_entry_t* entry = (mb2_mmap_entry_t*)entry_addr;

                // Only use low 32-bit addresses (we're 32-bit kernel)
                if (entry->base_addr < 0xFFFFFFFF) {
                    uint32_t base = (uint32_t)entry->base_addr;
                    uint32_t len  = (entry->length > 0xFFFFFFFF) ?
                                    0xFFFFFFFF - base : (uint32_t)entry->length;

                    if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                        pmm_mark_region_free(base, len);
                        detected_ram += len;
                    }
                }
                entry_addr += mmap_tag->entry_size;
            }
        }

        // Advance to next tag (8-byte aligned)
        addr += (tag->size + 7) & ~7;
    }

    pmm_total_pages = detected_ram / PAGE_SIZE;

    // Re-mark critical regions as used:
    // 0x0000 - 0x1000: Real mode IVT + BIOS data
    pmm_mark_region_used(0x0, 0x1000);
    // 0x7C00 - 0x8000: Boot sector
    pmm_mark_region_used(0x7C00, 0x400);
    // 0x100000 - kernel end (approx 2MB for safety)
    pmm_mark_region_used(0x100000, 0x200000);
    // Multiboot2 info struct
    pmm_mark_region_used(multiboot_info_addr, 0x1000);

    telem_push(3, 0); // init event

    // Report
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE, VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY, VGA_BLACK);

    terminal_setcolor(white);
    terminal_write("  [ PMM  ] Physical Memory Manager initialized\n");
    terminal_setcolor(grey);
    terminal_write("           Total pages: ");
    // Print number inline
    char buf[12];
    uint32_t n = pmm_total_pages;
    if (n == 0) { buf[0]='0'; buf[1]='\0'; }
    else {
        char tmp[12]; int i=0;
        while(n>0){tmp[i++]='0'+(n%10);n/=10;}
        int j=0; while(i>0) buf[j++]=tmp[--i]; buf[j]='\0';
    }
    terminal_write(buf);
    terminal_setcolor(green);
    terminal_write("  |  Free: ");
    n = pmm_free_pages;
    if (n == 0) { buf[0]='0'; buf[1]='\0'; }
    else {
        char tmp[12]; int i=0;
        while(n>0){tmp[i++]='0'+(n%10);n/=10;}
        int j=0; while(i>0) buf[j++]=tmp[--i]; buf[j]='\0';
    }
    terminal_write(buf);
    terminal_write(" pages free\n");
}

// ── Allocate one page ─────────────────────────────────────────────────────────
void* pmm_alloc_page(void) {
    if (pmm_free_pages == 0) {
        telem_push(2, 0); // OOM event
        return NULL;
    }

    // Next-fit search starting from last allocation
    for (uint32_t i = 0; i < pmm_total_pages; i++) {
        uint32_t idx = (pmm_last_alloc + i) % pmm_total_pages;
        if (!bitmap_test(idx)) {
            bitmap_set(idx);
            pmm_free_pages--;
            pmm_last_alloc = idx + 1;
            uint32_t phys_addr = idx * PAGE_SIZE;
            telem_push(0, phys_addr); // alloc event
            return (void*)phys_addr;
        }
    }

    telem_push(2, 0); // OOM
    return NULL;
}

// ── Free one page ─────────────────────────────────────────────────────────────
void pmm_free_page(void* addr) {
    uint32_t page_idx = (uint32_t)addr / PAGE_SIZE;
    if (page_idx >= PMM_MAX_PAGES) return;
    if (bitmap_test(page_idx)) {
        bitmap_clear(page_idx);
        pmm_free_pages++;
        telem_push(1, (uint32_t)addr); // free event
    }
}

// ── Query ─────────────────────────────────────────────────────────────────────
bool pmm_is_page_free(uint32_t page_index) {
    return !bitmap_test(page_index);
}

pmm_stats_t pmm_get_stats(void) {
    pmm_stats_t s;
    s.total_pages    = pmm_total_pages;
    s.free_pages     = pmm_free_pages;
    s.used_pages     = pmm_total_pages - pmm_free_pages;
    s.reserved_pages = PMM_MAX_PAGES - pmm_total_pages;
    return s;
}
