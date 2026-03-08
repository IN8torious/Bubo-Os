/*
 * BUBO OS — Controller Driver Implementation
 * kernel/bubo_controller.c
 *
 * 4-controller support. Port 2 (index 1) is always Landon's.
 *
 * The Many-Faced God principle applies here:
 * It does not matter which face the user presents —
 * controller, voice, eyes, hands, keyboard —
 * the same presence receives them all.
 *
 * This driver is one face. The unified input arbiter is the god behind it.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#include "../include/bubo_controller.h"

/* =========================================================================
 * Internal state — 4 controller slots
 * ========================================================================= */

static bubo_controller_t controllers[BUBO_MAX_CONTROLLERS];

/* Controller event ring buffer */
#define CTRL_EVENT_BUFFER_SIZE  128
static bubo_ctrl_event_t ctrl_event_buffer[CTRL_EVENT_BUFFER_SIZE];
static volatile uint32_t ctrl_buf_head = 0;
static volatile uint32_t ctrl_buf_tail = 0;

/* =========================================================================
 * Event buffer helpers
 * ========================================================================= */

static void ctrl_push_event(const bubo_ctrl_event_t *e) {
    uint32_t next = (ctrl_buf_head + 1) % CTRL_EVENT_BUFFER_SIZE;
    if (next == ctrl_buf_tail) {
        ctrl_buf_tail = (ctrl_buf_tail + 1) % CTRL_EVENT_BUFFER_SIZE;
    }
    ctrl_event_buffer[ctrl_buf_head] = *e;
    ctrl_buf_head = next;
}

int bubo_controller_poll_event(bubo_ctrl_event_t *event_out) {
    if (ctrl_buf_head == ctrl_buf_tail) return 0;
    *event_out = ctrl_event_buffer[ctrl_buf_tail];
    ctrl_buf_tail = (ctrl_buf_tail + 1) % CTRL_EVENT_BUFFER_SIZE;
    return 1;
}

/* =========================================================================
 * Default Landon accessibility profile
 *
 * Tuned for cerebral palsy:
 *   - Larger dead zones (reduces tremor noise)
 *   - Reduced sensitivity (prevents overshooting)
 *   - Lower trigger threshold (lighter press required)
 *   - Turbo on A button (reduces repeated press effort)
 *   - No axis inversion
 * ========================================================================= */

void bubo_controller_load_landon_profile(bubo_accessibility_profile_t *p) {
    /* Identity remap — no changes by default, user can customize */
    for (int i = 0; i < 16; i++) {
        p->remap_table[i] = (uint8_t)i;
    }

    /* Sensitivity: 0.75x (192/256) — gentler response */
    p->left_stick_sensitivity  = 192;
    p->right_stick_sensitivity = 192;

    /* Dead zones: ~20% — absorbs tremor */
    p->left_dead_zone  = 6553;
    p->right_dead_zone = 6553;

    /* Trigger threshold: very light press (15/255) */
    p->trigger_threshold = 15;

    /* Turbo on A button — auto-repeat every 120ms */
    p->turbo_mask     = BTN_A;
    p->turbo_rate_ms  = 120;

    /* Single-button mode off by default */
    p->single_button_mode    = 0;
    p->single_button_trigger = 0;
    p->single_button_count   = 0;
    p->single_button_index   = 0;

    /* No axis inversion */
    p->invert_left_y  = 0;
    p->invert_right_y = 0;
    p->invert_left_x  = 0;
    p->invert_right_x = 0;

    p->enabled = 1;
}

/* =========================================================================
 * Analog processing helpers
 * ========================================================================= */

int16_t bubo_controller_apply_dead_zone(int16_t value, uint16_t dead_zone) {
    int32_t v = value;
    if (v < 0) v = -v;
    if ((uint32_t)v < dead_zone) return 0;

    /* Rescale: map [dead_zone, 32767] → [0, 32767] */
    int32_t range = 32767 - dead_zone;
    if (range <= 0) return 0;
    int32_t scaled = ((v - dead_zone) * 32767) / range;
    if (scaled > 32767) scaled = 32767;
    return (int16_t)(value < 0 ? -scaled : scaled);
}

int16_t bubo_controller_apply_sensitivity(int16_t value, uint16_t sensitivity_fp) {
    /* sensitivity_fp is fixed-point: 256 = 1.0x */
    int32_t result = ((int32_t)value * sensitivity_fp) / 256;
    if (result >  32767) result =  32767;
    if (result < -32768) result = -32768;
    return (int16_t)result;
}

/* =========================================================================
 * Apply accessibility profile to raw analog state
 * ========================================================================= */

