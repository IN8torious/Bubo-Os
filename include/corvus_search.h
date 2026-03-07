// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/corvus_search.h — Header for corvus_search.c
// =============================================================================

#pragma once

#include <stdint.h>

// Function declarations for corvus_search.c
void corvus_search_init();
void corvus_search_open();
void corvus_search_close();
void corvus_search_draw();
void corvus_search_handle_key(uint8_t key);
void corvus_search_voice(const char* query);
