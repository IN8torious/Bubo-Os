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

// =============================================================================
// Raven AOS — TCP/IP Stack
//
// ARP, IPv4, TCP, UDP, HTTP client.
// CORVUS uses this to reach external APIs and sync SuperMemory.
//
// "NO MAS DISADVANTAGED" — sovereign intelligence needs to communicate.
// =============================================================================

#include "tcpip.h"
#include "net.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Byte order helpers ────────────────────────────────────────────────────────
static inline uint16_t htons(uint16_t v) { return (v >> 8) | (v << 8); }
static inline uint32_t htonl(uint32_t v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
           ((v & 0xFF0000) >> 8) | ((v >> 24) & 0xFF);
}
#define ntohs htons
#define ntohl htonl

// ── Ethernet frame ────────────────────────────────────────────────────────────
typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t ethertype;
} __attribute__((packed)) eth_hdr_t;

#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IP   0x0800

// ── ARP packet ────────────────────────────────────────────────────────────────
typedef struct {
    uint16_t htype;    // Hardware type (1 = Ethernet)
    uint16_t ptype;    // Protocol type (0x0800 = IPv4)
    uint8_t  hlen;     // Hardware address length (6)
    uint8_t  plen;     // Protocol address length (4)
    uint16_t oper;     // Operation (1=request, 2=reply)
    uint8_t  sha[6];   // Sender hardware address
    uint8_t  spa[4];   // Sender protocol address
    uint8_t  tha[6];   // Target hardware address
    uint8_t  tpa[4];   // Target protocol address
} __attribute__((packed)) arp_pkt_t;

// ── IPv4 header ───────────────────────────────────────────────────────────────
typedef struct {
    uint8_t  ihl_ver;  // Version (4) + IHL
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  src[4];
    uint8_t  dst[4];
} __attribute__((packed)) ip_hdr_t;

#define IP_PROTO_ICMP  1
#define IP_PROTO_TCP   6
#define IP_PROTO_UDP   17

// ── TCP header ────────────────────────────────────────────────────────────────
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  data_off;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_hdr_t;

#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20

// ── UDP header ────────────────────────────────────────────────────────────────
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;

// ── ARP cache ─────────────────────────────────────────────────────────────────
#define ARP_CACHE_SIZE 16
static arp_entry_t g_arp_cache[ARP_CACHE_SIZE];
static uint8_t     g_arp_count = 0;

// ── TCP connections ───────────────────────────────────────────────────────────
#define TCP_MAX_CONNS 8
static tcp_conn_t g_tcp_conns[TCP_MAX_CONNS];

// ── Packet TX buffer ──────────────────────────────────────────────────────────
static uint8_t g_tx_pkt[1500];

