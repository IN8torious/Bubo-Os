// =============================================================================
// BUBO OS — Admin Panel Header
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Forward declarations for healer subsystems (defined in healer.cpp)
class FramebufferSubsystem;
class SoundSubsystem;
class NetworkSubsystem;
extern FramebufferSubsystem* g_fb_subsystem;
extern SoundSubsystem*       g_snd_subsystem;
extern NetworkSubsystem*     g_net_subsystem;

#ifdef __cplusplus
extern "C" {
#endif

// Open/close the admin panel (F12 key)
void admin_panel_open(void);
void admin_panel_close(void);
bool admin_panel_is_open(void);

// Feed a keypress to the panel (called from keyboard interrupt handler)
void admin_panel_key(char c);

// Generate a full debug report string — paste to Manus for help
void admin_panel_debug_report(char* buf, uint32_t max);

#ifdef __cplusplus
}
#endif
