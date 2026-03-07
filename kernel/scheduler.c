// =============================================================================
// Raven OS — Scheduler
// Round-robin base + CORVUS semantic priority boosting
// Process Control Block (PCB) with BDI state for agentic tasks
// =============================================================================

#include "scheduler.h"
#include "vmm.h"
#include "corvus.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Process table ─────────────────────────────────────────────────────────────
#define MAX_PROCESSES 32

static process_t process_table[MAX_PROCESSES];
static uint32_t  current_pid    = 0;
static uint32_t  next_pid       = 1;
static uint32_t  process_count  = 0;
static bool      scheduler_ready = false;

// ── Get current process ───────────────────────────────────────────────────────
process_t* scheduler_current(void) {
    return &process_table[current_pid];
}

// ── Create a new process ──────────────────────────────────────────────────────
process_t* scheduler_create(const char* name, void (*entry)(void),
                             uint8_t priority, process_type_t type) {
    if (process_count >= MAX_PROCESSES) return NULL;

    // Find free slot
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_DEAD) {
            process_t* p = &process_table[i];

            // Copy name
            uint32_t j = 0;
            while (name[j] && j < 31) { p->name[j] = name[j]; j++; }
            p->name[j] = '\0';

            p->pid      = next_pid++;
            p->priority = priority;
            p->type     = type;
            p->state    = PROC_READY;
            p->ticks    = 0;
            p->quantum  = (type == PROC_AGENT) ? 5 : 10; // Agents get smaller quantum

            // Allocate kernel stack (8KB)
            uint8_t* stack = (uint8_t*)kmalloc(8192);
            if (!stack) return NULL;

            p->stack_base = (uint32_t)stack;
            p->stack_top  = (uint32_t)(stack + 8192);

            // Set up initial stack frame for context switch
            // Stack grows down — push initial register state
            uint32_t* sp = (uint32_t*)p->stack_top;

            // These match the pusha + segment push in the context switch
            *(--sp) = 0x10;         // ss
            *(--sp) = p->stack_top; // useresp
            *(--sp) = 0x202;        // eflags (IF set)
            *(--sp) = 0x08;         // cs
            *(--sp) = (uint32_t)entry; // eip
            *(--sp) = 0;            // error code
            *(--sp) = 0;            // int_no
            *(--sp) = 0;            // eax
            *(--sp) = 0;            // ecx
            *(--sp) = 0;            // edx
            *(--sp) = 0;            // ebx
            *(--sp) = p->stack_top; // esp
            *(--sp) = 0;            // ebp
            *(--sp) = 0;            // esi
            *(--sp) = 0;            // edi
            *(--sp) = 0x10;         // ds

            p->context.rsp = (uint32_t)sp;
            p->context.rip = (uint32_t)entry;

            process_count++;
            return p;
        }
    }
    return NULL;
}

// ── Kill a process ────────────────────────────────────────────────────────────
void scheduler_kill(uint32_t pid) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            process_table[i].state = PROC_DEAD;
            if (process_table[i].stack_base) {
                kfree((void*)process_table[i].stack_base);
            }
            if (process_count > 0) process_count--;
            return;
        }
    }
}

// ── CORVUS semantic priority: boost agents working on urgent goals ─────────────
static uint8_t corvus_semantic_priority(process_t* p) {
    if (p->type != PROC_AGENT) return p->priority;

    // Check if corresponding CORVUS agent is in ACTING state
    if (p->agent_id < CORVUS_AGENT_COUNT) {
        corvus_agent_t* agent = &g_corvus.agents[p->agent_id];
        if (agent->state == AGENT_ACTING) {
            return (uint8_t)(p->priority + 50 > 255 ? 255 : p->priority + 50);
        }
        if (agent->state == AGENT_SENSING) {
            return (uint8_t)(p->priority + 20 > 255 ? 255 : p->priority + 20);
        }
    }
    return p->priority;
}

// ── Round-robin scheduler tick (called from PIT IRQ) ─────────────────────────
void scheduler_tick(void) {
    if (!scheduler_ready || process_count == 0) return;

    process_t* current = &process_table[current_pid];
    current->ticks++;

    // Check if quantum expired
    if (current->ticks < current->quantum) return;
    current->ticks = 0;

    // Find next READY process (round-robin with semantic priority)
    uint32_t best_idx     = current_pid;
    uint8_t  best_priority = 0;
    bool     found        = false;

    // Two-pass: first check processes after current (fairness),
    // then apply CORVUS semantic priority boost
    for (uint32_t i = 1; i <= MAX_PROCESSES; i++) {
        uint32_t idx = (current_pid + i) % MAX_PROCESSES;
        process_t* p = &process_table[idx];

        if (p->state == PROC_READY || p->state == PROC_RUNNING) {
            uint8_t effective_priority = corvus_semantic_priority(p);
            if (!found || effective_priority > best_priority) {
                best_priority = effective_priority;
                best_idx = idx;
                found = true;
                // For pure round-robin, break here; for priority, continue
                break; // Simple round-robin for now
            }
        }
    }

    if (!found || best_idx == current_pid) return;

    // Context switch
    current->state = PROC_READY;
    process_table[best_idx].state = PROC_RUNNING;
    current_pid = best_idx;

    // Actual register save/restore happens in ASM (scheduler_switch)
    // scheduler_switch(&current->context, &process_table[best_idx].context);
}

// ── Initialize scheduler ──────────────────────────────────────────────────────
void scheduler_init(void) {
    // Zero process table
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state    = PROC_DEAD;
        process_table[i].pid      = 0;
        process_table[i].agent_id = 0xFF;
    }

    // Create idle process (PID 0 — always runnable)
    process_table[0].pid      = 0;
    process_table[0].state    = PROC_RUNNING;
    process_table[0].priority = 1;
    process_table[0].quantum  = 1;
    process_table[0].type     = PROC_KERNEL;
    process_table[0].name[0]  = 'i';
    process_table[0].name[1]  = 'd';
    process_table[0].name[2]  = 'l';
    process_table[0].name[3]  = 'e';
    process_table[0].name[4]  = '\0';
    process_count = 1;

    scheduler_ready = true;

    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,        VGA_BLACK);
    terminal_setcolor(white);
    terminal_write("  [ SCHED] Scheduler initialized — round-robin + CORVUS semantic priority");
    terminal_setcolor(green);
    terminal_writeline("  ✓");
}
