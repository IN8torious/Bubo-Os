#pragma once
// =============================================================================
// Raven AOS — CORVUS MAS Constitutional Governance Layer Header
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

void corvus_constitution_init(void);
bool corvus_evaluate_action(uint32_t agent_id, const char* action_desc);
void corvus_print_constitution(void);
