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

// =============================================================================
// Instinct OS — Network Driver (RTL8139 + Intel E1000)
//
// Bare-metal Ethernet driver. CORVUS uses this to reach external APIs,
// sync SuperMemory, and communicate with the outside world.
//
// "NO MAS DISADVANTAGED" — sovereign intelligence needs a voice to the world.
// =============================================================================

#include "net.h"
#include "vga.h"
#include "port.h"
#include <stdint.h>
#include <stdbool.h>

// ── RTL8139 registers ─────────────────────────────────────────────────────────
#define RTL_IDR0        0x00   // MAC address
#define RTL_MAR0        0x08   // Multicast filter
#define RTL_TSD0        0x10   // Tx status descriptor 0-3
#define RTL_TSAD0       0x20   // Tx start address 0-3
#define RTL_RBSTART     0x30   // Rx buffer start
#define RTL_ERBCR       0x34   // Early Rx byte count
#define RTL_ERSR        0x36   // Early Rx status
#define RTL_CR          0x37   // Command register
#define RTL_CAPR        0x38   // Current address of packet read
#define RTL_CBR         0x3A   // Current buffer address
#define RTL_IMR         0x3C   // Interrupt mask
#define RTL_ISR         0x3E   // Interrupt status
#define RTL_TCR         0x40   // Tx config
#define RTL_RCR         0x44   // Rx config
#define RTL_TCTR        0x48   // Timer count
#define RTL_MPC         0x4C   // Missed packet counter
#define RTL_9346CR      0x50   // 9346 command
#define RTL_CONFIG0     0x51
#define RTL_CONFIG1     0x52
#define RTL_BMCR        0x62   // Basic mode control (MII)

// RTL command bits
#define RTL_CR_RST      0x10
#define RTL_CR_RE       0x08   // Rx enable
#define RTL_CR_TE       0x04   // Tx enable
#define RTL_CR_BUFE     0x01   // Rx buffer empty

// RTL Rx config
#define RTL_RCR_AAP     0x01   // Accept all packets
#define RTL_RCR_APM     0x02   // Accept physical match
#define RTL_RCR_AM      0x04   // Accept multicast
#define RTL_RCR_AB      0x08   // Accept broadcast
#define RTL_RCR_RBLEN8K 0x00   // 8KB Rx buffer
#define RTL_RCR_MXDMA   0x700  // Max DMA burst = unlimited
#define RTL_RCR_WRAP    0x80   // Wrap around

// RTL Tx status bits
#define RTL_TSD_OWN     0x2000
#define RTL_TSD_TUN     0x4000
#define RTL_TSD_TOK     0x8000

// ── Intel E1000 registers ─────────────────────────────────────────────────────
#define E1000_CTRL      0x0000
#define E1000_STATUS    0x0008
#define E1000_EECD      0x0010
#define E1000_EERD      0x0014
#define E1000_ICR       0x00C0
#define E1000_IMS       0x00D0
#define E1000_RCTL      0x0100
#define E1000_TCTL      0x0400
#define E1000_RDBAL     0x2800
#define E1000_RDBAH     0x2804
#define E1000_RDLEN     0x2808
#define E1000_RDH       0x2810
#define E1000_RDT       0x2818
#define E1000_TDBAL     0x3800
#define E1000_TDBAH     0x3804
#define E1000_TDLEN     0x3808
#define E1000_TDH       0x3810
#define E1000_TDT       0x3818
#define E1000_RAL0      0x5400
#define E1000_RAH0      0x5404

// E1000 control bits
#define E1000_CTRL_RST  (1 << 26)
#define E1000_CTRL_ASDE (1 << 5)
#define E1000_CTRL_SLU  (1 << 6)
#define E1000_RCTL_EN   (1 << 1)
#define E1000_RCTL_SBP  (1 << 2)
#define E1000_RCTL_UPE  (1 << 3)
#define E1000_RCTL_MPE  (1 << 4)
#define E1000_RCTL_BAM  (1 << 15)
#define E1000_RCTL_BSIZE_4096 (3 << 16)
#define E1000_RCTL_SECRC (1 << 26)
#define E1000_TCTL_EN   (1 << 1)
#define E1000_TCTL_PSP  (1 << 3)

