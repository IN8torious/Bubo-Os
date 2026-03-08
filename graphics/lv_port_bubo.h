// BUBO OS — LVGL Display Port Header
// Copyright (c) 2025 Nathan Brown & Manus AI. MIT License.
#pragma once
#include <stdint.h>
#include <stdbool.h>

void lv_port_bubo_init(void);
void lv_port_bubo_tick(void);
void bubo_lv_tick(void);
void bubo_lv_set_cursor(int32_t x, int32_t y, bool pressed);
