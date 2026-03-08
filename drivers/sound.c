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

// =============================================================================
// Deep Flow OS — Sound Driver (AC97 / Intel HDA)
//
// Handles audio capture (microphone) and playback (speaker).
// This is the hardware layer beneath Landon's voice interface.
// CORVUS listens through this driver. CORVUS speaks through this driver.
//
// "NO MAS DISADVANTAGED" — Landon speaks, the machine obeys.
// =============================================================================

#include "sound.h"
#include "vga.h"
#include "port.h"
#include <stdint.h>
#include <stdbool.h>

// ── AC97 I/O ports (via PCI BAR0 = Native Audio Mixer, BAR1 = Native Audio Bus Master) ──
// These are the standard AC97 port offsets
#define AC97_MIXER_BASE     0x0000   // Detected at runtime from PCI
#define AC97_BUS_BASE       0x0100

// AC97 Mixer registers
#define AC97_RESET          0x00
#define AC97_MASTER_VOL     0x02
#define AC97_HEADPHONE_VOL  0x04
#define AC97_MONO_VOL       0x06
#define AC97_PCM_OUT_VOL    0x18
#define AC97_MIC_VOL        0x0E
#define AC97_LINE_IN_VOL    0x10
#define AC97_REC_SELECT     0x1A   // Record select (0=mic, 1=CD, 3=line-in)
#define AC97_REC_GAIN       0x1C
#define AC97_SAMPLE_RATE    0x2C   // PCM front DAC rate
#define AC97_MIC_RATE       0x32   // Mic ADC rate

// AC97 Bus Master registers
#define AC97_PCM_OUT_BDBAR  0x10   // PCM out buffer descriptor base
#define AC97_PCM_IN_BDBAR   0x00   // PCM in (capture) buffer descriptor base
#define AC97_PCM_OUT_CIV    0x14   // Current index value
#define AC97_PCM_IN_CIV     0x04
#define AC97_PCM_OUT_LVI    0x15   // Last valid index
#define AC97_PCM_IN_LVI     0x05
#define AC97_PCM_OUT_SR     0x16   // Status register
#define AC97_PCM_IN_SR      0x06
#define AC97_PCM_OUT_CR     0x1B   // Control register
#define AC97_PCM_IN_CR      0x0B
#define AC97_GLOB_CNT       0x2C   // Global control
#define AC97_GLOB_STS       0x30   // Global status

// Control register bits
#define AC97_CR_RUN         0x01   // DMA run
#define AC97_CR_RESET       0x02   // Reset
#define AC97_CR_LVBIE       0x04   // Last valid buffer interrupt enable
#define AC97_CR_FEIE        0x08   // FIFO error interrupt enable
#define AC97_CR_IOCE        0x10   // Interrupt on completion enable

// ── DMA Buffer Descriptor ─────────────────────────────────────────────────────
typedef struct {
    uint32_t addr;      // Physical address of buffer
    uint16_t samples;   // Number of samples
    uint16_t flags;     // BUP (buffer underrun policy) | IOC (interrupt on completion)
} __attribute__((packed)) ac97_bd_t;

#define AC97_BD_IOC     0x8000
#define AC97_BD_BUP     0x4000
#define AC97_BD_COUNT   32

// ── Driver state ──────────────────────────────────────────────────────────────
static sound_state_t g_sound;
static uint16_t g_mixer_base = 0;
static uint16_t g_bus_base   = 0;

// Capture buffer — 64KB ring buffer for microphone input
#define CAPTURE_BUF_SIZE  (64 * 1024)
static uint8_t  g_capture_buf[CAPTURE_BUF_SIZE];
static uint32_t g_capture_write = 0;
static uint32_t g_capture_read  = 0;

// Playback buffer
#define PLAYBACK_BUF_SIZE (64 * 1024)
static uint8_t  g_playback_buf[PLAYBACK_BUF_SIZE];

// Voice recognition ring buffer (16-bit PCM, 16kHz, mono)
#define VOICE_BUF_SAMPLES 16000   // 1 second of audio
static int16_t  g_voice_buf[VOICE_BUF_SAMPLES];
static uint32_t g_voice_pos = 0;
static bool     g_voice_active = false;

// Simple speech energy threshold for VAD (Voice Activity Detection)
#define VAD_THRESHOLD   800
#define VAD_SILENCE_MS  500

