/*
 * BUBO OS — Keyboard Driver
 * bubo_keyboard.h
 *
 * BUBO Philosophy on keyboards:
 *   A keyboard is a tool for regular humans who want to use BUBO OS.
 *   It must work exactly as expected — no surprises — but with one
 *   important change: the BUBO key replaces Ctrl+Alt+Del.
 *
 *   Home key = BUBO key. Press it and BUBO appears.
 *   No three-finger salute. No interruption. Just presence.
 *
 *   For Landon and others who cannot use a keyboard at all,
 *   this driver is transparent — voice, eyes, and gestures
 *   flow through the same intent pipeline without needing
 *   a single keypress.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#ifndef BUBO_KEYBOARD_H
#define BUBO_KEYBOARD_H

#include <stdint.h>
#include "../include/deepflow_colors.h"

/* =========================================================================
 * PS/2 Keyboard I/O Ports
 * ========================================================================= */
#define KB_DATA_PORT        0x60
#define KB_STATUS_PORT      0x64
#define KB_CMD_PORT         0x64

#define KB_STATUS_OUTPUT    0x01    /* Output buffer full — data ready to read */
#define KB_STATUS_INPUT     0x02    /* Input buffer full — wait before writing  */

/* =========================================================================
 * Scan Code Set 1 — Key Identifiers
 * ========================================================================= */

/* Function keys */
#define KEY_F1              0x3B
#define KEY_F2              0x3C
#define KEY_F3              0x3D
#define KEY_F4              0x3E
#define KEY_F5              0x3F
#define KEY_F6              0x40
#define KEY_F7              0x41
#define KEY_F8              0x42
#define KEY_F9              0x43
#define KEY_F10             0x44
#define KEY_F11             0x57
#define KEY_F12             0x58

/* Navigation */
#define KEY_HOME            0x47    /* THE BUBO KEY — summons companion */
#define KEY_END             0x4F
#define KEY_PGUP            0x49
#define KEY_PGDN            0x51
#define KEY_INSERT          0x52
#define KEY_DELETE          0x53
#define KEY_UP              0x48
#define KEY_DOWN            0x50
#define KEY_LEFT            0x4B
#define KEY_RIGHT           0x4D

/* Modifiers */
#define KEY_LSHIFT          0x2A
#define KEY_RSHIFT          0x36
#define KEY_LCTRL           0x1D
#define KEY_LALT            0x38
#define KEY_CAPSLOCK        0x3A
#define KEY_NUMLOCK         0x45
#define KEY_SCROLLLOCK      0x46

/* Special */
#define KEY_ESCAPE          0x01
#define KEY_BACKSPACE       0x0E
#define KEY_TAB             0x0F
#define KEY_ENTER           0x1C
#define KEY_SPACE           0x39

/* Release flag — bit 7 set means key released */
#define KEY_RELEASE_FLAG    0x80

/* Extended key prefix */
#define KEY_EXTENDED        0xE0

/* =========================================================================
 * BUBO Key Bindings
 * The BUBO key is Home. All BUBO shortcuts start with Home.
 * This replaces Ctrl+Alt+Del and any other system-interrupt combos.
 * ========================================================================= */

typedef enum {
    BUBO_SHORTCUT_NONE          = 0,
    BUBO_SHORTCUT_SUMMON,           /* Home alone          — summon BUBO companion */
    BUBO_SHORTCUT_VOICE,            /* Home + V            — activate voice input  */
    BUBO_SHORTCUT_GAZE,             /* Home + G            — activate eye gaze     */
    BUBO_SHORTCUT_DASHBOARD,        /* Home + D            — show dashboard        */
    BUBO_SHORTCUT_GAMING,           /* Home + X            — launch gaming mode    */
    BUBO_SHORTCUT_ACCESSIBILITY,    /* Home + A            — accessibility panel   */
    BUBO_SHORTCUT_SCREENSHOT,       /* Home + S            — screenshot            */
    BUBO_SHORTCUT_CLOSE,            /* Home + W            — close focused window  */
    BUBO_SHORTCUT_FULLSCREEN,       /* Home + F            — toggle fullscreen     */
    BUBO_SHORTCUT_VOLUME_UP,        /* Home + Up           — volume up             */
    BUBO_SHORTCUT_VOLUME_DOWN,      /* Home + Down         — volume down           */
    BUBO_SHORTCUT_MUTE,             /* Home + M            — mute/unmute           */
    BUBO_SHORTCUT_REBOOT,           /* Home + R + confirm  — reboot (safe)         */
    BUBO_SHORTCUT_SHUTDOWN,         /* Home + Q + confirm  — shutdown (safe)       */
} bubo_shortcut_t;

/* =========================================================================
 * Keyboard State
 * ========================================================================= */

typedef struct {
    /* Modifier state */
    uint8_t shift_held;
    uint8_t ctrl_held;
    uint8_t alt_held;
    uint8_t caps_lock;
    uint8_t num_lock;

    /* BUBO key state */
    uint8_t bubo_key_held;          /* Home key is currently held */
    uint8_t bubo_key_consumed;      /* Chord was detected, suppress solo action */

    /* Extended key state */
    uint8_t extended_prefix;        /* 0xE0 received, next byte is extended */

    /* Last key pressed (for repeat) */
    uint8_t last_scancode;
    uint32_t repeat_timer;

    /* Input enabled */
    uint8_t enabled;
} bubo_keyboard_state_t;

/* =========================================================================
 * Key Event — what gets sent to the unified input arbiter
 * ========================================================================= */

typedef struct {
    uint8_t  scancode;          /* Raw PS/2 scancode                     */
    char     ascii;             /* Resolved ASCII character (0 if none)  */
    uint8_t  pressed;           /* 1 = pressed, 0 = released             */
    uint8_t  shift;             /* Shift was held                        */
    uint8_t  ctrl;              /* Ctrl was held                         */
    uint8_t  alt;               /* Alt was held                          */
    bubo_shortcut_t shortcut;   /* BUBO shortcut detected (or NONE)      */
    uint32_t timestamp_ms;      /* Kernel tick when event occurred       */
} bubo_key_event_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/* Initialize keyboard driver — install IRQ1 handler */
void bubo_keyboard_init(void);

/* Enable / disable keyboard input */
void bubo_keyboard_enable(void);
void bubo_keyboard_disable(void);

/* IRQ1 handler — called by interrupt dispatcher */
void bubo_keyboard_irq_handler(void);

/* Poll for next key event (non-blocking — returns 0 if no event) */
int bubo_keyboard_poll(bubo_key_event_t *event_out);

/* Blocking read — waits until a key event is available */
void bubo_keyboard_read(bubo_key_event_t *event_out);

/* Get current keyboard state */
const bubo_keyboard_state_t *bubo_keyboard_get_state(void);

/* Translate scancode to ASCII using current modifier state */
char bubo_keyboard_scancode_to_ascii(uint8_t scancode, uint8_t shift, uint8_t caps);

/* Detect BUBO shortcut from current key + modifier state */
bubo_shortcut_t bubo_keyboard_detect_shortcut(uint8_t scancode,
                                               const bubo_keyboard_state_t *state);

#endif /* BUBO_KEYBOARD_H */
