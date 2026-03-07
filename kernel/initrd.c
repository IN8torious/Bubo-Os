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

// =============================================================================
// Raven AOS — initrd (Initial RAM Disk)
// Simple read-only RAM filesystem loaded by GRUB as a multiboot module
// Format: custom Raven Archive (RVAR) — header + file entries + data
// =============================================================================
#include "initrd.h"
#include "vfs.h"
#include "vmm.h"
#include "vga.h"

// ── RVAR Archive format ───────────────────────────────────────────────────────
// [RVAR_HEADER] [RVAR_ENTRY x nfiles] [file data...]
#define RVAR_MAGIC   0x52564152   // "RVAR"
#define RVAR_MAX_FILES 128

typedef struct {
    uint32_t magic;
    uint32_t nfiles;
    uint32_t data_offset;   // offset from start of archive to file data
    uint32_t reserved;
} __attribute__((packed)) rvar_header_t;

typedef struct {
    char     name[64];
    uint32_t offset;        // offset from data_offset
    uint32_t size;
    uint32_t type;          // VFS_TYPE_FILE or VFS_TYPE_DIR
    uint32_t reserved;
} __attribute__((packed)) rvar_entry_t;

// ── initrd state ─────────────────────────────────────────────────────────────
static uint8_t*      g_initrd_data  = 0;
static uint64_t      g_initrd_size  = 0;
static vfs_node_t    g_initrd_nodes[RVAR_MAX_FILES];
static vfs_ops_t     g_initrd_ops;
static uint32_t      g_nfiles       = 0;
static vfs_node_t    g_initrd_root;

// ── String helpers ────────────────────────────────────────────────────────────
static int rd_strlen(const char* s) { int n=0; while(s[n]) n++; return n; }
static int rd_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void rd_strcpy(char* d, const char* s, int m) {
    int i=0; while(s[i]&&i<m-1){d[i]=s[i];i++;} d[i]=0;
}

// ── VFS ops for initrd files ──────────────────────────────────────────────────
static uint32_t initrd_read(vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buf) {
    if (!node || !node->impl) return 0;
    uint8_t* data = (uint8_t*)node->impl;
    uint64_t fsize = node->size;
    if (offset >= fsize) return 0;
    uint32_t avail = (uint32_t)(fsize - offset);
    if (size > avail) size = avail;
    for (uint32_t i = 0; i < size; i++) buf[i] = data[offset + i];
    return size;
}

static uint32_t initrd_write(vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buf) {
    (void)node; (void)offset; (void)size; (void)buf;
    return 0; // initrd is read-only
}

static bool initrd_open(vfs_node_t* node, uint32_t flags) {
    (void)node; (void)flags; return true;
}

static void initrd_close(vfs_node_t* node) { (void)node; }

static vfs_node_t* initrd_finddir(vfs_node_t* node, const char* name) {
    for (uint32_t i = 0; i < node->child_count; i++) {
        if (rd_strcmp(node->children[i]->name, name) == 0)
            return node->children[i];
    }
    return 0;
}

