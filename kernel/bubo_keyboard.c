/*
 * BUBO OS — Keyboard Driver Implementation
 * kernel/bubo_keyboard.c
 *
 * PS/2 keyboard driver with BUBO key remapping.
 * Home key = BUBO key. No Ctrl+Alt+Del. Just presence.
 *
 * For people who cannot use a keyboard at all, this driver
 * is completely transparent — they never need to touch it.
 * Voice, eyes, and gestures bypass this entirely and flow
 * directly into the unified input arbiter.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#include "../include/bubo_keyboard.h"

/* =========================================================================
 * Internal state
 * ========================================================================= */

static bubo_keyboard_state_t kb_state;

/* Circular event buffer */
#define KB_EVENT_BUFFER_SIZE    64
static bubo_key_event_t kb_event_buffer[KB_EVENT_BUFFER_SIZE];
static volatile uint32_t kb_buf_head = 0;
static volatile uint32_t kb_buf_tail = 0;

/* =========================================================================
 * Scancode → ASCII tables (US QWERTY, Set 1)
 * ========================================================================= */

static const char scancode_to_ascii_lower[128] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0,
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,    '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,  '-',  0,   0,   0,  '+',  0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static const char scancode_to_ascii_upper[128] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  0,
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',  '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,    '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,  '-',  0,   0,   0,  '+',  0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

/* =========================================================================
 * Low-level I/O helpers
 * ========================================================================= */

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    outb(0x80, 0x00);
}

/* =========================================================================
 * Event buffer helpers
 * ========================================================================= */

static int kb_buffer_push(const bubo_key_event_t *event) {
    uint32_t next = (kb_buf_head + 1) % KB_EVENT_BUFFER_SIZE;
    if (next == kb_buf_tail) {
        /* Buffer full — drop oldest event */
        kb_buf_tail = (kb_buf_tail + 1) % KB_EVENT_BUFFER_SIZE;
    }
    kb_event_buffer[kb_buf_head] = *event;
    kb_buf_head = next;
    return 1;
}

static int kb_buffer_pop(bubo_key_event_t *event_out) {
    if (kb_buf_head == kb_buf_tail) return 0; /* Empty */
    *event_out = kb_event_buffer[kb_buf_tail];
    kb_buf_tail = (kb_buf_tail + 1) % KB_EVENT_BUFFER_SIZE;
    return 1;
}

/* =========================================================================
 * Scancode → ASCII
 * ========================================================================= */

char bubo_keyboard_scancode_to_ascii(uint8_t scancode, uint8_t shift, uint8_t caps) {
    if (scancode >= 128) return 0;
    uint8_t use_upper = shift ^ caps; /* XOR: shift inverts caps */
    if (use_upper) {
        return scancode_to_ascii_upper[scancode];
    }
    return scancode_to_ascii_lower[scancode];
}

/* =========================================================================
 * BUBO Shortcut Detection
 * Home key alone = summon BUBO
 * Home + letter  = specific shortcut
 * ========================================================================= */

bubo_shortcut_t bubo_keyboard_detect_shortcut(uint8_t scancode,
                                               const bubo_keyboard_state_t *state) {
    if (!state->bubo_key_held) return BUBO_SHORTCUT_NONE;

    /* Home alone (no other key) — handled on release in IRQ handler */
    if (scancode == KEY_HOME) return BUBO_SHORTCUT_SUMMON;

    /* Home + letter shortcuts */
    char c = bubo_keyboard_scancode_to_ascii(scancode, 0, 0);
    switch (c) {
        case 'v': return BUBO_SHORTCUT_VOICE;
        case 'g': return BUBO_SHORTCUT_GAZE;
        case 'd': return BUBO_SHORTCUT_DASHBOARD;
        case 'x': return BUBO_SHORTCUT_GAMING;
        case 'a': return BUBO_SHORTCUT_ACCESSIBILITY;
        case 's': return BUBO_SHORTCUT_SCREENSHOT;
        case 'w': return BUBO_SHORTCUT_CLOSE;
        case 'f': return BUBO_SHORTCUT_FULLSCREEN;
        case 'm': return BUBO_SHORTCUT_MUTE;
        case 'r': return BUBO_SHORTCUT_REBOOT;
        case 'q': return BUBO_SHORTCUT_SHUTDOWN;
        default:  break;
    }

    /* Home + arrow keys */
    if (scancode == KEY_UP)   return BUBO_SHORTCUT_VOLUME_UP;
    if (scancode == KEY_DOWN) return BUBO_SHORTCUT_VOLUME_DOWN;

    return BUBO_SHORTCUT_NONE;
}

/* =========================================================================
 * IRQ1 Handler — called by interrupt dispatcher
 * ========================================================================= */

