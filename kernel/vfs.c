// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
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
// Deep Flow OS — Virtual File System (VFS)
// =============================================================================
#include "vfs.h"
#include "vmm.h"
#include "vga.h"

static vfs_node_t  g_root_node;
static vfs_mount_t g_mounts[VFS_MAX_MOUNTS];
static vfs_fd_t    g_fds[VFS_MAX_FDS];
static bool        g_vfs_ready = false;

// ── String helpers (no libc) ──────────────────────────────────────────────────
static int vfs_strlen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}
static int vfs_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void vfs_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

// ── Initialize VFS ────────────────────────────────────────────────────────────
void vfs_init(void) {
    // Clear all mounts and FDs
    for (int i = 0; i < VFS_MAX_MOUNTS; i++) g_mounts[i].used = false;
    for (int i = 0; i < VFS_MAX_FDS;    i++) g_fds[i].used    = false;

    // Set up root node
    vfs_strcpy(g_root_node.name, "/", VFS_MAX_NAME);
    g_root_node.type        = VFS_TYPE_DIR;
    g_root_node.size        = 0;
    g_root_node.inode       = 0;
    g_root_node.flags       = 0;
    g_root_node.impl        = 0;
    g_root_node.ops         = 0;
    g_root_node.parent      = 0;
    g_root_node.child_count = 0;

    g_vfs_ready = true;
    terminal_write("[VFS] Virtual filesystem initialized\n");
}

// ── Mount a filesystem at a path ──────────────────────────────────────────────
bool vfs_mount(const char* path, vfs_node_t* root) {
    for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (!g_mounts[i].used) {
            vfs_strcpy(g_mounts[i].path, path, VFS_MAX_PATH);
            g_mounts[i].root = root;
            g_mounts[i].used = true;
            terminal_write("[VFS] Mounted at ");
            terminal_write(path);
            terminal_write("\n");
            return true;
        }
    }
    terminal_write("[VFS] ERROR: No mount slots available\n");
    return false;
}

// ── Resolve a path to a VFS node ─────────────────────────────────────────────
vfs_node_t* vfs_resolve(const char* path) {
    if (!path || !path[0]) return 0;

    // Check mounts first (longest prefix match)
    vfs_mount_t* best_mount = 0;
    int best_len = 0;
    for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
        if (!g_mounts[i].used) continue;
        int mlen = vfs_strlen(g_mounts[i].path);
        if (mlen > best_len) {
            // Check if path starts with mount path
            bool match = true;
            for (int j = 0; j < mlen; j++) {
                if (path[j] != g_mounts[i].path[j]) { match = false; break; }
            }
            if (match) { best_mount = &g_mounts[i]; best_len = mlen; }
        }
    }

    vfs_node_t* current = best_mount ? best_mount->root : &g_root_node;
    const char* p = path + best_len;

    // Walk the path
    while (*p) {
        // Skip slashes
        while (*p == '/') p++;
        if (!*p) break;

        // Extract next component
        char component[VFS_MAX_NAME];
        int ci = 0;
        while (*p && *p != '/' && ci < VFS_MAX_NAME - 1) {
            component[ci++] = *p++;
        }
        component[ci] = 0;

        // Find child with this name
        if (current->ops && current->ops->finddir) {
            current = current->ops->finddir(current, component);
        } else {
            // Linear search through children
            bool found = false;
            for (uint32_t i = 0; i < current->child_count; i++) {
                if (vfs_strcmp(current->children[i]->name, component) == 0) {
                    current = current->children[i];
                    found = true;
                    break;
                }
            }
            if (!found) return 0;
        }
        if (!current) return 0;
    }
    return current;
}

// ── Open a file, return fd ────────────────────────────────────────────────────
int32_t vfs_open(const char* path, uint32_t flags) {
    vfs_node_t* node = vfs_resolve(path);
    if (!node) return -1;

    // Find free fd
    for (int i = 0; i < VFS_MAX_FDS; i++) {
        if (!g_fds[i].used) {
            g_fds[i].node   = node;
            g_fds[i].offset = 0;
            g_fds[i].flags  = flags;
            g_fds[i].used   = true;
            if (node->ops && node->ops->open)
                node->ops->open(node, flags);
            return i;
        }
    }
    return -1; // No free FDs
}

// ── Close a file descriptor ───────────────────────────────────────────────────
void vfs_close(int32_t fd) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !g_fds[fd].used) return;
    if (g_fds[fd].node->ops && g_fds[fd].node->ops->close)
        g_fds[fd].node->ops->close(g_fds[fd].node);
    g_fds[fd].used = false;
}

// ── Read from a file descriptor ───────────────────────────────────────────────
int32_t vfs_read(int32_t fd, void* buf, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !g_fds[fd].used) return -1;
    vfs_node_t* node = g_fds[fd].node;
    if (!node->ops || !node->ops->read) return -1;
    uint32_t n = node->ops->read(node, g_fds[fd].offset, size, (uint8_t*)buf);
    g_fds[fd].offset += n;
    return (int32_t)n;
}

