/*
 * BUBO OS — Vera Workflow
 * include/vera_workflow.h
 *
 * Vera is the complete workflow pipeline.
 * She sits between the world and the agents.
 *
 * Raw input comes in from six faces:
 *   keyboard, controller, voice, gaze, gesture, internal
 *
 * Vera resolves them into a single intent stream.
 * She routes each intent to the right agent.
 * She tracks every intent through to completion.
 * She does not care which face fired. Only what was meant.
 *
 * The Rinnegan boot sequence:
 *   Each face that comes online lights one tomoe.
 *   When all six tomoe spin — Vera is alive.
 *   The eye opens. The system is ready.
 *
 * "Valar dohaeris." All must serve.
 * Vera serves the user. Always. Without exception.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Pankuch — BUBO OS Community License v1.0
 */

#ifndef VERA_WORKFLOW_H
#define VERA_WORKFLOW_H

#include <stdint.h>
#include "bubo_input_map.h"

/* =============================================================================
 * VERA PIPELINE STAGES
 * An intent travels through these stages from raw input to completed action.
 * ============================================================================= */

typedef enum {
    VERA_STAGE_RAW       = 0,   /* Raw hardware event received                 */
    VERA_STAGE_RESOLVED  = 1,   /* Resolved to a semantic intent               */
    VERA_STAGE_ROUTED    = 2,   /* Routed to the correct agent                 */
    VERA_STAGE_EXECUTING = 3,   /* Agent is executing the intent               */
    VERA_STAGE_COMPLETE  = 4,   /* Intent fulfilled                            */
    VERA_STAGE_FAILED    = 5,   /* Intent could not be fulfilled               */
    VERA_STAGE_CANCELLED = 6,   /* Intent was superseded or cancelled          */
} vera_stage_t;

/* =============================================================================
 * VERA INTENT — the unit of work Vera manages
 * Every action in BUBO OS starts as a Vera intent.
 * ============================================================================= */

typedef struct {
    uint32_t    id;             /* Unique intent ID (auto-incrementing)        */
    uint8_t     intent_code;    /* INTENT_* from bubo_input_map.h              */
    uint8_t     channel;        /* INPUT_CHANNEL_* — which face fired this     */
    uint8_t     port;           /* Controller port (0-3) if channel=CONTROLLER */
    uint8_t     is_landon;      /* 1 if this came from Landon's input channel  */

    /* Payload — depends on intent_code */
    int16_t     x;              /* Cursor X, gaze X, or axis value             */
    int16_t     y;              /* Cursor Y, gaze Y, or axis value             */
    float       confidence;     /* Voice/gesture confidence (0.0–1.0)          */
    char        text[64];       /* Voice transcript if applicable              */

    /* Pipeline state */
    vera_stage_t stage;
    uint8_t     agent_id;       /* Which agent is handling this intent         */
    uint32_t    created_ms;     /* Kernel tick when intent was created         */
    uint32_t    updated_ms;     /* Kernel tick when stage last changed         */
    uint32_t    timeout_ms;     /* Abandon intent if not complete by this tick */
} vera_intent_t;

/* =============================================================================
 * VERA FACE STATE — tracks each input channel's readiness
 * Used to drive the Rinnegan boot animation.
 * ============================================================================= */

typedef struct {
    uint8_t  tomoe_active[RINNEGAN_TOMOE_COUNT]; /* 1 = this face is online   */
    uint8_t  all_faces_ready;                    /* 1 = all six tomoe lit     */
    uint32_t face_ready_time_ms[RINNEGAN_TOMOE_COUNT]; /* When each came online*/
    uint32_t fully_ready_time_ms;                /* When all six were ready   */
} vera_face_state_t;

/* =============================================================================
 * VERA STATE — the full workflow engine state
 * ============================================================================= */

#define VERA_INTENT_QUEUE_SIZE      256     /* Max pending intents             */
#define VERA_HISTORY_SIZE           64      /* Recent completed intents        */
#define VERA_INTENT_TIMEOUT_MS      5000    /* Default timeout: 5 seconds      */

typedef struct {
    /* Intent queue */
    vera_intent_t   queue[VERA_INTENT_QUEUE_SIZE];
    uint32_t        queue_head;
    uint32_t        queue_tail;
    uint32_t        next_intent_id;

    /* Completed intent history (ring buffer) */
    vera_intent_t   history[VERA_HISTORY_SIZE];
    uint32_t        history_head;

    /* Face / boot state */
    vera_face_state_t faces;

    /* Active drag intent (gaze dwell-drag protocol) */
    uint8_t         drag_active;
    vera_intent_t   drag_intent;
    int16_t         drag_origin_x;
    int16_t         drag_origin_y;
    uint32_t        drag_dwell_start_ms;

    /* System state */
    uint8_t         initialized;
    uint8_t         paused;         /* 1 = Vera is paused (e.g. in VM/gaming) */
} vera_state_t;

/* =============================================================================
 * PUBLIC API
 * ============================================================================= */

/* Initialize Vera — call once at boot, before any input drivers */
void vera_init(void);

/* Notify Vera that an input face came online — advances Rinnegan animation */
void vera_face_online(uint8_t tomoe_index);

/* Check if all faces are ready */
int vera_all_faces_ready(void);

/* Get current face state (for boot animation renderer) */
const vera_face_state_t *vera_get_face_state(void);

/* Submit a raw event from any input channel — Vera resolves and queues it */
void vera_submit_keyboard_event(uint8_t scancode, uint8_t pressed,
                                 uint8_t shift, uint8_t ctrl, uint8_t alt,
                                 uint8_t shortcut_code);

void vera_submit_controller_event(uint8_t port, uint8_t is_landon,
                                   uint8_t event_type, uint16_t button,
                                   int16_t axis_x, int16_t axis_y);

void vera_submit_voice_intent(uint8_t intent_code, float confidence,
                               const char *transcript);

void vera_submit_gaze_event(int16_t gaze_x, int16_t gaze_y,
                             uint8_t blink_detected, uint32_t dwell_ms);

void vera_submit_gesture_event(uint8_t intent_code, float confidence,
                                int16_t x, int16_t y);

/* Process the intent queue — call once per kernel tick */
void vera_tick(uint32_t now_ms);

/* Poll for the next resolved intent (non-blocking — returns 0 if empty) */
int vera_poll_intent(vera_intent_t *intent_out);

/* Mark an intent as complete or failed */
void vera_complete_intent(uint32_t intent_id);
void vera_fail_intent(uint32_t intent_id);

/* Pause / resume Vera (e.g. when entering Windows VM for gaming) */
void vera_pause(void);
void vera_resume(void);

/* Get Vera's current state (read-only) */
const vera_state_t *vera_get_state(void);

#endif /* VERA_WORKFLOW_H */
