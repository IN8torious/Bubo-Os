// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/posix_stub.h — Header for posix_stub.c — syscall numbers, error codes, stat struct
// =============================================================================

#pragma once

#include <stdint.h>

// Syscall numbers (arbitrary, for Instinct OS internal use)
#define SYS_OPEN        0
#define SYS_CLOSE       1
#define SYS_READ        2
#define SYS_WRITE       3
#define SYS_LSEEK       4
#define SYS_STAT        5
#define SYS_FSTAT       6
#define SYS_MKDIR       7
#define SYS_UNLINK      8
#define SYS_GETPID      9
#define SYS_GETUID      10
#define SYS_EXIT        11
#define SYS_BRK         12
#define SYS_MMAP        13
#define SYS_MUNMAP      14
#define SYS_FORK        15
#define SYS_EXECVE      16
#define SYS_SOCKET      17

// Common POSIX error codes
#define EPERM           1       // Operation not permitted
#define ENOENT          2       // No such file or directory
#define ESRCH           3       // No such process
#define EINTR           4       // Interrupted system call
#define EIO             5       // I/O error
#define ENXIO           6       // No such device or address
#define E2BIG           7       // Argument list too long
#define ENOEXEC         8       // Exec format error
#define EBADF           9       // Bad file number
#define ECHILD          10      // No child processes
#define EAGAIN          11      // Try again
#define ENOMEM          12      // Out of memory
#define EACCES          13      // Permission denied
#define EFAULT          14      // Bad address
#define ENOTBLK         15      // Block device required
#define EBUSY           16      // Device or resource busy
#define EEXIST          17      // File exists
#define EXDEV           18      // Cross-device link
#define ENODEV          19      // No such device
#define ENOTDIR         20      // Not a directory
#define EISDIR          21      // Is a directory
#define EINVAL          22      // Invalid argument
#define ENFILE          23      // File table overflow
#define EMFILE          24      // Too many open files
#define ENOTTY          25      // Not a typewriter
#define ETXTBSY         26      // Text file busy
#define EFBIG           27      // File too large
#define ENOSPC          28      // No space left on device
#define ESPIPE          29      // Illegal seek
#define EROFS           30      // Read-only file system
#define EMLINK          31      // Too many links
#define EPIPE           32      // Broken pipe
#define EDOM            33      // Math argument out of domain of func
#define ERANGE          34      // Math result not representable
#define ENOSYS          38      // Function not implemented

// Stat structure (simplified for bare-metal)
// This structure is a basic representation and may not include all fields
// found in a full POSIX stat struct.
typedef struct {
    uint32_t st_dev;        // ID of device containing file
    uint32_t st_ino;        // Inode number
    uint16_t st_mode;       // File type and mode
    uint16_t st_nlink;      // Number of hard links
    uint16_t st_uid;        // User ID of owner
    uint16_t st_gid;        // Group ID of owner
    uint32_t st_rdev;       // Device ID (if special file)
    uint32_t st_size;       // Total size, in bytes
    uint32_t st_blksize;    // Block size for filesystem I/O
    uint32_t st_blocks;     // Number of 512B blocks allocated
    uint32_t st_atime;      // Time of last access
    uint32_t st_mtime;      // Time of last modification
    uint32_t st_ctime;      // Time of last status change
} stat_t;

// File types for st_mode
#define S_IFMT          0170000 // Mask for file type
#define S_IFSOCK        0140000 // Socket
#define S_IFLNK         0120000 // Symbolic link
#define S_IFREG         0100000 // Regular file
#define S_IFBLK         0060000 // Block device
#define S_IFDIR         0040000 // Directory
#define S_IFCHR         0020000 // Character device
#define S_IFIFO         0010000 // FIFO

// File permissions for st_mode
#define S_IRWXU         00700   // RWE for owner
#define S_IRUSR         00400   // Read by owner
#define S_IWUSR         00200   // Write by owner
#define S_IXUSR         00100   // Execute by owner

#define S_IRWXG         00070   // RWE for group
#define S_IRGRP         00040   // Read by group
#define S_IWGRP         00020   // Write by group
#define S_IXGRP         00010   // Execute by group

#define S_IRWXO         00007   // RWE for others
#define S_IROTH         00004   // Read by others
#define S_IWOTH         00002   // Write by others
#define S_IXOTH         00001   // Execute by others