// ── Buffers ───────────────────────────────────────────────────────────────────
#define RX_BUF_SIZE     (8192 + 16 + 1500)
#define TX_BUF_SIZE     1536
#define TX_BUF_COUNT    4
#define RX_DESC_COUNT   32
#define TX_DESC_COUNT   8

static uint8_t  g_rx_buf[RX_BUF_SIZE]  __attribute__((aligned(4)));
static uint8_t  g_tx_buf[TX_BUF_COUNT][TX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t  g_tx_idx = 0;

// E1000 descriptors
typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

static e1000_rx_desc_t g_rx_descs[RX_DESC_COUNT] __attribute__((aligned(16)));
static e1000_tx_desc_t g_tx_descs[TX_DESC_COUNT] __attribute__((aligned(16)));
static uint8_t g_e1000_rx_bufs[RX_DESC_COUNT][2048] __attribute__((aligned(4)));

// ── Driver state ──────────────────────────────────────────────────────────────
static net_state_t g_net;
static uint16_t    g_rtl_base = 0;
static uint32_t*   g_e1000_mmio = 0;
static bool        g_use_e1000 = false;

// Receive ring buffer
#define NET_RX_RING  (16 * 1024)
static uint8_t  g_net_rx_ring[NET_RX_RING];
static uint32_t g_net_rx_write = 0;
static uint32_t g_net_rx_read  = 0;

// ── PCI helpers ───────────────────────────────────────────────────────────────
static uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC);
    port_outd(0xCF8, addr);
    return port_ind(0xCFC);
}
static void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC);
    port_outd(0xCF8, addr);
    port_outd(0xCFC, val);
}

// ── E1000 MMIO helpers ────────────────────────────────────────────────────────
static inline void e1000_write(uint32_t reg, uint32_t val) {
    volatile uint32_t* p = (volatile uint32_t*)((uint8_t*)g_e1000_mmio + reg);
    *p = val;
}
static inline uint32_t e1000_read(uint32_t reg) {
    volatile uint32_t* p = (volatile uint32_t*)((uint8_t*)g_e1000_mmio + reg);
    return *p;
}

// ── Scan PCI for NIC ──────────────────────────────────────────────────────────
static bool net_find_nic(void) {
    for (uint8_t bus = 0; bus < 4; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0);
            uint16_t vendor = (uint16_t)(id & 0xFFFF);
            uint16_t device = (uint16_t)(id >> 16);

            // RTL8139
            if (vendor == 0x10EC && device == 0x8139) {
                uint32_t bar0 = pci_read(bus, dev, 0, 0x10) & ~0x3;
                g_rtl_base = (uint16_t)bar0;
                g_use_e1000 = false;
                // Enable bus mastering
                uint32_t cmd = pci_read(bus, dev, 0, 0x04);
                pci_write(bus, dev, 0, 0x04, cmd | 0x07);
                terminal_write("[NET] RTL8139 found\n");
                return true;
            }

            // Intel E1000 (82540, 82545, 82574, QEMU default)
            if (vendor == 0x8086 &&
                (device == 0x100E || device == 0x100F || device == 0x10D3 ||
                 device == 0x1502 || device == 0x1503)) {
                uint32_t bar0 = pci_read(bus, dev, 0, 0x10) & ~0xF;
                g_e1000_mmio = (uint32_t*)(uint64_t)bar0;
                g_use_e1000 = true;
                uint32_t cmd = pci_read(bus, dev, 0, 0x04);
                pci_write(bus, dev, 0, 0x04, cmd | 0x07);
                terminal_write("[NET] Intel E1000 found\n");
                return true;
            }
        }
    }
    return false;
}

