/*
 * BUBO OS — Controller Driver
 * bubo_controller.h
 *
 * 4-controller support via USB HID (XHCI) and legacy gameport.
 *
 * Controller layout:
 *   Port 1 — Nathan (standard profile)
 *   Port 2 — Landon (accessibility profile — always reserved)
 *   Port 3 — Guest player 3
 *   Port 4 — Guest player 4
 *
 * Controller 2 (Landon's) has a dedicated accessibility profile:
 *   - Button remapping (any button can do any action)
 *   - Analog sensitivity scaling (0.0 to 2.0x)
 *   - Dead zone expansion (larger center dead zone)
 *   - Turbo mode (auto-repeat for held buttons)
 *   - Single-button mode (one button does everything, sequentially)
 *   - Trigger sensitivity (lighter press threshold)
 *
 * All four controllers feed into the unified input arbiter.
 * In gaming mode, they map directly to the Windows VM via virtio-input.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#ifndef BUBO_CONTROLLER_H
#define BUBO_CONTROLLER_H

#include <stdint.h>

/* =========================================================================
 * Controller Ports
 * ========================================================================= */

#define BUBO_MAX_CONTROLLERS        4
#define BUBO_LANDON_CONTROLLER_PORT 1   /* Zero-indexed: port 2 = index 1 */

/* =========================================================================
 * Button Bitmask (XInput-compatible layout)
 * ========================================================================= */

#define BTN_DPAD_UP         (1 << 0)
#define BTN_DPAD_DOWN       (1 << 1)
#define BTN_DPAD_LEFT       (1 << 2)
#define BTN_DPAD_RIGHT      (1 << 3)
#define BTN_START           (1 << 4)
#define BTN_SELECT          (1 << 5)   /* Back/Select */
#define BTN_L3              (1 << 6)   /* Left stick click */
#define BTN_R3              (1 << 7)   /* Right stick click */
#define BTN_LB              (1 << 8)   /* Left bumper */
#define BTN_RB              (1 << 9)   /* Right bumper */
#define BTN_GUIDE           (1 << 10)  /* Home/Guide button */
#define BTN_A               (1 << 12)
#define BTN_B               (1 << 13)
#define BTN_X               (1 << 14)
#define BTN_Y               (1 << 15)

/* =========================================================================
 * Analog Axes — signed 16-bit, range -32768 to +32767
 * ========================================================================= */

typedef struct {
    int16_t left_x;     /* Left stick horizontal  */
    int16_t left_y;     /* Left stick vertical    */
    int16_t right_x;    /* Right stick horizontal */
    int16_t right_y;    /* Right stick vertical   */
    uint8_t left_trigger;   /* 0–255 */
    uint8_t right_trigger;  /* 0–255 */
} bubo_analog_t;

/* =========================================================================
 * Accessibility Profile — per-controller remapping and tuning
 * ========================================================================= */

#define BUBO_REMAP_NONE     0xFF    /* No remap — use physical button */

typedef struct {
    /* Button remapping: remap_table[physical_btn_index] = logical_btn_index */
    /* Use BUBO_REMAP_NONE to pass through unchanged                         */
    uint8_t  remap_table[16];

    /* Analog sensitivity multiplier (fixed-point: 256 = 1.0x) */
    uint16_t left_stick_sensitivity;    /* Default: 256 (1.0x) */
    uint16_t right_stick_sensitivity;   /* Default: 256 (1.0x) */

    /* Dead zone radius (0–32767). Inputs within this range read as zero */
    uint16_t left_dead_zone;            /* Default: 4096  (~12%) */
    uint16_t right_dead_zone;           /* Default: 4096  (~12%) */

    /* Trigger threshold — minimum value to register as pressed (0–255) */
    uint8_t  trigger_threshold;         /* Default: 30 */

    /* Turbo: buttons in this mask auto-repeat when held */
    uint16_t turbo_mask;                /* Default: 0 (no turbo) */
    uint16_t turbo_rate_ms;             /* Repeat interval in ms. Default: 150 */

    /* Single-button sequential mode */
    /* When enabled, one designated button cycles through a sequence of actions */
    uint8_t  single_button_mode;        /* 0 = off, 1 = on */
    uint8_t  single_button_trigger;     /* Which button triggers the cycle */
    uint16_t single_button_sequence[8]; /* Up to 8 actions in sequence */
    uint8_t  single_button_count;       /* How many actions in sequence */
    uint8_t  single_button_index;       /* Current position in sequence */

    /* Invert axes */
    uint8_t  invert_left_y;
    uint8_t  invert_right_y;
    uint8_t  invert_left_x;
    uint8_t  invert_right_x;

    /* Profile active flag */
    uint8_t  enabled;
} bubo_accessibility_profile_t;