static void apply_profile_to_analog(bubo_controller_t *c) {
    bubo_accessibility_profile_t *p = &c->profile;
    if (!p->enabled) {
        c->analog = c->analog_raw;
        return;
    }

    /* Dead zone */
    c->analog.left_x  = bubo_controller_apply_dead_zone(c->analog_raw.left_x,  p->left_dead_zone);
    c->analog.left_y  = bubo_controller_apply_dead_zone(c->analog_raw.left_y,  p->left_dead_zone);
    c->analog.right_x = bubo_controller_apply_dead_zone(c->analog_raw.right_x, p->right_dead_zone);
    c->analog.right_y = bubo_controller_apply_dead_zone(c->analog_raw.right_y, p->right_dead_zone);

    /* Sensitivity */
    c->analog.left_x  = bubo_controller_apply_sensitivity(c->analog.left_x,  p->left_stick_sensitivity);
    c->analog.left_y  = bubo_controller_apply_sensitivity(c->analog.left_y,  p->left_stick_sensitivity);
    c->analog.right_x = bubo_controller_apply_sensitivity(c->analog.right_x, p->right_stick_sensitivity);
    c->analog.right_y = bubo_controller_apply_sensitivity(c->analog.right_y, p->right_stick_sensitivity);

    /* Axis inversion */
    if (p->invert_left_x)  c->analog.left_x  = -c->analog.left_x;
    if (p->invert_left_y)  c->analog.left_y  = -c->analog.left_y;
    if (p->invert_right_x) c->analog.right_x = -c->analog.right_x;
    if (p->invert_right_y) c->analog.right_y = -c->analog.right_y;

    /* Trigger threshold */
    c->analog.left_trigger  = (c->analog_raw.left_trigger  < p->trigger_threshold) ? 0 : c->analog_raw.left_trigger;
    c->analog.right_trigger = (c->analog_raw.right_trigger < p->trigger_threshold) ? 0 : c->analog_raw.right_trigger;
}

/* =========================================================================
 * Apply accessibility profile to raw button state
 * ========================================================================= */

static uint16_t apply_profile_to_buttons(bubo_controller_t *c, uint16_t raw_buttons) {
    bubo_accessibility_profile_t *p = &c->profile;
    if (!p->enabled) return raw_buttons;

    /* Button remapping */
    uint16_t remapped = 0;
    for (int i = 0; i < 16; i++) {
        if (raw_buttons & (1 << i)) {
            uint8_t dest = p->remap_table[i];
            if (dest != BUBO_REMAP_NONE && dest < 16) {
                remapped |= (1 << dest);
            }
        }
    }
    return remapped;
}

/* =========================================================================
 * Update turbo state for a controller
 * ========================================================================= */

static void update_turbo(bubo_controller_t *c, uint32_t now_ms) {
    bubo_accessibility_profile_t *p = &c->profile;
    if (!p->enabled || !p->turbo_mask) return;

    for (int i = 0; i < 16; i++) {
        uint16_t bit = (1 << i);
        if (!(p->turbo_mask & bit)) continue;

        if (c->buttons & bit) {
            /* Button held — check turbo timer */
            if (c->turbo_timers[i] == 0) {
                c->turbo_timers[i] = now_ms + p->turbo_rate_ms;
            } else if (now_ms >= c->turbo_timers[i]) {
                /* Fire synthetic press event */
                bubo_ctrl_event_t e = {0};
                e.type       = CTRL_EVENT_BUTTON_DOWN;
                e.port       = c->port;
                e.is_landon  = c->is_landon;
                e.button     = bit;
                e.timestamp_ms = now_ms;
                ctrl_push_event(&e);
                c->turbo_timers[i] = now_ms + p->turbo_rate_ms;
            }
        } else {
            c->turbo_timers[i] = 0;
        }
    }
}

/* =========================================================================
 * Simulate reading from USB HID report
 * In real hardware this is called by the XHCI interrupt handler
 * with the raw HID report buffer from the controller.
 *
 * For now: stub that returns zeroed state (no hardware yet).
 * When XHCI driver is integrated from ToaruOS, replace this.
 * ========================================================================= */

static void read_controller_hardware(uint8_t port,
                                      uint16_t *buttons_out,
                                      bubo_analog_t *analog_out) {
    (void)port;
    /* Stub — zero state until XHCI driver is integrated */
    *buttons_out = 0;
    analog_out->left_x        = 0;
    analog_out->left_y        = 0;
    analog_out->right_x       = 0;
    analog_out->right_y       = 0;
    analog_out->left_trigger  = 0;
    analog_out->right_trigger = 0;
}

/* =========================================================================
 * Poll a single controller and generate events
 * ========================================================================= */

