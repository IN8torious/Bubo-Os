// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
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
// Deep Flow OS — TCP/IP Stack Header
// =============================================================================
#include <stdint.h>
#include <stdbool.h>

#define TCP_RX_BUF 4096

typedef enum {
    TCP_STATE_CLOSED = 0,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT,
} tcp_state_t;

typedef struct {
    uint8_t     ip[4];
    uint8_t     mac[6];
    bool        valid;
} arp_entry_t;

typedef struct {
    uint8_t     src_ip[4];
    uint8_t     dst_ip[4];
    uint16_t    src_port;
    uint16_t    dst_port;
    uint32_t    seq;
    uint32_t    ack;
    tcp_state_t state;
    uint8_t     rx_buf[TCP_RX_BUF];
    uint16_t    rx_len;
} tcp_conn_t;

bool        tcpip_init(void);
void        tcpip_process_packet(const uint8_t* pkt, uint16_t len);
tcp_conn_t* tcp_connect(const uint8_t* dst_ip, uint16_t dst_port);
bool        tcp_send_data(tcp_conn_t* conn, const uint8_t* data, uint16_t len);
bool        http_get(const uint8_t* server_ip, uint16_t port,
                     const char* path, char* response_buf, uint32_t buf_size);