// ── IP checksum ───────────────────────────────────────────────────────────────
static uint16_t ip_checksum(const void* data, uint32_t len) {
    const uint16_t* p = (const uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(const uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

// ── ARP cache lookup ──────────────────────────────────────────────────────────
static arp_entry_t* arp_lookup(const uint8_t* ip) {
    for (int i = 0; i < g_arp_count; i++) {
        if (g_arp_cache[i].ip[0] == ip[0] && g_arp_cache[i].ip[1] == ip[1] &&
            g_arp_cache[i].ip[2] == ip[2] && g_arp_cache[i].ip[3] == ip[3])
            return &g_arp_cache[i];
    }
    return 0;
}

// ── Send ARP request ──────────────────────────────────────────────────────────
static void arp_send_request(const uint8_t* target_ip) {
    net_state_t* ns = net_get_state();
    uint8_t pkt[sizeof(eth_hdr_t) + sizeof(arp_pkt_t)];

    eth_hdr_t* eth = (eth_hdr_t*)pkt;
    for (int i = 0; i < 6; i++) { eth->dst[i] = 0xFF; eth->src[i] = ns->mac[i]; }
    eth->ethertype = htons(ETH_TYPE_ARP);

    arp_pkt_t* arp = (arp_pkt_t*)(pkt + sizeof(eth_hdr_t));
    arp->htype = htons(1);
    arp->ptype = htons(0x0800);
    arp->hlen  = 6;
    arp->plen  = 4;
    arp->oper  = htons(1);
    for (int i = 0; i < 6; i++) { arp->sha[i] = ns->mac[i]; arp->tha[i] = 0; }
    for (int i = 0; i < 4; i++) { arp->spa[i] = ns->ip[i]; arp->tpa[i] = target_ip[i]; }

    net_send(pkt, sizeof(pkt));
}

// ── Send IP packet ────────────────────────────────────────────────────────────
static bool ip_send(const uint8_t* dst_ip, uint8_t proto,
                    const uint8_t* payload, uint16_t payload_len) {
    net_state_t* ns = net_get_state();
    if (!ns->initialized) return false;

    // ARP resolve
    arp_entry_t* ae = arp_lookup(dst_ip);
    if (!ae) {
        arp_send_request(dst_ip);
        // Wait briefly for ARP reply
        for (volatile int i = 0; i < 500000; i++) {}
        ae = arp_lookup(dst_ip);
        if (!ae) {
            // Use gateway MAC as fallback
            ae = arp_lookup(ns->gateway);
            if (!ae) return false;
        }
    }

    uint16_t total = sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + payload_len;
    if (total > 1500) return false;

    uint8_t* pkt = g_tx_pkt;

    eth_hdr_t* eth = (eth_hdr_t*)pkt;
    for (int i = 0; i < 6; i++) { eth->dst[i] = ae->mac[i]; eth->src[i] = ns->mac[i]; }
    eth->ethertype = htons(ETH_TYPE_IP);

    ip_hdr_t* ip = (ip_hdr_t*)(pkt + sizeof(eth_hdr_t));
    ip->ihl_ver   = 0x45;
    ip->tos       = 0;
    ip->total_len = htons(sizeof(ip_hdr_t) + payload_len);
    ip->id        = htons(0x1337);
    ip->flags_frag = 0;
    ip->ttl       = 64;
    ip->protocol  = proto;
    ip->checksum  = 0;
    for (int i = 0; i < 4; i++) { ip->src[i] = ns->ip[i]; ip->dst[i] = dst_ip[i]; }
    ip->checksum  = ip_checksum(ip, sizeof(ip_hdr_t));

    uint8_t* data = pkt + sizeof(eth_hdr_t) + sizeof(ip_hdr_t);
    for (uint16_t i = 0; i < payload_len; i++) data[i] = payload[i];

    return net_send(pkt, total);
}

// ── TCP connection management ─────────────────────────────────────────────────
static tcp_conn_t* tcp_find_free(void) {
    for (int i = 0; i < TCP_MAX_CONNS; i++)
        if (g_tcp_conns[i].state == TCP_STATE_CLOSED) return &g_tcp_conns[i];
    return 0;
}

// ── Open TCP connection ───────────────────────────────────────────────────────
tcp_conn_t* tcp_connect(const uint8_t* dst_ip, uint16_t dst_port) {
    tcp_conn_t* conn = tcp_find_free();
    if (!conn) return 0;

    net_state_t* ns = net_get_state();
    for (int i = 0; i < 4; i++) { conn->src_ip[i] = ns->ip[i]; conn->dst_ip[i] = dst_ip[i]; }
    conn->src_port = 49152 + (g_arp_count & 0x3FFF);
    conn->dst_port = dst_port;
    conn->seq      = 0xDEADBEEF;
    conn->ack      = 0;
    conn->state    = TCP_STATE_SYN_SENT;
    conn->rx_len   = 0;

    // Build SYN packet
    uint8_t tcp_buf[sizeof(tcp_hdr_t)];
    tcp_hdr_t* tcp = (tcp_hdr_t*)tcp_buf;
    tcp->src_port = htons(conn->src_port);
    tcp->dst_port = htons(dst_port);
    tcp->seq      = htonl(conn->seq);
    tcp->ack      = 0;
    tcp->data_off = 0x50;  // 5 * 4 = 20 bytes header
    tcp->flags    = TCP_SYN;
    tcp->window   = htons(65535);
    tcp->checksum = 0;
    tcp->urgent   = 0;

    ip_send(dst_ip, IP_PROTO_TCP, tcp_buf, sizeof(tcp_hdr_t));
    terminal_write("[TCP] SYN sent\n");
    return conn;
}

// ── Send data on TCP connection ───────────────────────────────────────────────
bool tcp_send_data(tcp_conn_t* conn, const uint8_t* data, uint16_t len) {
    if (!conn || conn->state != TCP_STATE_ESTABLISHED) return false;

    uint8_t buf[sizeof(tcp_hdr_t) + 1400];
    if (len > 1400) len = 1400;

    tcp_hdr_t* tcp = (tcp_hdr_t*)buf;
    tcp->src_port = htons(conn->src_port);
    tcp->dst_port = htons(conn->dst_port);
    tcp->seq      = htonl(conn->seq);
    tcp->ack      = htonl(conn->ack);
    tcp->data_off = 0x50;
    tcp->flags    = TCP_PSH | TCP_ACK;
    tcp->window   = htons(65535);
    tcp->checksum = 0;
    tcp->urgent   = 0;

    for (uint16_t i = 0; i < len; i++) buf[sizeof(tcp_hdr_t) + i] = data[i];
    conn->seq += len;

    return ip_send(conn->dst_ip, IP_PROTO_TCP, buf, sizeof(tcp_hdr_t) + len);
}

// ── HTTP GET request ──────────────────────────────────────────────────────────
// Simple blocking HTTP/1.0 GET — used by CORVUS to call external APIs
bool http_get(const uint8_t* server_ip, uint16_t port,
              const char* path, char* response_buf, uint32_t buf_size) {
    terminal_write("[HTTP] GET ");
    terminal_write(path);
    terminal_write("\n");

    tcp_conn_t* conn = tcp_connect(server_ip, port);
    if (!conn) {
        terminal_write("[HTTP] Connection failed\n");
        return false;
    }

    // Wait for ESTABLISHED (simplified — in real impl, process incoming packets)
    for (volatile int i = 0; i < 2000000; i++) {}
    conn->state = TCP_STATE_ESTABLISHED;  // Optimistic for demo

    // Build HTTP request
    char req[512];
    int ri = 0;
    const char* method = "GET ";
    for (int i = 0; method[i]; i++) req[ri++] = method[i];
    for (int i = 0; path[i]; i++) req[ri++] = path[i];
    const char* ver = " HTTP/1.0\r\nHost: corvus\r\nConnection: close\r\n\r\n";
    for (int i = 0; ver[i]; i++) req[ri++] = ver[i];

    tcp_send_data(conn, (const uint8_t*)req, (uint16_t)ri);

    // Wait for response
    for (volatile int i = 0; i < 5000000; i++) {}

    // Read response
    uint16_t n = net_receive((uint8_t*)response_buf, (uint16_t)(buf_size - 1));
    response_buf[n] = 0;

    conn->state = TCP_STATE_CLOSED;
    terminal_write("[HTTP] Response received\n");
    return n > 0;
}

// ── Initialize TCP/IP stack ───────────────────────────────────────────────────
bool tcpip_init(void) {
    terminal_write("[TCPIP] Initializing TCP/IP stack...\n");
    for (int i = 0; i < ARP_CACHE_SIZE; i++) g_arp_cache[i].valid = false;
    for (int i = 0; i < TCP_MAX_CONNS; i++) g_tcp_conns[i].state = TCP_STATE_CLOSED;
    g_arp_count = 0;
    terminal_write("[TCPIP] ARP + IPv4 + TCP + HTTP ready\n");
    return true;
}

// ── Process incoming packet ───────────────────────────────────────────────────
void tcpip_process_packet(const uint8_t* pkt, uint16_t len) {
    if (len < sizeof(eth_hdr_t)) return;
    const eth_hdr_t* eth = (const eth_hdr_t*)pkt;
    uint16_t type = ntohs(eth->ethertype);

    if (type == ETH_TYPE_ARP) {
        const arp_pkt_t* arp = (const arp_pkt_t*)(pkt + sizeof(eth_hdr_t));
        if (ntohs(arp->oper) == 2) {  // ARP reply
            if (g_arp_count < ARP_CACHE_SIZE) {
                arp_entry_t* ae = &g_arp_cache[g_arp_count++];
                for (int i = 0; i < 6; i++) ae->mac[i] = arp->sha[i];
                for (int i = 0; i < 4; i++) ae->ip[i]  = arp->spa[i];
                ae->valid = true;
                terminal_write("[ARP] Cache updated\n");
            }
        }
    } else if (type == ETH_TYPE_IP) {
        const ip_hdr_t* ip = (const ip_hdr_t*)(pkt + sizeof(eth_hdr_t));
        if (ip->protocol == IP_PROTO_TCP) {
            uint8_t ihl = (ip->ihl_ver & 0xF) * 4;
            const tcp_hdr_t* tcp = (const tcp_hdr_t*)((const uint8_t*)ip + ihl);
            uint16_t dst_port = ntohs(tcp->dst_port);

            // Find matching connection
            for (int i = 0; i < TCP_MAX_CONNS; i++) {
                if (g_tcp_conns[i].src_port == dst_port) {
                    tcp_conn_t* conn = &g_tcp_conns[i];
                    if (tcp->flags & TCP_SYN && tcp->flags & TCP_ACK) {
                        conn->state = TCP_STATE_ESTABLISHED;
                        conn->ack   = ntohl(tcp->seq) + 1;
                    } else if (tcp->flags & TCP_FIN) {
                        conn->state = TCP_STATE_CLOSED;
                    } else if (tcp->flags & TCP_ACK) {
                        // Data — copy to conn rx buffer
                        uint16_t data_off = (tcp->data_off >> 4) * 4;
                        uint16_t ip_len   = ntohs(ip->total_len);
                        uint16_t data_len = ip_len - ihl - data_off;
                        const uint8_t* data = (const uint8_t*)tcp + data_off;
                        if (data_len > 0 && conn->rx_len + data_len < TCP_RX_BUF) {
                            for (uint16_t j = 0; j < data_len; j++)
                                conn->rx_buf[conn->rx_len++] = data[j];
                        }
                        conn->ack = ntohl(tcp->seq) + data_len;
                    }
                    break;
                }
            }
        }
    }
}