/* =========================================================================
 * Controller State — raw + processed
 * ========================================================================= */

typedef struct {
    /* Connection */
    uint8_t  connected;
    uint8_t  port;              /* 0–3 */
    uint8_t  is_landon;         /* 1 if this is Landon's controller */

    /* Raw hardware state */
    uint16_t buttons_raw;       /* Raw button bitmask from hardware */
    bubo_analog_t analog_raw;   /* Raw analog values from hardware */

    /* Processed state (after accessibility profile applied) */
    uint16_t buttons;           /* Processed button bitmask */
    bubo_analog_t analog;       /* Processed analog values */

    /* Edge detection */
    uint16_t buttons_just_pressed;   /* Buttons that went down this frame */
    uint16_t buttons_just_released;  /* Buttons that went up this frame */
    uint16_t buttons_prev;           /* Previous frame button state */

    /* Turbo state */
    uint32_t turbo_timers[16];  /* Per-button turbo countdown */

    /* Accessibility profile */
    bubo_accessibility_profile_t profile;

    /* USB device info */
    uint8_t  usb_port;
    uint16_t vendor_id;
    uint16_t product_id;

    /* Timestamp of last update */
    uint32_t last_update_ms;
} bubo_controller_t;

/* =========================================================================
 * Controller Event — sent to unified input arbiter
 * ========================================================================= */

typedef enum {
    CTRL_EVENT_BUTTON_DOWN,
    CTRL_EVENT_BUTTON_UP,
    CTRL_EVENT_ANALOG_MOVE,
    CTRL_EVENT_CONNECTED,
    CTRL_EVENT_DISCONNECTED,
} bubo_ctrl_event_type_t;

typedef struct {
    bubo_ctrl_event_type_t type;
    uint8_t  port;              /* Controller port 0–3 */
    uint8_t  is_landon;
    uint16_t button;            /* For button events: which button */
    bubo_analog_t analog;       /* For analog events: current state */
    uint32_t timestamp_ms;
} bubo_ctrl_event_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/* Initialize all controller slots */
void bubo_controller_init(void);

/* Poll all connected controllers — call once per frame */
void bubo_controller_poll_all(void);

/* Get controller state by port (0–3) */
const bubo_controller_t *bubo_controller_get(uint8_t port);

/* Get Landon's controller directly */
const bubo_controller_t *bubo_controller_get_landon(void);

/* Check if a button is currently held on a controller */
int bubo_controller_button_held(uint8_t port, uint16_t button_mask);

/* Check if a button was just pressed this frame */
int bubo_controller_button_just_pressed(uint8_t port, uint16_t button_mask);

/* Get processed analog state for a controller */
const bubo_analog_t *bubo_controller_get_analog(uint8_t port);

/* Apply dead zone to a single axis value */
int16_t bubo_controller_apply_dead_zone(int16_t value, uint16_t dead_zone);

/* Apply sensitivity to a single axis value */
int16_t bubo_controller_apply_sensitivity(int16_t value, uint16_t sensitivity_fp);

/* Load default accessibility profile for Landon's controller */
void bubo_controller_load_landon_profile(bubo_accessibility_profile_t *profile);

/* Save/load accessibility profile to/from persistent storage */
void bubo_controller_save_profile(uint8_t port, const bubo_accessibility_profile_t *profile);
void bubo_controller_load_profile(uint8_t port, bubo_accessibility_profile_t *profile);

/* Poll for next controller event (non-blocking — returns 0 if none) */
int bubo_controller_poll_event(bubo_ctrl_event_t *event_out);

#endif /* BUBO_CONTROLLER_H */
