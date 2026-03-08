/*
 * BUBO OS — Vera Workflow Implementation
 * kernel/vera_workflow.c
 *
 * Vera is the complete workflow pipeline.
 * She receives raw events from all six input faces,
 * resolves them into semantic intents,
 * routes them to the right agent,
 * and tracks them through to completion.
 *
 * She does not care which face fired.
 * She only cares what was meant.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#include "../include/vera_workflow.h"
#include "../include/bubo_input_map.h"

/* =============================================================================
 * Internal state
 * ============================================================================= */

static vera_state_t vera;

/* =============================================================================
 * Helpers
 * ============================================================================= */

static void vera_zero(void *ptr, uint32_t size) {
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++) p[i] = 0;
}

static vera_intent_t *vera_alloc_intent(void) {
    uint32_t next = (vera.queue_head + 1) % VERA_INTENT_QUEUE_SIZE;
    if (next == vera.queue_tail) {
        /* Queue full — drop oldest */
        vera.queue_tail = (vera.queue_tail + 1) % VERA_INTENT_QUEUE_SIZE;
    }
    vera_intent_t *slot = &vera.queue[vera.queue_head];
    vera_zero(slot, sizeof(vera_intent_t));
    slot->id = vera.next_intent_id++;
    vera.queue_head = next;
    return slot;
}

static void vera_push_to_history(const vera_intent_t *intent) {
    vera.history[vera.history_head] = *intent;
    vera.history_head = (vera.history_head + 1) % VERA_HISTORY_SIZE;
}

/* =============================================================================
 * Initialization
 * ============================================================================= */

void vera_init(void) {
    vera_zero(&vera, sizeof(vera_state_t));
    vera.next_intent_id = 1;
    vera.initialized    = 1;
}

/* =============================================================================
 * Face / Rinnegan boot sync
 *
 * Each input face calls vera_face_online() when it finishes initializing.
 * This lights the corresponding tomoe in the boot animation.
 * When all six are lit, vera.faces.all_faces_ready = 1.
 * ============================================================================= */

void vera_face_online(uint8_t tomoe_index) {
    if (tomoe_index >= RINNEGAN_TOMOE_COUNT) return;
    if (vera.faces.tomoe_active[tomoe_index]) return; /* Already lit */

    vera.faces.tomoe_active[tomoe_index]    = 1;
    vera.faces.face_ready_time_ms[tomoe_index] = 0; /* Filled by boot renderer */

    /* Check if all faces are now ready */
    uint8_t all = 1;
    for (int i = 0; i < RINNEGAN_TOMOE_COUNT; i++) {
        if (!vera.faces.tomoe_active[i]) { all = 0; break; }
    }
    if (all && !vera.faces.all_faces_ready) {
        vera.faces.all_faces_ready = 1;
    }
}

int vera_all_faces_ready(void) {
    return vera.faces.all_faces_ready;
}

const vera_face_state_t *vera_get_face_state(void) {
    return &vera.faces;
}

/* =============================================================================
 * Intent resolution — keyboard events
 * ============================================================================= */

