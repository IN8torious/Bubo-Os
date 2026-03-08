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

#pragma once
// =============================================================================
// Deep Flow OS — User Mode (Ring 3)
// TSS, syscall interface, user process bootstrap
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

// ── Segment selectors ─────────────────────────────────────────────────────────
#define KERNEL_CODE_SEG   0x08
#define KERNEL_DATA_SEG   0x10
#define USER_CODE_SEG     0x18   // Ring 3 code
#define USER_DATA_SEG     0x20   // Ring 3 data
#define TSS_SEG           0x28   // TSS descriptor

// ── Syscall numbers ───────────────────────────────────────────────────────────
#define SYS_EXIT          0
#define SYS_WRITE         1
#define SYS_READ          2
#define SYS_OPEN          3
#define SYS_CLOSE         4
#define SYS_MALLOC        5
#define SYS_FREE          6
#define SYS_GETPID        7
#define SYS_YIELD         8
#define SYS_SLEEP         9
#define SYS_CORVUS_CALL   10    // Direct CORVUS agent invocation
#define SYS_SPEAK         11    // CORVUS speaks to user (accessibility)
#define SYS_LISTEN        12    // CORVUS listens for voice input
#define SYS_DRIVE_CMD     13    // Racing game drive command
#define SYS_MAX           14

// ── Task State Segment (64-bit) ───────────────────────────────────────────────
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;          // Kernel stack pointer (Ring 0)
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;          // Interrupt stack table entries
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss64_t;

// ── Syscall result ────────────────────────────────────────────────────────────
typedef struct {
    int64_t  value;
    bool     error;
    uint32_t errno;
} syscall_result_t;

// ── Public API ────────────────────────────────────────────────────────────────
void usermode_init(void);
void tss_init(uint64_t kernel_stack);
void tss_set_kernel_stack(uint64_t rsp0);
void jump_to_usermode(uint64_t entry, uint64_t user_stack);

// Syscall handler (called from ISR stub)
syscall_result_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2,
                                  uint64_t a3, uint64_t a4, uint64_t a5);
