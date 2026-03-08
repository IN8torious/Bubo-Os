// =============================================================================
// BUBO OS — C-compatible interface to C++ kernel extensions
// Include this from C files (kernel.c, keyboard.c, etc.)
// The actual implementations are in C++ (.cpp) files.
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Healer ────────────────────────────────────────────────────────────────────
void healer_init(uint32_t mb_info_addr);
void healer_boot_subsystems(void);
void healer_watchdog_tick(void);

// ── Admin Panel (Kami) ────────────────────────────────────────────────────────
void admin_panel_init(void);
void admin_panel_open(void);
void admin_panel_close(void);
bool admin_panel_is_open(void);
void admin_panel_key(char c);
void admin_panel_render(void);
void admin_panel_debug_report(char* buf, uint32_t max);

// ── RAVEN Meta-Debugger ───────────────────────────────────────────────────────
void raven_init(void);
void raven_log(uint8_t event, uint8_t severity, const char* msg, uint64_t data);
void raven_watchdog_tick(void);
void raven_dump_mode_toggle(void);
void raven_render_dump(void);

// ── Hardware Detection ────────────────────────────────────────────────────────
void hwdetect_init(void);
uint32_t hwdetect_ram_mb(void);
uint32_t hwdetect_cpu_cores(void);
bool hwdetect_has_sse2(void);
bool hwdetect_has_avx2(void);
const char* hwdetect_cpu_brand(void);

// ── Low-Spec Adaptive Quality ─────────────────────────────────────────────────
void lowspec_init(void);
bool lowspec_rain_enabled(void);
bool lowspec_particles_enabled(void);
bool lowspec_shadows_enabled(void);
uint32_t lowspec_target_fps(void);

// ── UE5 Performance Co-Pilot ─────────────────────────────────────────────────
void ue5_patch_init(void);
void ue5_patch_tick(void);

#ifdef __cplusplus
}
#endif