void vera_submit_keyboard_event(uint8_t scancode, uint8_t pressed,
                                 uint8_t shift, uint8_t ctrl, uint8_t alt,
                                 uint8_t shortcut_code) {
    if (!vera.initialized || vera.paused) return;
    if (!pressed) return; /* Only act on key-down for now */

    vera_intent_t *intent = vera_alloc_intent();
    intent->channel     = INPUT_CHANNEL_KEYBOARD;
    intent->is_landon   = 0; /* Keyboard is not Landon's primary channel */
    intent->confidence  = 1.0f;
    intent->stage       = VERA_STAGE_RAW;

    /* Resolve shortcut to intent code */
    if (shortcut_code != BUBO_SHORTCUT_NONE) {
        switch (shortcut_code) {
            case BUBO_SHORTCUT_SUMMON:        intent->intent_code = INTENT_SUMMON_BUBO;     break;
            case BUBO_SHORTCUT_VOICE:         intent->intent_code = INTENT_VOICE_ON;        break;
            case BUBO_SHORTCUT_GAZE:          intent->intent_code = INTENT_GAZE_ON;         break;
            case BUBO_SHORTCUT_DASHBOARD:     intent->intent_code = INTENT_DASHBOARD;       break;
            case BUBO_SHORTCUT_GAMING:        intent->intent_code = INTENT_GAMING;          break;
            case BUBO_SHORTCUT_ACCESSIBILITY: intent->intent_code = INTENT_ACCESSIBILITY;   break;
            case BUBO_SHORTCUT_SCREENSHOT:    intent->intent_code = INTENT_SCREENSHOT;      break;
            case BUBO_SHORTCUT_CLOSE:         intent->intent_code = INTENT_CLOSE;           break;
            case BUBO_SHORTCUT_FULLSCREEN:    intent->intent_code = INTENT_FULLSCREEN;      break;
            case BUBO_SHORTCUT_VOLUME_UP:     intent->intent_code = INTENT_VOLUME_UP;       break;
            case BUBO_SHORTCUT_VOLUME_DOWN:   intent->intent_code = INTENT_VOLUME_DOWN;     break;
            case BUBO_SHORTCUT_MUTE:          intent->intent_code = INTENT_MUTE;            break;
            default:                          intent->intent_code = INTENT_NONE;            break;
        }
    } else {
        /* Regular key — navigation keys become navigation intents */
        switch (scancode) {
            case KEY_NAV_UP:    intent->intent_code = INTENT_NAVIGATE_UP;    break;
            case KEY_NAV_DOWN:  intent->intent_code = INTENT_NAVIGATE_DOWN;  break;
            case KEY_NAV_LEFT:  intent->intent_code = INTENT_NAVIGATE_LEFT;  break;
            case KEY_NAV_RIGHT: intent->intent_code = INTENT_NAVIGATE_RIGHT; break;
            case KEY_ENTER:     intent->intent_code = INTENT_CONFIRM;        break;
            case KEY_ESCAPE:    intent->intent_code = INTENT_CANCEL;         break;
            case KEY_SPACE:     intent->intent_code = INTENT_SELECT;         break;
            default:            intent->intent_code = INTENT_NONE;           break;
        }
        (void)shift; (void)ctrl; (void)alt; /* Used by text input layer, not intent layer */
    }

    intent->stage = VERA_STAGE_RESOLVED;
}

/* =============================================================================
 * Intent resolution — controller events
 * ============================================================================= */

void vera_submit_controller_event(uint8_t port, uint8_t is_landon,
                                   uint8_t event_type, uint16_t button,
                                   int16_t axis_x, int16_t axis_y) {
    if (!vera.initialized || vera.paused) return;
    if (event_type != 0 /* CTRL_EVENT_BUTTON_DOWN */) return;

    vera_intent_t *intent = vera_alloc_intent();
    intent->channel    = INPUT_CHANNEL_CONTROLLER;
    intent->port       = port;
    intent->is_landon  = is_landon;
    intent->confidence = 1.0f;
    intent->x          = axis_x;
    intent->y          = axis_y;
    intent->stage      = VERA_STAGE_RAW;

    /* Map controller buttons to intents */
    switch (button) {
        case CTRL_NAV_UP:       intent->intent_code = INTENT_NAVIGATE_UP;    break;
        case CTRL_NAV_DOWN:     intent->intent_code = INTENT_NAVIGATE_DOWN;  break;
        case CTRL_NAV_LEFT:     intent->intent_code = INTENT_NAVIGATE_LEFT;  break;
        case CTRL_NAV_RIGHT:    intent->intent_code = INTENT_NAVIGATE_RIGHT; break;
        case CTRL_A:            intent->intent_code = INTENT_CONFIRM;        break;
        case CTRL_B:            intent->intent_code = INTENT_CANCEL;         break;
        case CTRL_X:            intent->intent_code = INTENT_SELECT;         break;
        case CTRL_HOME:         intent->intent_code = INTENT_SUMMON_BUBO;    break;
        case CTRL_MENU:         intent->intent_code = INTENT_DASHBOARD;      break;
        default:                intent->intent_code = INTENT_NONE;           break;
    }

    intent->stage = VERA_STAGE_RESOLVED;
}

/* =============================================================================
 * Intent resolution — voice events (already resolved by dysarthria engine)
 * ============================================================================= */