void bubo_keyboard_irq_handler(void) {
    if (!kb_state.enabled) {
        /* Drain the port to clear the interrupt */
        inb(KB_DATA_PORT);
        return;
    }

    uint8_t status = inb(KB_STATUS_PORT);
    if (!(status & KB_STATUS_OUTPUT)) return; /* Nothing to read */

    uint8_t scancode = inb(KB_DATA_PORT);

    /* Handle extended key prefix */
    if (scancode == KEY_EXTENDED) {
        kb_state.extended_prefix = 1;
        return;
    }

    uint8_t released = (scancode & KEY_RELEASE_FLAG) ? 1 : 0;
    uint8_t raw_code = scancode & ~KEY_RELEASE_FLAG;

    /* Build the event */
    bubo_key_event_t event = {0};
    event.scancode    = raw_code;
    event.pressed     = !released;
    event.shift       = kb_state.shift_held;
    event.ctrl        = kb_state.ctrl_held;
    event.alt         = kb_state.alt_held;
    event.shortcut    = BUBO_SHORTCUT_NONE;
    event.timestamp_ms = 0; /* Filled by kernel tick when available */

    /* Update modifier state */
    switch (raw_code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            kb_state.shift_held = !released;
            break;
        case KEY_LCTRL:
            kb_state.ctrl_held = !released;
            break;
        case KEY_LALT:
            kb_state.alt_held = !released;
            break;
        case KEY_CAPSLOCK:
            if (!released) kb_state.caps_lock ^= 1;
            break;
        case KEY_NUMLOCK:
            if (!released) kb_state.num_lock ^= 1;
            break;
    }

    /* -----------------------------------------------------------------------
     * BUBO KEY (Home) handling
     * The Home key is the BUBO key. When held and another key is pressed,
     * it forms a chord shortcut. When pressed and released alone, it summons
     * the BUBO companion.
     * ----------------------------------------------------------------------- */
    if (raw_code == KEY_HOME) {
        if (!released) {
            /* Home pressed — start tracking */
            kb_state.bubo_key_held     = 1;
            kb_state.bubo_key_consumed = 0;
        } else {
            /* Home released */
            if (!kb_state.bubo_key_consumed) {
                /* No chord was formed — solo BUBO key press */
                event.shortcut = BUBO_SHORTCUT_SUMMON;
                kb_buffer_push(&event);
            }
            kb_state.bubo_key_held     = 0;
            kb_state.bubo_key_consumed = 0;
        }
        kb_state.extended_prefix = 0;
        return;
    }

    /* -----------------------------------------------------------------------
     * Chord detection — if BUBO key is held, check for shortcut
     * ----------------------------------------------------------------------- */
    if (kb_state.bubo_key_held && !released) {
        bubo_shortcut_t shortcut = bubo_keyboard_detect_shortcut(raw_code, &kb_state);
        if (shortcut != BUBO_SHORTCUT_NONE) {
            event.shortcut = shortcut;
            kb_state.bubo_key_consumed = 1;
            kb_buffer_push(&event);
            kb_state.extended_prefix = 0;
            return;
        }
    }

    /* -----------------------------------------------------------------------
     * Regular key — resolve ASCII and push event
     * ----------------------------------------------------------------------- */
    event.ascii = bubo_keyboard_scancode_to_ascii(raw_code,
                                                   kb_state.shift_held,
                                                   kb_state.caps_lock);
    kb_buffer_push(&event);
    kb_state.extended_prefix = 0;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void bubo_keyboard_init(void) {
    /* Zero out state */
    for (uint32_t i = 0; i < sizeof(bubo_keyboard_state_t); i++) {
        ((uint8_t *)&kb_state)[i] = 0;
    }
    kb_buf_head = 0;
    kb_buf_tail = 0;
    kb_state.enabled = 1;

    /* Flush any pending data in the PS/2 buffer */
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT) {
        inb(KB_DATA_PORT);
        io_wait();
    }

    /* Enable keyboard scanning (command 0xF4) */
    /* Wait for input buffer to clear */
    while (inb(KB_STATUS_PORT) & KB_STATUS_INPUT);
    outb(KB_CMD_PORT, 0xAE);    /* Enable keyboard interface */
    io_wait();
    while (inb(KB_STATUS_PORT) & KB_STATUS_INPUT);
    outb(KB_DATA_PORT, 0xF4);   /* Enable scanning */
    io_wait();
}

void bubo_keyboard_enable(void) {
    kb_state.enabled = 1;
}

void bubo_keyboard_disable(void) {
    kb_state.enabled = 0;
}

int bubo_keyboard_poll(bubo_key_event_t *event_out) {
    return kb_buffer_pop(event_out);
}

void bubo_keyboard_read(bubo_key_event_t *event_out) {
    while (!kb_buffer_pop(event_out)) {
        /* Spin — in a real kernel this would yield to scheduler */
        __asm__ volatile ("hlt");
    }
}

const bubo_keyboard_state_t *bubo_keyboard_get_state(void) {
    return &kb_state;
}