// ── RTL8139 init ──────────────────────────────────────────────────────────────
static void rtl_init(void) {
    // Power on
    port_outb(g_rtl_base + RTL_CONFIG1, 0x00);
    // Software reset
    port_outb(g_rtl_base + RTL_CR, RTL_CR_RST);
    while (port_inb(g_rtl_base + RTL_CR) & RTL_CR_RST) {}

    // Read MAC address
    for (int i = 0; i < 6; i++)
        g_net.mac[i] = port_inb(g_rtl_base + RTL_IDR0 + i);

    // Set Rx buffer
    port_outd(g_rtl_base + RTL_RBSTART, (uint32_t)(uint64_t)g_rx_buf);

    // Enable interrupts: Tx OK, Rx OK, Tx error, Rx error
    port_outw(g_rtl_base + RTL_IMR, 0x0005);

    // Rx config: accept broadcast + physical match, 8KB buffer, no wrap
    port_outd(g_rtl_base + RTL_RCR,
              RTL_RCR_AB | RTL_RCR_APM | RTL_RCR_AM |
              RTL_RCR_MXDMA | RTL_RCR_WRAP);

    // Tx config: max DMA burst
    port_outd(g_rtl_base + RTL_TCR, 0x03000700);

    // Enable Rx + Tx
    port_outb(g_rtl_base + RTL_CR, RTL_CR_RE | RTL_CR_TE);
}

// ── E1000 init ────────────────────────────────────────────────────────────────
static void e1000_init(void) {
    // Reset
    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | E1000_CTRL_RST);
    for (volatile int i = 0; i < 100000; i++) {}

    // Auto-speed detect + link up
    e1000_write(E1000_CTRL, E1000_CTRL_ASDE | E1000_CTRL_SLU);

    // Read MAC from EEPROM
    for (int i = 0; i < 3; i++) {
        e1000_write(E1000_EERD, 1 | ((uint32_t)i << 8));
        uint32_t v;
        do { v = e1000_read(E1000_EERD); } while (!(v & 0x10));
        uint16_t w = (uint16_t)(v >> 16);
        g_net.mac[i*2]   = (uint8_t)(w & 0xFF);
        g_net.mac[i*2+1] = (uint8_t)(w >> 8);
    }

    // Set MAC in receive address register
    uint32_t ral = g_net.mac[0] | ((uint32_t)g_net.mac[1] << 8) |
                   ((uint32_t)g_net.mac[2] << 16) | ((uint32_t)g_net.mac[3] << 24);
    uint32_t rah = g_net.mac[4] | ((uint32_t)g_net.mac[5] << 8) | (1u << 31);
    e1000_write(E1000_RAL0, ral);
    e1000_write(E1000_RAH0, rah);

    // Setup Rx descriptors
    for (int i = 0; i < RX_DESC_COUNT; i++) {
        g_rx_descs[i].addr   = (uint64_t)g_e1000_rx_bufs[i];
        g_rx_descs[i].status = 0;
    }
    e1000_write(E1000_RDBAL, (uint32_t)(uint64_t)g_rx_descs);
    e1000_write(E1000_RDBAH, (uint32_t)((uint64_t)g_rx_descs >> 32));
    e1000_write(E1000_RDLEN, RX_DESC_COUNT * sizeof(e1000_rx_desc_t));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, RX_DESC_COUNT - 1);
    e1000_write(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM |
                             E1000_RCTL_BSIZE_4096 | E1000_RCTL_SECRC);

    // Setup Tx descriptors
    for (int i = 0; i < TX_DESC_COUNT; i++) {
        g_tx_descs[i].status = 0xFF;  // Mark as done
    }
    e1000_write(E1000_TDBAL, (uint32_t)(uint64_t)g_tx_descs);
    e1000_write(E1000_TDBAH, (uint32_t)((uint64_t)g_tx_descs >> 32));
    e1000_write(E1000_TDLEN, TX_DESC_COUNT * sizeof(e1000_tx_desc_t));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    e1000_write(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | (0x10 << 4) | (0x40 << 12));

    // Enable interrupts
    e1000_write(E1000_IMS, 0x1F6DC);
}

