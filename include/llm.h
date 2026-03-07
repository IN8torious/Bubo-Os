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

#pragma once
// =============================================================================
// Raven AOS — CORVUS LLM Engine Header (Cactus/GGML-compatible)
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     initialized;
    bool     loaded;          // True if model weights are in memory
    uint64_t model_size;      // Bytes
    uint32_t ctx_used;        // Tokens used in current context
    uint64_t inferences;      // Total inference count
} llm_state_t;

bool         llm_init(void);
bool         llm_infer(const char* prompt, char* output, uint32_t max_output);
bool         llm_contains(const char* haystack, const char* needle);
void         llm_reset_context(void);
llm_state_t* llm_get_state(void);
