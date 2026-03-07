// =============================================================================
// Raven OS — Virtual Memory Manager (VMM)
// Implements x86 32-bit paging: page directory + page tables
// Maps kernel to identity (0x0 → 0x0) for now, with heap at 0xD0000000
// Layer 3 of the 7-Layer AOS Architecture
// =============================================================================

#include "vmm.h"
#include "pmm.h"
#include "vga.h"
#include <stdint.h>
#include <stddef.h>

// ── Page directory (4KB aligned, 1024 entries) ────────────────────────────────
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// ── Heap state ────────────────────────────────────────────────────────────────
#define HEAP_START      0xD0000000
#define HEAP_MAX_SIZE   (16 * 1024 * 1024)  // 16MB heap

typedef struct heap_block {
    uint32_t           magic;   // 0xC0FFEE for valid block
    uint32_t           size;    // Size of data (not including header)
    bool               free;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

#define HEAP_MAGIC 0xC0FFEE

static heap_block_t* heap_head = NULL;
static uint32_t      heap_current = HEAP_START;
static bool          vmm_initialized = false;

// ── Map a virtual address to a physical address ───────────────────────────────
void vmm_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    // Get or create page table
    uint32_t* page_table;

    if (!(page_directory[pd_idx] & VMM_PRESENT)) {
        // Allocate a new page table
        page_table = (uint32_t*)pmm_alloc_page();
        if (!page_table) return;

        // Zero it out
        for (int i = 0; i < 1024; i++) page_table[i] = 0;

        page_directory[pd_idx] = (uint32_t)page_table | VMM_PRESENT | VMM_WRITABLE;
    } else {
        page_table = (uint32_t*)(page_directory[pd_idx] & ~0xFFF);
    }

    page_table[pt_idx] = (phys & ~0xFFF) | (flags & 0xFFF) | VMM_PRESENT;

    // Flush TLB for this address
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// ── Unmap a virtual address ───────────────────────────────────────────────────
void vmm_unmap(uint32_t virt) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(page_directory[pd_idx] & VMM_PRESENT)) return;

    uint32_t* page_table = (uint32_t*)(page_directory[pd_idx] & ~0xFFF);
    page_table[pt_idx] = 0;

    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// ── Expand heap by allocating and mapping new pages ───────────────────────────
static bool heap_expand(uint32_t size) {
    uint32_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < pages_needed; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) return false;

        vmm_map(heap_current, (uint32_t)phys, VMM_PRESENT | VMM_WRITABLE);
        heap_current += PAGE_SIZE;

        if (heap_current >= HEAP_START + HEAP_MAX_SIZE) return false;
    }
    return true;
}

// ── kmalloc ───────────────────────────────────────────────────────────────────
void* kmalloc(uint32_t size) {
    if (!vmm_initialized || size == 0) return NULL;

    // Align to 8 bytes
    size = (size + 7) & ~7;

    // Search for a free block (first-fit)
    heap_block_t* block = heap_head;
    while (block) {
        if (block->free && block->size >= size) {
            // Split block if it's significantly larger
            if (block->size >= size + sizeof(heap_block_t) + 16) {
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)block +
                                          sizeof(heap_block_t) + size);
                new_block->magic = HEAP_MAGIC;
                new_block->size  = block->size - size - sizeof(heap_block_t);
                new_block->free  = true;
                new_block->next  = block->next;
                new_block->prev  = block;

                if (block->next) block->next->prev = new_block;
                block->next = new_block;
                block->size = size;
            }
            block->free = false;
            return (void*)((uint8_t*)block + sizeof(heap_block_t));
        }
        block = block->next;
    }

    // No free block found — expand heap
    uint32_t alloc_size = size + sizeof(heap_block_t);
    if (!heap_expand(alloc_size)) return NULL;

    // Create new block at end of heap
    heap_block_t* new_block = (heap_block_t*)(heap_current - alloc_size -
                               ((alloc_size % PAGE_SIZE) ?
                                PAGE_SIZE - (alloc_size % PAGE_SIZE) : 0));

    // Walk to end of list
    heap_block_t* last = heap_head;
    if (last) {
        while (last->next) last = last->next;
    }

    new_block->magic = HEAP_MAGIC;
    new_block->size  = size;
    new_block->free  = false;
    new_block->next  = NULL;
    new_block->prev  = last;

    if (last) last->next = new_block;
    else heap_head = new_block;

    return (void*)((uint8_t*)new_block + sizeof(heap_block_t));
}

// ── kfree ─────────────────────────────────────────────────────────────────────
void kfree(void* ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) return; // Corrupt block

    block->free = true;

    // Coalesce with next block
    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }

    // Coalesce with previous block
    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}

// ── Initialize VMM and enable paging ─────────────────────────────────────────
void vmm_init(void) {
    // Zero page directory
    for (int i = 0; i < 1024; i++) page_directory[i] = 0;

    // Identity map first 4MB (kernel lives here)
    // Using a pre-allocated page table for speed
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (uint64_t)(i * PAGE_SIZE) | VMM_PRESENT | VMM_WRITABLE;
    }
    page_directory[0] = (uint64_t)first_page_table | VMM_PRESENT | VMM_WRITABLE;

    // Load page directory into CR3
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)page_directory));

    // Enable paging in CR0
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000001ULL;  // PE + PG
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    // Initialize heap — map first 4 pages at HEAP_START
    for (int i = 0; i < 4; i++) {
        void* phys = pmm_alloc_page();
        if (phys) {
            vmm_map(HEAP_START + i * PAGE_SIZE, (uint32_t)phys,
                    VMM_PRESENT | VMM_WRITABLE);
        }
    }

    // Set up heap header
    heap_head = (heap_block_t*)HEAP_START;
    heap_head->magic = HEAP_MAGIC;
    heap_head->size  = 4 * PAGE_SIZE - sizeof(heap_block_t);
    heap_head->free  = true;
    heap_head->next  = NULL;
    heap_head->prev  = NULL;
    heap_current = HEAP_START + 4 * PAGE_SIZE;

    vmm_initialized = true;

    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
    terminal_setcolor(white);
    terminal_write("  [ VMM  ] Paging enabled — kernel identity mapped (0-4MB)");
    terminal_setcolor(green);
    terminal_writeline("  ✓");
    terminal_setcolor(white);
    terminal_write("  [ HEAP ] kmalloc heap initialized at 0xD0000000 — ");
    terminal_setcolor(green);
    terminal_writeline("16MB available");
}