// ── Port I/O helpers ──────────────────────────────────────────────────────────
static inline void mixer_write16(uint16_t reg, uint16_t val) {
    port_outw(g_mixer_base + reg, val);
}
static inline uint16_t mixer_read16(uint16_t reg) {
    return port_inw(g_mixer_base + reg);
}
static inline void bus_write8(uint16_t reg, uint8_t val) {
    port_outb(g_bus_base + reg, val);
}
static inline void bus_write32(uint16_t reg, uint32_t val) {
    port_outd(g_bus_base + reg, val);
}
static inline uint16_t bus_read16(uint16_t reg) {
    return port_inw(g_bus_base + reg);
}

// ── PCI scan for AC97 ─────────────────────────────────────────────────────────
// Vendor/Device IDs for common AC97 controllers
#define PCI_VENDOR_INTEL    0x8086
#define PCI_DEV_ICH_AC97    0x2415   // Intel ICH AC97
#define PCI_DEV_ICH2_AC97   0x2425
#define PCI_DEV_ICH3_AC97   0x2445
#define PCI_DEV_ICH4_AC97   0x24C5
#define PCI_DEV_ICH5_AC97   0x24D5
#define PCI_VENDOR_REALTEK  0x10EC
#define PCI_DEV_ALC_AC97    0x0101

static uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
                    (reg & 0xFC);
    port_outd(0xCF8, addr);
    return port_ind(0xCFC);
}

static bool sound_find_ac97(void) {
    // Scan PCI bus 0 for AC97 audio controller
    for (uint8_t dev = 0; dev < 32; dev++) {
        uint32_t id = pci_read(0, dev, 0, 0);
        uint16_t vendor = (uint16_t)(id & 0xFFFF);
        uint16_t device = (uint16_t)(id >> 16);

        bool found = false;
        if (vendor == PCI_VENDOR_INTEL &&
            (device == PCI_DEV_ICH_AC97  || device == PCI_DEV_ICH2_AC97 ||
             device == PCI_DEV_ICH3_AC97 || device == PCI_DEV_ICH4_AC97 ||
             device == PCI_DEV_ICH5_AC97)) {
            found = true;
        }
        if (vendor == PCI_VENDOR_REALTEK && device == PCI_DEV_ALC_AC97) {
            found = true;
        }

        if (found) {
            // BAR0 = mixer base (I/O), BAR1 = bus master base (I/O)
            uint32_t bar0 = pci_read(0, dev, 0, 0x10) & ~0x3;
            uint32_t bar1 = pci_read(0, dev, 0, 0x14) & ~0x3;
            g_mixer_base = (uint16_t)bar0;
            g_bus_base   = (uint16_t)bar1;
            terminal_write("[SOUND] AC97 found — mixer=0x");
            // Print hex
            char hex[5]; uint16_t v = g_mixer_base;
            for (int i=3;i>=0;i--){hex[i]="0123456789ABCDEF"[v&0xF];v>>=4;}
            hex[4]=0; terminal_write(hex);
            terminal_write(" bus=0x");
            v = g_bus_base;
            for (int i=3;i>=0;i--){hex[i]="0123456789ABCDEF"[v&0xF];v>>=4;}
            terminal_write(hex); terminal_write("\n");
            return true;
        }
    }
    return false;
}

// ── Initialize sound driver ───────────────────────────────────────────────────
bool sound_init(void) {
    terminal_write("[SOUND] Initializing audio driver...\n");

    g_sound.initialized  = false;
    g_sound.capture_active = false;
    g_sound.playback_active = false;
    g_sound.sample_rate  = 16000;
    g_sound.channels     = 1;
    g_sound.bits         = 16;

    // Try to find AC97 on PCI bus
    if (!sound_find_ac97()) {
        // QEMU default: try well-known ports
        g_mixer_base = 0xE100;
        g_bus_base   = 0xE200;
        terminal_write("[SOUND] AC97 not found on PCI — using QEMU defaults\n");
    }

    // Cold reset
    mixer_write16(AC97_RESET, 0x0000);
    // Busy-wait for codec ready
    for (volatile int i = 0; i < 100000; i++) {}

    // Set master volume (0 = max, 0x8000 = mute)
    mixer_write16(AC97_MASTER_VOL, 0x0000);
    mixer_write16(AC97_PCM_OUT_VOL, 0x0808);

    // Set mic volume — max gain for Landon
    mixer_write16(AC97_MIC_VOL, 0x0000);
    mixer_write16(AC97_REC_SELECT, 0x0000);  // Select mic as record source
    mixer_write16(AC97_REC_GAIN, 0x0F0F);    // Max record gain

    // Set sample rate to 16kHz for voice recognition
    mixer_write16(AC97_SAMPLE_RATE, 16000);
    mixer_write16(AC97_MIC_RATE,    16000);

    // Enable global: AC97 warm reset + variable rate audio
    bus_write32(AC97_GLOB_CNT, 0x00000002);
    for (volatile int i = 0; i < 50000; i++) {}

    g_sound.initialized = true;
    terminal_write("[SOUND] Audio driver initialized — 16kHz mono 16-bit\n");
    terminal_write("[SOUND] Microphone ready — BUBO is listening for Landon\n");
    return true;
}

