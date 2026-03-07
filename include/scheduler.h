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

#ifndef RAVEN_SCHEDULER_H
#define RAVEN_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

// Process states
typedef enum {
    PROC_DEAD    = 0,
    PROC_READY   = 1,
    PROC_RUNNING = 2,
    PROC_BLOCKED = 3,
    PROC_SLEEPING = 4,
} proc_state_t;

// Process types
typedef enum {
    PROC_KERNEL  = 0,   // Kernel thread
    PROC_USER    = 1,   // User-mode process
    PROC_AGENT   = 2,   // CORVUS agent process
    PROC_DRIVER  = 3,   // Driver process
} process_type_t;

// CPU context (saved during context switch)
typedef struct {
    uint64_t rsp;       // Stack pointer
    uint64_t rbp;       // Base pointer
    uint64_t rbx;       // Callee-saved
    uint64_t r12, r13, r14, r15;  // Callee-saved
    uint64_t rflags;    // CPU flags
    uint64_t rip;       // Instruction pointer (for new tasks)
} cpu_context_t;

// Process Control Block (PCB)
typedef struct {
    uint32_t       pid;
    char           name[32];
    proc_state_t   state;
    process_type_t type;
    cpu_context_t  context;

    uint64_t       stack_base;
    uint64_t       stack_top;

    uint8_t        priority;    // Base priority 0-255
    uint8_t        quantum;     // Time slices before preemption
    uint32_t       ticks;       // Ticks used in current quantum

    // CORVUS integration
    uint8_t        agent_id;    // 0xFF if not an agent
    char           goal[64];    // Current BDI goal (for agents)

    // Statistics
    uint32_t       total_ticks;
    uint32_t       page_faults;
} process_t;

// Initialize scheduler
void scheduler_init(void);

// Called every PIT tick
void scheduler_tick(void);

// Create a new process
process_t* scheduler_create(const char* name, void (*entry)(void),
                             uint8_t priority, process_type_t type);

// Kill a process by PID
void scheduler_kill(uint32_t pid);

// Get current running process
process_t* scheduler_current(void);

// ASM context switch (in scheduler_asm.asm)
void scheduler_switch(cpu_context_t* old_ctx, cpu_context_t* new_ctx);

#endif // RAVEN_SCHEDULER_H