void vera_submit_voice_intent(uint8_t intent_code, float confidence,
                               const char *transcript) {
    if (!vera.initialized || vera.paused) return;
    if (confidence < 0.55f) return; /* Below minimum confidence — discard */

    vera_intent_t *intent = vera_alloc_intent();
    intent->channel      = INPUT_CHANNEL_VOICE;
    intent->is_landon    = 1; /* Voice is Landon's primary channel */
    intent->intent_code  = intent_code;
    intent->confidence   = confidence;
    intent->stage        = VERA_STAGE_RESOLVED;

    /* Copy transcript */
    if (transcript) {
        int i = 0;
        while (transcript[i] && i < 63) {
            intent->text[i] = transcript[i];
            i++;
        }
        intent->text[i] = 0;
    }
}

/* =============================================================================
 * Intent resolution — gaze events (Rinnegan eye tracking)
 * ============================================================================= */

void vera_submit_gaze_event(int16_t gaze_x, int16_t gaze_y,
                             uint8_t blink_detected, uint32_t dwell_ms) {
    if (!vera.initialized || vera.paused) return;

    /* Blink = drop drag if active */
    if (blink_detected && vera.drag_active) {
        vera_intent_t *drop = vera_alloc_intent();
        drop->channel     = INPUT_CHANNEL_GAZE;
        drop->is_landon   = 1;
        drop->intent_code = INTENT_DRAG_END;
        drop->x           = gaze_x;
        drop->y           = gaze_y;
        drop->confidence  = 1.0f;
        drop->stage       = VERA_STAGE_RESOLVED;
        vera.drag_active  = 0;
        return;
    }

    /* Dwell-drag: hold gaze 300ms on a target to start dragging */
    if (dwell_ms >= GAZE_DWELL_DRAG_MS && !vera.drag_active) {
        vera_intent_t *drag = vera_alloc_intent();
        drag->channel     = INPUT_CHANNEL_GAZE;
        drag->is_landon   = 1;
        drag->intent_code = INTENT_DRAG_START;
        drag->x           = gaze_x;
        drag->y           = gaze_y;
        drag->confidence  = 1.0f;
        drag->stage       = VERA_STAGE_RESOLVED;
        vera.drag_active  = 1;
        vera.drag_origin_x = gaze_x;
        vera.drag_origin_y = gaze_y;
        return;
    }

    /* Dwell-select: hold gaze 500ms to select */
    if (dwell_ms >= GAZE_DWELL_SELECT_MS && !vera.drag_active) {
        vera_intent_t *sel = vera_alloc_intent();
        sel->channel     = INPUT_CHANNEL_GAZE;
        sel->is_landon   = 1;
        sel->intent_code = INTENT_SELECT;
        sel->x           = gaze_x;
        sel->y           = gaze_y;
        sel->confidence  = 1.0f;
        sel->stage       = VERA_STAGE_RESOLVED;
        return;
    }

    /* Gaze movement — update position, no intent unless threshold crossed */
    (void)gaze_x; (void)gaze_y; /* Position tracked by gaze renderer directly */
}

/* =============================================================================
 * Intent resolution — gesture events (MediaPipe)
 * ============================================================================= */

void vera_submit_gesture_event(uint8_t intent_code, float confidence,
                                int16_t x, int16_t y) {
    if (!vera.initialized || vera.paused) return;
    if (confidence < GESTURE_CONFIDENCE_MIN) return;

    vera_intent_t *intent = vera_alloc_intent();
    intent->channel     = INPUT_CHANNEL_GESTURE;
    intent->is_landon   = 1;
    intent->intent_code = intent_code;
    intent->confidence  = confidence;
    intent->x           = x;
    intent->y           = y;
    intent->stage       = VERA_STAGE_RESOLVED;
}

/* =============================================================================
 * Vera tick — process queue, route intents, timeout stale ones
 * Call once per kernel tick (every ~10ms)
 * ============================================================================= */

