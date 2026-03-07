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

#pragma once
// =============================================================================
// Instinct OS — Virtual File System (VFS)
// Abstraction layer: same API for initrd, FAT32, devfs, procfs
// =============================================================================
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define VFS_MAX_PATH      256
#define VFS_MAX_NAME       64
#define VFS_MAX_MOUNTS     16
#define VFS_MAX_FDS        64
#define VFS_MAX_CHILDREN   32

// ── File types ────────────────────────────────────────────────────────────────
#define VFS_TYPE_FILE      0x01
#define VFS_TYPE_DIR       0x02
#define VFS_TYPE_CHARDEV   0x03
#define VFS_TYPE_BLKDEV    0x04
#define VFS_TYPE_PIPE      0x05
#define VFS_TYPE_SYMLINK   0x06

// ── Open flags ────────────────────────────────────────────────────────────────
#define VFS_O_RDONLY       0x00
#define VFS_O_WRONLY       0x01
#define VFS_O_RDWR         0x02
#define VFS_O_CREAT        0x40
#define VFS_O_TRUNC        0x200
#define VFS_O_APPEND       0x400

// ── VFS node (inode equivalent) ───────────────────────────────────────────────
typedef struct vfs_node vfs_node_t;

typedef struct {
    uint32_t (*read) (vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buf);
    uint32_t (*write)(vfs_node_t* node, uint64_t offset, uint32_t size, uint8_t* buf);
    bool     (*open) (vfs_node_t* node, uint32_t flags);
    void     (*close)(vfs_node_t* node);
    vfs_node_t* (*finddir)(vfs_node_t* node, const char* name);
    bool     (*mkdir)(vfs_node_t* node, const char* name);
} vfs_ops_t;

struct vfs_node {
    char        name[VFS_MAX_NAME];
    uint32_t    type;
    uint64_t    size;
    uint64_t    inode;
    uint32_t    flags;
    void*       impl;           // filesystem-specific data
    vfs_ops_t*  ops;
    vfs_node_t* parent;
    vfs_node_t* children[VFS_MAX_CHILDREN];
    uint32_t    child_count;
};

// ── File descriptor ───────────────────────────────────────────────────────────
typedef struct {
    vfs_node_t* node;
    uint64_t    offset;
    uint32_t    flags;
    bool        used;
} vfs_fd_t;

// ── Mount point ───────────────────────────────────────────────────────────────
typedef struct {
    char        path[VFS_MAX_PATH];
    vfs_node_t* root;
    bool        used;
} vfs_mount_t;

// ── Public API ────────────────────────────────────────────────────────────────
void        vfs_init(void);
bool        vfs_mount(const char* path, vfs_node_t* root);
vfs_node_t* vfs_resolve(const char* path);
int32_t     vfs_open(const char* path, uint32_t flags);
void        vfs_close(int32_t fd);
int32_t     vfs_read(int32_t fd, void* buf, uint32_t size);
int32_t     vfs_write(int32_t fd, const void* buf, uint32_t size);
int32_t     vfs_seek(int32_t fd, int64_t offset, int whence);
bool        vfs_mkdir(const char* path);
bool        vfs_exists(const char* path);
uint64_t    vfs_size(const char* path);
vfs_node_t* vfs_get_root(void);
