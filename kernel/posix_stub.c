// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious (Nathan Samuel) | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/posix_stub.c — POSIX Compatibility Stub Layer
// Provides POSIX-compatible syscall stubs so Linux binaries can run on
// Raven AOS. This is the foundation of the universal POSIX layer (v1.5+).
// =============================================================================

#include "posix_stub.h"
#include "vfs.h"
#include <stdint.h>
#include <stdbool.h>

// ── POSIX file descriptor table ───────────────────────────────────────────────
#define POSIX_MAX_FD  64

typedef struct {
    bool     open;
    uint32_t vfs_handle;
    uint32_t flags;
    uint32_t offset;
} posix_fd_t;

static posix_fd_t g_fd_table[POSIX_MAX_FD];
static bool g_posix_init = false;

void posix_stub_init(void) {
    for (uint32_t i = 0; i < POSIX_MAX_FD; i++) {
        g_fd_table[i].open = false;
        g_fd_table[i].vfs_handle = 0;
        g_fd_table[i].flags = 0;
        g_fd_table[i].offset = 0;
    }
    // stdin=0, stdout=1, stderr=2 reserved
    g_fd_table[0].open = true;
    g_fd_table[1].open = true;
    g_fd_table[2].open = true;
    g_posix_init = true;
}

// ── sys_open ──────────────────────────────────────────────────────────────────
int32_t posix_open(const char* path, uint32_t flags) {
    if (!g_posix_init) return -1;
    // Find free fd slot
    for (uint32_t i = 3; i < POSIX_MAX_FD; i++) {
        if (!g_fd_table[i].open) {
            // Try to open via VFS
            vfs_node_t* node = vfs_resolve(path);
            // O_CREAT: create via vfs_open with create flag
            if (!node && (flags & VFS_O_CREAT)) { node = vfs_resolve("/"); }
            if (!node) return -1;
            g_fd_table[i].open       = true;
            g_fd_table[i].vfs_handle = (uint32_t)i;
            g_fd_table[i].flags      = flags;
            g_fd_table[i].offset     = 0;
            return (int32_t)i;
        }
    }
    return -1;  // EMFILE
}

// ── sys_close ─────────────────────────────────────────────────────────────────
int32_t posix_close(int32_t fd) {
    if (fd < 0 || fd >= POSIX_MAX_FD) return -1;
    if (!g_fd_table[fd].open) return -1;
    g_fd_table[fd].open = false;
    g_fd_table[fd].vfs_handle = 0;
    g_fd_table[fd].offset = 0;
    return 0;
}

// ── sys_read ──────────────────────────────────────────────────────────────────
int32_t posix_read(int32_t fd, void* buf, uint32_t count) {
    if (fd < 0 || fd >= POSIX_MAX_FD) return -1;
    if (!g_fd_table[fd].open) return -1;
    if (fd == 0) {
        // stdin — keyboard input (stub: return 0 bytes for now)
        return 0;
    }
    return vfs_read(fd, buf, count);
}

// ── sys_write ─────────────────────────────────────────────────────────────────
int32_t posix_write(int32_t fd, const void* buf, uint32_t count) {
    if (fd < 0 || fd >= POSIX_MAX_FD) return -1;
    if (fd == 1 || fd == 2) {
        // stdout/stderr — write to terminal
        const char* s = (const char*)buf;
        for (uint32_t i = 0; i < count; i++) {
            // terminal_putchar(s[i]);  // wired to terminal in kernel
            (void)s[i];
        }
        return (int32_t)count;
    }
    if (!g_fd_table[fd].open) return -1;
    return vfs_write(fd, buf, count);
}

// ── sys_lseek ─────────────────────────────────────────────────────────────────
int32_t posix_lseek(int32_t fd, int32_t offset, uint32_t whence) {
    if (fd < 3 || fd >= POSIX_MAX_FD) return -1;
    if (!g_fd_table[fd].open) return -1;
    switch (whence) {
        case 0: g_fd_table[fd].offset = (uint32_t)offset; break;         // SEEK_SET
        case 1: g_fd_table[fd].offset += (uint32_t)offset; break;        // SEEK_CUR
        case 2: g_fd_table[fd].offset = 0xFFFFFFFF; break;               // SEEK_END stub
    }
    return (int32_t)g_fd_table[fd].offset;
}

// ── sys_exit ──────────────────────────────────────────────────────────────────
void posix_exit(int32_t code) {
    (void)code;
    // In Raven AOS, user-mode process exit returns to kernel scheduler
    // For now, infinite loop (will be replaced with scheduler yield in v1.5)
    while (1) {}
}

// ── Syscall dispatcher ────────────────────────────────────────────────────────
// This is called from the Ring 3 → Ring 0 syscall gate (INT 0x80 or SYSCALL)
uint64_t posix_syscall(uint64_t nr, uint64_t a0, uint64_t a1, uint64_t a2) {
    switch (nr) {
        case 0:  return (uint64_t)posix_read((int32_t)a0, (void*)a1, (uint32_t)a2);
        case 1:  return (uint64_t)posix_write((int32_t)a0, (const void*)a1, (uint32_t)a2);
        case 2:  return (uint64_t)posix_open((const char*)a0, (uint32_t)a1);
        case 3:  return (uint64_t)posix_close((int32_t)a0);
        case 8:  return (uint64_t)posix_lseek((int32_t)a0, (int32_t)a1, (uint32_t)a2);
        case 60: posix_exit((int32_t)a0); return 0;
        default: return (uint64_t)-1;  // ENOSYS
    }
}