void vera_tick(uint32_t now_ms) {
    if (!vera.initialized) return;

    uint32_t i = vera.queue_tail;
    while (i != vera.queue_head) {
        vera_intent_t *intent = &vera.queue[i];

        /* Timeout check */
        if (intent->timeout_ms > 0 && now_ms > intent->timeout_ms) {
            intent->stage = VERA_STAGE_FAILED;
            vera_push_to_history(intent);
            /* Remove from queue */
            vera.queue_tail = (vera.queue_tail + 1) % VERA_INTENT_QUEUE_SIZE;
            i = vera.queue_tail;
            continue;
        }

        /* Route resolved intents to agents */
        if (intent->stage == VERA_STAGE_RESOLVED) {
            /*
             * Routing table:
             *   INTENT_SUMMON_BUBO     → BUBO companion agent
             *   INTENT_GAMING          → hypervisor / VM agent
             *   INTENT_ACCESSIBILITY   → accessibility panel
             *   INTENT_NAVIGATE_*      → focused window / UI
             *   INTENT_DRAG_*          → window manager
             *   All others             → BUBO companion for dispatch
             *
             * Agent IDs are defined in corvus_bubo.h
             * For now we set agent_id = 0 (BUBO companion handles all)
             */
            intent->agent_id = 0;
            intent->stage    = VERA_STAGE_ROUTED;
            intent->updated_ms = now_ms;

            /* Set default timeout */
            if (intent->timeout_ms == 0) {
                intent->timeout_ms = now_ms + VERA_INTENT_TIMEOUT_MS;
            }
        }

        i = (i + 1) % VERA_INTENT_QUEUE_SIZE;
    }
}

/* =============================================================================
 * Poll for next routed intent
 * ============================================================================= */

int vera_poll_intent(vera_intent_t *intent_out) {
    uint32_t i = vera.queue_tail;
    while (i != vera.queue_head) {
        vera_intent_t *intent = &vera.queue[i];
        if (intent->stage == VERA_STAGE_ROUTED) {
            *intent_out = *intent;
            intent->stage = VERA_STAGE_EXECUTING;
            return 1;
        }
        i = (i + 1) % VERA_INTENT_QUEUE_SIZE;
    }
    return 0;
}

/* =============================================================================
 * Complete / fail intents
 * ============================================================================= */

void vera_complete_intent(uint32_t intent_id) {
    for (uint32_t i = vera.queue_tail; i != vera.queue_head;
         i = (i + 1) % VERA_INTENT_QUEUE_SIZE) {
        if (vera.queue[i].id == intent_id) {
            vera.queue[i].stage = VERA_STAGE_COMPLETE;
            vera_push_to_history(&vera.queue[i]);
            return;
        }
    }
}

void vera_fail_intent(uint32_t intent_id) {
    for (uint32_t i = vera.queue_tail; i != vera.queue_head;
         i = (i + 1) % VERA_INTENT_QUEUE_SIZE) {
        if (vera.queue[i].id == intent_id) {
            vera.queue[i].stage = VERA_STAGE_FAILED;
            vera_push_to_history(&vera.queue[i]);
            return;
        }
    }
}

/* =============================================================================
 * Pause / resume (e.g. when entering Windows VM for gaming)
 * ============================================================================= */

void vera_pause(void)  { vera.paused = 1; }
void vera_resume(void) { vera.paused = 0; }

const vera_state_t *vera_get_state(void) { return &vera; }

/* =============================================================================
 * vera_notify_face_ready — called by desktop/shell when a context comes online
 * ============================================================================= */
void vera_notify_face_ready(int face_id)
{
    if (face_id >= 0 && face_id < RINNEGAN_TOMOE_COUNT) {
        vera_face_online((uint8_t)face_id);
    }
}

/* =============================================================================
 * vera_submit_keyboard_char — submit a single Unicode character from the desktop
 * ============================================================================= */
void vera_submit_keyboard_char(uint32_t key)
{
    vera_submit_keyboard_event((uint8_t)(key & 0xFF), 1, 0, 0, 0, 0);
}

/* vera_notify_face_ready — called by desktop/shell when a context comes online */
void vera_notify_face_ready(int face_id)
{
    if (face_id >= 0 && face_id < RINNEGAN_TOMOE_COUNT) {
        vera_face_online((uint8_t)face_id);
    }
}

/* vera_submit_keyboard_char — submit a single Unicode character from the desktop */
void vera_submit_keyboard_char(uint32_t key)
{
    vera_submit_keyboard_event((uint8_t)(key & 0xFF), 1, 0, 0, 0, 0);
}
