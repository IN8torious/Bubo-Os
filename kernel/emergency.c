// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/emergency.c — Emergency Stop System
// Highest-priority kernel interrupt. One word halts everything.
// Built for Landon. Built for everyone who needs a reliable stop.
// =============================================================================

#include "emergency.h"
#include "corvus.h"
#include "voice.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

#define EMERGENCY_MAX_CALLBACKS 8

typedef enum {
    EMERGENCY_STOP = 0,
    EMERGENCY_ABORT,
    EMERGENCY_KERNEL_PANIC,
    EMERGENCY_VOICE,
} emergency_reason_t;

typedef void (*emergency_callback_t)(emergency_reason_t reason);


// Emergency state
static bool g_emergency_active = false;
static uint32_t g_emergency_timestamp = 0;
static emergency_callback_t g_callbacks[EMERGENCY_MAX_CALLBACKS];
static uint32_t g_callback_count = 0;

// Emergency trigger words — checked before any other voice processing
static const char* EMERGENCY_WORDS[] = {
    "STOP", "ABORT", "HALT", "EMERGENCY", "HELP", "CANCEL",
    "STO",  "ABO",   "HAL", "EMERG",     "HEP",  "CANC",
    (const char*)0
};

static bool str_eq_upper(const char* a, const char* b) {
    uint32_t i = 0;
    while (a[i] && b[i]) {
        char ca = a[i], cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return false;
        i++;
    }
    return a[i] == 0 && b[i] == 0;
}

static bool str_pfx_upper(const char* s, const char* pfx) {
    uint32_t i = 0;
    while (pfx[i]) {
        char cs = s[i], cp = pfx[i];
        if (cs >= 'a' && cs <= 'z') cs -= 32;
        if (cp >= 'a' && cp <= 'z') cp -= 32;
        if (cs != cp) return false;
        i++;
    }
    return true;
}

void emergency_init(void) {
    g_emergency_active  = false;
    g_emergency_timestamp = 0;
    g_callback_count    = 0;
    terminal_write("[EMERGENCY] Emergency stop system armed. Say STOP or ABORT to halt.\n");
}

bool emergency_check_word(const char* word) {
    if (!word || !word[0]) return false;
    for (uint32_t i = 0; EMERGENCY_WORDS[i]; i++) {
        if (str_eq_upper(word, EMERGENCY_WORDS[i])) return true;
        if (str_pfx_upper(word, EMERGENCY_WORDS[i])) return true;
    }
    return false;
}

void emergency_trigger(emergency_reason_t reason) {
    if (g_emergency_active) return;  // Already stopped
    g_emergency_active = true;

    // Immediately halt all CORVUS agents except HEAL and SEC
    corvus_emergency_halt();

    // Notify all registered callbacks
    for (uint32_t i = 0; i < g_callback_count; i++) {
        if (g_callbacks[i]) g_callbacks[i](reason);
    }

    // Visual alert
    terminal_write("\n");
    terminal_write("!!! CORVUS EMERGENCY STOP ACTIVATED !!!\n");
    terminal_write("All systems halted. Say RESUME to continue.\n");
    terminal_write("\n");
}

void emergency_resume(void) {
    if (!g_emergency_active) return;
    g_emergency_active = false;
    corvus_emergency_resume();
    terminal_write("[EMERGENCY] Systems resumed. CORVUS active.\n");
}

bool emergency_is_active(void) {
    return g_emergency_active;
}

void emergency_register_callback(emergency_callback_t cb) {
    if (g_callback_count < EMERGENCY_MAX_CALLBACKS && cb) {
        g_callbacks[g_callback_count++] = cb;
    }
}

void emergency_voice_check(const char* word) {
    // Called from voice pipeline BEFORE any other processing
    if (emergency_check_word(word)) {
        if (g_emergency_active) {
            // Second STOP = resume
            emergency_resume();
        } else {
            emergency_trigger(EMERGENCY_VOICE);
        }
    }
}