// ── Initialize network driver ─────────────────────────────────────────────────
bool net_init(void) {
    terminal_write("[NET] Initializing network stack...\n");

    g_net.initialized = false;
    g_net.link_up     = false;
    g_net.rx_packets  = 0;
    g_net.tx_packets  = 0;
    g_net.rx_bytes    = 0;
    g_net.tx_bytes    = 0;

    // Default IP config (DHCP will override)
    g_net.ip[0] = 10; g_net.ip[1] = 0; g_net.ip[2] = 2; g_net.ip[3] = 15;
    g_net.gateway[0] = 10; g_net.gateway[1] = 0; g_net.gateway[2] = 2; g_net.gateway[3] = 2;
    g_net.netmask[0] = 255; g_net.netmask[1] = 255; g_net.netmask[2] = 255; g_net.netmask[3] = 0;

    if (!net_find_nic()) {
        terminal_write("[NET] No NIC found — network unavailable\n");
        return false;
    }

    if (g_use_e1000) e1000_init();
    else             rtl_init();

    g_net.initialized = true;
    g_net.link_up     = true;

    terminal_write("[NET] MAC: ");
    for (int i = 0; i < 6; i++) {
        char hex[3];
        hex[0] = "0123456789ABCDEF"[g_net.mac[i] >> 4];
        hex[1] = "0123456789ABCDEF"[g_net.mac[i] & 0xF];
        hex[2] = 0;
        terminal_write(hex);
        if (i < 5) terminal_write(":");
    }
    terminal_write("\n");
    terminal_write("[NET] Network stack ready — CORVUS can reach the world\n");
    return true;
}

// ── Send a raw Ethernet frame ─────────────────────────────────────────────────
bool net_send(const uint8_t* data, uint16_t len) {
    if (!g_net.initialized || !g_net.link_up) return false;
    if (len > TX_BUF_SIZE) return false;

    if (g_use_e1000) {
        uint32_t tail = e1000_read(E1000_TDT);
        // Copy to tx buffer
        for (uint16_t i = 0; i < len; i++)
            g_tx_buf[tail % TX_BUF_COUNT][i] = data[i];
        g_tx_descs[tail].addr   = (uint64_t)g_tx_buf[tail % TX_BUF_COUNT];
        g_tx_descs[tail].length = len;
        g_tx_descs[tail].cmd    = 0x0B;  // EOP | IFCS | RS
        g_tx_descs[tail].status = 0;
        e1000_write(E1000_TDT, (tail + 1) % TX_DESC_COUNT);
    } else {
        // RTL8139 — use one of 4 Tx descriptors
        uint8_t idx = g_tx_idx & 0x3;
        for (uint16_t i = 0; i < len; i++)
            g_tx_buf[idx][i] = data[i];
        port_outd(g_rtl_base + RTL_TSAD0 + idx * 4, (uint32_t)(uint64_t)g_tx_buf[idx]);
        port_outd(g_rtl_base + RTL_TSD0  + idx * 4, len & 0x1FFF);
        g_tx_idx++;
    }

    g_net.tx_packets++;
    g_net.tx_bytes += len;
    return true;
}

// ── Receive a raw Ethernet frame ──────────────────────────────────────────────
uint16_t net_receive(uint8_t* buf, uint16_t max_len) {
    if (!g_net.initialized) return 0;

    uint32_t avail = (g_net_rx_write - g_net_rx_read);
    if (avail == 0) return 0;
    if (avail > max_len) avail = max_len;

    for (uint32_t i = 0; i < avail; i++)
        buf[i] = g_net_rx_ring[(g_net_rx_read + i) % NET_RX_RING];
    g_net_rx_read = (g_net_rx_read + avail) % NET_RX_RING;

    g_net.rx_packets++;
    g_net.rx_bytes += avail;
    return (uint16_t)avail;
}

// ── Get state ─────────────────────────────────────────────────────────────────
net_state_t* net_get_state(void) {
    return &g_net;
}
