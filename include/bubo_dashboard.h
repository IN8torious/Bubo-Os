/*
 * BUBO OS — Dashboard Header
 * include/bubo_dashboard.h
 *
 * The BUBO dashboard is the always-visible status layer:
 * weather, CORVUS activity, Vera input state, Landon's quick-access bar.
 * It renders on top of the desktop and updates every frame tick.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Pankuch — BUBO OS Community License v1.0
 */

#ifndef BUBO_DASHBOARD_H
#define BUBO_DASHBOARD_H

#include <stdint.h>
#include "deepflow_colors.h"

/* ── Dashboard state ────────────────────────────────────────────────────── */

typedef struct {
    uint8_t  visible;           /* 1 = dashboard is shown                  */
    uint8_t  compact;           /* 1 = compact mode (accessibility bar)    */
    uint32_t last_tick_ms;      /* Last render tick                        */
} bubo_dashboard_t;

/* ── API ────────────────────────────────────────────────────────────────── */

void bubo_dashboard_init(void);
void bubo_dashboard_tick(uint32_t tick_ms);
void bubo_dashboard_show(void);
void bubo_dashboard_hide(void);
void bubo_dashboard_set_compact(uint8_t compact);

#endif /* BUBO_DASHBOARD_H */