// ── Start microphone capture ──────────────────────────────────────────────────
void sound_start_capture(void) {
    if (!g_sound.initialized) return;

    // Reset capture channel
    bus_write8(AC97_PCM_IN_CR, AC97_CR_RESET);
    for (volatile int i = 0; i < 10000; i++) {}

    // Set buffer descriptor base address
    bus_write32(AC97_PCM_IN_BDBAR, (uint32_t)(uint64_t)g_capture_buf);

    // Set last valid index
    bus_write8(AC97_PCM_IN_LVI, AC97_BD_COUNT - 1);

    // Start DMA
    bus_write8(AC97_PCM_IN_CR, AC97_CR_RUN | AC97_CR_IOCE);

    g_sound.capture_active = true;
    terminal_write("[SOUND] Microphone capture started\n");
}

// ── Stop microphone capture ───────────────────────────────────────────────────
void sound_stop_capture(void) {
    if (!g_sound.initialized) return;
    bus_write8(AC97_PCM_IN_CR, 0);
    g_sound.capture_active = false;
}

// ── Read captured audio samples ───────────────────────────────────────────────
uint32_t sound_read_capture(int16_t* buf, uint32_t max_samples) {
    if (!g_sound.capture_active) return 0;

    // Read from capture ring buffer
    uint32_t available = (g_capture_write - g_capture_read) / 2;
    if (available > max_samples) available = max_samples;

    for (uint32_t i = 0; i < available; i++) {
        uint32_t idx = (g_capture_read + i * 2) % CAPTURE_BUF_SIZE;
        buf[i] = (int16_t)(g_capture_buf[idx] | (g_capture_buf[idx+1] << 8));
    }
    g_capture_read = (g_capture_read + available * 2) % CAPTURE_BUF_SIZE;
    return available;
}

// ── Simple beep via PC speaker (always available) ────────────────────────────
void sound_beep(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0) {
        // Stop beep
        uint8_t tmp = port_inb(0x61);
        port_outb(0x61, tmp & ~0x03);
        return;
    }

    // PIT channel 2 for PC speaker
    uint32_t divisor = 1193180 / freq_hz;
    port_outb(0x43, 0xB6);
    port_outb(0x42, (uint8_t)(divisor & 0xFF));
    port_outb(0x42, (uint8_t)(divisor >> 8));

    // Enable speaker
    uint8_t tmp = port_inb(0x61);
    port_outb(0x61, tmp | 0x03);

    // Duration
    for (volatile uint32_t i = 0; i < duration_ms * 5000; i++) {}

    // Stop
    tmp = port_inb(0x61);
    port_outb(0x61, tmp & ~0x03);
}

// ── CORVUS audio feedback tones ───────────────────────────────────────────────
void sound_corvus_ack(void) {
    // Two-tone acknowledgment: CORVUS heard the command
    sound_beep(880, 80);
    sound_beep(1320, 80);
}

void sound_corvus_ready(void) {
    // Three-tone ready signal: BUBO is listening
    sound_beep(440, 100);
    sound_beep(660, 100);
    sound_beep(880, 150);
}

void sound_corvus_error(void) {
    // Low tone: command not understood
    sound_beep(220, 300);
}

void sound_nitro_activate(void) {
    // Rising sweep: NITRO activated — 1400HP unleashed
    for (uint32_t f = 400; f <= 2000; f += 100) {
        sound_beep(f, 20);
    }
}

// ── Get driver state ──────────────────────────────────────────────────────────
sound_state_t* sound_get_state(void) {
    return &g_sound;
}
