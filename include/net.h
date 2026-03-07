// =============================================================================
// Instinct OS — Dedicated to Landon Pankuch
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
// Instinct OS — Network Driver Header (RTL8139 / Intel E1000)
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     initialized;
    bool     link_up;
    uint8_t  mac[6];
    uint8_t  ip[4];
    uint8_t  gateway[4];
    uint8_t  netmask[4];
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} net_state_t;

bool         net_init(void);
bool         net_send(const uint8_t* data, uint16_t len);
uint16_t     net_receive(uint8_t* buf, uint16_t max_len);
net_state_t* net_get_state(void);