static void poll_controller(bubo_controller_t *c, uint32_t now_ms) {
    if (!c->connected) return;

    /* Read raw hardware state */
    uint16_t raw_buttons;
    bubo_analog_t raw_analog;
    read_controller_hardware(c->port, &raw_buttons, &raw_analog);

    c->buttons_raw = raw_buttons;
    c->analog_raw  = raw_analog;

    /* Apply accessibility profile */
    uint16_t processed_buttons = apply_profile_to_buttons(c, raw_buttons);
    apply_profile_to_analog(c);

    /* Edge detection */
    c->buttons_just_pressed  = processed_buttons & ~c->buttons_prev;
    c->buttons_just_released = c->buttons_prev   & ~processed_buttons;
    c->buttons_prev          = c->buttons;
    c->buttons               = processed_buttons;

    /* Generate button events */
    for (int i = 0; i < 16; i++) {
        uint16_t bit = (1 << i);

        if (c->buttons_just_pressed & bit) {
            bubo_ctrl_event_t e = {0};
            e.type         = CTRL_EVENT_BUTTON_DOWN;
            e.port         = c->port;
            e.is_landon    = c->is_landon;
            e.button       = bit;
            e.timestamp_ms = now_ms;
            ctrl_push_event(&e);
        }

        if (c->buttons_just_released & bit) {
            bubo_ctrl_event_t e = {0};
            e.type         = CTRL_EVENT_BUTTON_UP;
            e.port         = c->port;
            e.is_landon    = c->is_landon;
            e.button       = bit;
            e.timestamp_ms = now_ms;
            ctrl_push_event(&e);
        }
    }

    /* Generate analog event if any axis moved significantly */
    {
        bubo_ctrl_event_t e = {0};
        e.type         = CTRL_EVENT_ANALOG_MOVE;
        e.port         = c->port;
        e.is_landon    = c->is_landon;
        e.analog       = c->analog;
        e.timestamp_ms = now_ms;
        ctrl_push_event(&e);
    }

    /* Turbo processing */
    update_turbo(c, now_ms);

    c->last_update_ms = now_ms;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void bubo_controller_init(void) {
    for (uint8_t i = 0; i < BUBO_MAX_CONTROLLERS; i++) {
        bubo_controller_t *c = &controllers[i];

        /* Zero the struct */
        for (uint32_t b = 0; b < sizeof(bubo_controller_t); b++) {
            ((uint8_t *)c)[b] = 0;
        }

        c->port      = i;
        c->connected = 0;
        c->is_landon = (i == BUBO_LANDON_CONTROLLER_PORT) ? 1 : 0;

        /* Load default profile for all controllers */
        /* Landon's port gets the accessibility profile */
        if (c->is_landon) {
            bubo_controller_load_landon_profile(&c->profile);
        } else {
            /* Standard profile — identity remap, default dead zones */
            for (int j = 0; j < 16; j++) c->profile.remap_table[j] = (uint8_t)j;
            c->profile.left_stick_sensitivity  = 256;
            c->profile.right_stick_sensitivity = 256;
            c->profile.left_dead_zone          = 4096;
            c->profile.right_dead_zone         = 4096;
            c->profile.trigger_threshold       = 30;
            c->profile.turbo_mask              = 0;
            c->profile.turbo_rate_ms           = 150;
            c->profile.single_button_mode      = 0;
            c->profile.enabled                 = 1;
        }
    }

    /* Mark all ports as connected for now (XHCI will update this) */
    /* TODO: replace with real USB enumeration from XHCI driver    */
    for (uint8_t i = 0; i < BUBO_MAX_CONTROLLERS; i++) {
        controllers[i].connected = 1;
    }

    ctrl_buf_head = 0;
    ctrl_buf_tail = 0;
}

void bubo_controller_poll_all(void) {
    /* TODO: replace 0 with actual kernel tick / timestamp */
    uint32_t now_ms = 0;
    for (uint8_t i = 0; i < BUBO_MAX_CONTROLLERS; i++) {
        poll_controller(&controllers[i], now_ms);
    }
}

const bubo_controller_t *bubo_controller_get(uint8_t port) {
    if (port >= BUBO_MAX_CONTROLLERS) return 0;
    return &controllers[port];
}

const bubo_controller_t *bubo_controller_get_landon(void) {
    return &controllers[BUBO_LANDON_CONTROLLER_PORT];
}

int bubo_controller_button_held(uint8_t port, uint16_t button_mask) {
    if (port >= BUBO_MAX_CONTROLLERS) return 0;
    return (controllers[port].buttons & button_mask) ? 1 : 0;
}

int bubo_controller_button_just_pressed(uint8_t port, uint16_t button_mask) {
    if (port >= BUBO_MAX_CONTROLLERS) return 0;
    return (controllers[port].buttons_just_pressed & button_mask) ? 1 : 0;
}

const bubo_analog_t *bubo_controller_get_analog(uint8_t port) {
    if (port >= BUBO_MAX_CONTROLLERS) return 0;
    return &controllers[port].analog;
}

void bubo_controller_save_profile(uint8_t port, const bubo_accessibility_profile_t *profile) {
    if (port >= BUBO_MAX_CONTROLLERS) return;
    controllers[port].profile = *profile;
    /* TODO: persist to ARCHIVIST / VFS when storage driver is ready */
}

void bubo_controller_load_profile(uint8_t port, bubo_accessibility_profile_t *profile) {
    if (port >= BUBO_MAX_CONTROLLERS) return;
    *profile = controllers[port].profile;
}