// ── Initialize initrd from multiboot module ───────────────────────────────────
vfs_node_t* initrd_init(uint64_t addr, uint64_t size) {
    g_initrd_data = (uint8_t*)addr;
    g_initrd_size = size;

    terminal_write("[INITRD] Loading RAM disk at 0x");
    // Print address in hex
    char hex[17];
    uint64_t v = addr;
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = v & 0xF;
        hex[i] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
        v >>= 4;
    }
    hex[16] = 0;
    terminal_write(hex);
    terminal_write("\n");

    // Set up VFS ops
    g_initrd_ops.read    = initrd_read;
    g_initrd_ops.write   = initrd_write;
    g_initrd_ops.open    = initrd_open;
    g_initrd_ops.close   = initrd_close;
    g_initrd_ops.finddir = initrd_finddir;
    g_initrd_ops.mkdir   = 0;

    // Set up root node
    rd_strcpy(g_initrd_root.name, "initrd", VFS_MAX_NAME);
    g_initrd_root.type        = VFS_TYPE_DIR;
    g_initrd_root.size        = 0;
    g_initrd_root.inode       = 0;
    g_initrd_root.flags       = 0;
    g_initrd_root.impl        = 0;
    g_initrd_root.ops         = &g_initrd_ops;
    g_initrd_root.parent      = 0;
    g_initrd_root.child_count = 0;

    // Check for RVAR magic
    if (size < sizeof(rvar_header_t)) {
        terminal_write("[INITRD] No RVAR archive — empty initrd\n");
        return &g_initrd_root;
    }

    rvar_header_t* hdr = (rvar_header_t*)g_initrd_data;
    if (hdr->magic != RVAR_MAGIC) {
        terminal_write("[INITRD] No RVAR magic — treating as raw data\n");
        // Create a single "data" file with all contents
        g_initrd_nodes[0].name[0] = 'd'; g_initrd_nodes[0].name[1] = 'a';
        g_initrd_nodes[0].name[2] = 't'; g_initrd_nodes[0].name[3] = 'a';
        g_initrd_nodes[0].name[4] = 0;
        g_initrd_nodes[0].type  = VFS_TYPE_FILE;
        g_initrd_nodes[0].size  = size;
        g_initrd_nodes[0].inode = 1;
        g_initrd_nodes[0].impl  = g_initrd_data;
        g_initrd_nodes[0].ops   = &g_initrd_ops;
        g_initrd_nodes[0].parent = &g_initrd_root;
        g_initrd_nodes[0].child_count = 0;
        g_initrd_root.children[0] = &g_initrd_nodes[0];
        g_initrd_root.child_count = 1;
        g_nfiles = 1;
        return &g_initrd_root;
    }

    // Parse RVAR entries
    rvar_entry_t* entries = (rvar_entry_t*)(g_initrd_data + sizeof(rvar_header_t));
    uint32_t nfiles = hdr->nfiles;
    if (nfiles > RVAR_MAX_FILES) nfiles = RVAR_MAX_FILES;

    uint8_t* data_base = g_initrd_data + hdr->data_offset;

    for (uint32_t i = 0; i < nfiles; i++) {
        rd_strcpy(g_initrd_nodes[i].name, entries[i].name, VFS_MAX_NAME);
        g_initrd_nodes[i].type        = entries[i].type;
        g_initrd_nodes[i].size        = entries[i].size;
        g_initrd_nodes[i].inode       = i + 1;
        g_initrd_nodes[i].flags       = 0;
        g_initrd_nodes[i].impl        = data_base + entries[i].offset;
        g_initrd_nodes[i].ops         = &g_initrd_ops;
        g_initrd_nodes[i].parent      = &g_initrd_root;
        g_initrd_nodes[i].child_count = 0;

        if (g_initrd_root.child_count < VFS_MAX_CHILDREN) {
            g_initrd_root.children[g_initrd_root.child_count++] = &g_initrd_nodes[i];
        }
    }
    g_nfiles = nfiles;

    terminal_write("[INITRD] Loaded ");
    char nbuf[8];
    uint32_t n = nfiles;
    nbuf[0] = '0' + (n / 10) % 10;
    nbuf[1] = '0' + n % 10;
    nbuf[2] = ' '; nbuf[3] = 'f'; nbuf[4] = 'i'; nbuf[5] = 'l'; nbuf[6] = 'e'; nbuf[7] = 0;
    terminal_write(nbuf);
    terminal_write("s\n");

    return &g_initrd_root;
}

// ── List initrd contents ──────────────────────────────────────────────────────
void initrd_list(void) {
    terminal_write("[INITRD] Contents:\n");
    for (uint32_t i = 0; i < g_nfiles; i++) {
        terminal_write("  /initrd/");
        terminal_write(g_initrd_nodes[i].name);
        terminal_write("\n");
    }
}
