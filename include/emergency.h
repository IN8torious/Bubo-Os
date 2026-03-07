// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// kernel/emergency.h — Header for emergency.c
// =============================================================================
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Function to immediately halt all DRIVE agent commands and motor outputs.
// Logs the event and speaks an activation message.
void emergency_stop(void);

// Function to arm the emergency stop system. Typically called at boot.
void emergency_arm(void);

// Function to check if the emergency stop system is currently armed.
// Returns true if armed, false otherwise.
bool emergency_is_armed(void);