// ── Write to a file descriptor ────────────────────────────────────────────────
int32_t vfs_write(int32_t fd, const void* buf, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !g_fds[fd].used) return -1;
    vfs_node_t* node = g_fds[fd].node;
    if (!node->ops || !node->ops->write) return -1;
    uint32_t n = node->ops->write(node, g_fds[fd].offset, size, (uint8_t*)buf);
    g_fds[fd].offset += n;
    return (int32_t)n;
}

// ── Seek within a file ────────────────────────────────────────────────────────
int32_t vfs_seek(int32_t fd, int64_t offset, int whence) {
    if (fd < 0 || fd >= VFS_MAX_FDS || !g_fds[fd].used) return -1;
    vfs_node_t* node = g_fds[fd].node;
    if (whence == 0) g_fds[fd].offset = (uint64_t)offset;
    else if (whence == 1) g_fds[fd].offset += (uint64_t)offset;
    else if (whence == 2) g_fds[fd].offset = node->size + (uint64_t)offset;
    return 0;
}

// ── Check if a path exists ────────────────────────────────────────────────────
bool vfs_exists(const char* path) {
    return vfs_resolve(path) != 0;
}

// ── Get file size ─────────────────────────────────────────────────────────────
uint64_t vfs_size(const char* path) {
    vfs_node_t* node = vfs_resolve(path);
    return node ? node->size : 0;
}

// ── Get root node ─────────────────────────────────────────────────────────────
vfs_node_t* vfs_get_root(void) {
    return &g_root_node;
}

// ── Create directory ──────────────────────────────────────────────────────────
// Walks the path to find the parent directory, then allocates a new VFS node
// for the final component and attaches it as a child of the parent.
bool vfs_mkdir(const char* path) {
    if (!path || !path[0]) return false;

    // ── Separate parent path from the new directory name ──────────────────────
    char parent_path[VFS_MAX_PATH];
    char dir_name[VFS_MAX_NAME];

    // Find the last '/' to split parent from name
    int plen = vfs_strlen(path);
    int slash = -1;
    for (int i = plen - 1; i >= 0; i--) {
        if (path[i] == '/') { slash = i; break; }
    }

    if (slash <= 0) {
        // e.g. mkdir("docs") — parent is root
        vfs_strcpy(parent_path, "/", VFS_MAX_PATH);
        vfs_strcpy(dir_name, path + (slash + 1), VFS_MAX_NAME);
    } else {
        // Copy parent portion
        int i = 0;
        for (; i < slash && i < VFS_MAX_PATH - 1; i++) parent_path[i] = path[i];
        parent_path[i] = 0;
        vfs_strcpy(dir_name, path + slash + 1, VFS_MAX_NAME);
    }

    if (dir_name[0] == 0) return false; // trailing slash, nothing to create

    // ── Resolve the parent node ───────────────────────────────────────────────
    vfs_node_t* parent = vfs_resolve(parent_path);
    if (!parent) {
        terminal_write("[VFS] mkdir: parent not found: ");
        terminal_write(parent_path);
        terminal_write("\n");
        return false;
    }
    if (parent->type != VFS_TYPE_DIR) {
        terminal_write("[VFS] mkdir: parent is not a directory\n");
        return false;
    }
    if (parent->child_count >= VFS_MAX_CHILDREN) {
        terminal_write("[VFS] mkdir: parent directory is full\n");
        return false;
    }

    // ── Check that the name does not already exist ────────────────────────────
    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (vfs_strcmp(parent->children[i]->name, dir_name) == 0) {
            terminal_write("[VFS] mkdir: already exists: ");
            terminal_write(dir_name);
            terminal_write("\n");
            return false;
        }
    }

    // ── Delegate to filesystem ops if available ───────────────────────────────
    if (parent->ops && parent->ops->mkdir) {
        return parent->ops->mkdir(parent, dir_name);
    }

    // ── Default: allocate a new in-memory directory node ─────────────────────
    vfs_node_t* node = (vfs_node_t*)kzalloc(sizeof(vfs_node_t));
    if (!node) {
        terminal_write("[VFS] mkdir: out of memory\n");
        return false;
    }

    vfs_strcpy(node->name, dir_name, VFS_MAX_NAME);
    node->type        = VFS_TYPE_DIR;
    node->size        = 0;
    node->inode       = (uint64_t)(uintptr_t)node; // use address as unique inode
    node->flags       = 0;
    node->impl        = 0;
    node->ops         = parent->ops;  // inherit parent ops (e.g. same filesystem)
    node->parent      = parent;
    node->child_count = 0;

    // Attach to parent
    parent->children[parent->child_count++] = node;

    terminal_write("[VFS] mkdir: created ");
    terminal_write(path);
    terminal_write("\n");
    return true;
}
