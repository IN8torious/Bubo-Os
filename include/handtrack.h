// BUBO OS — Hand Gesture Recognition
// Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
//
// Bare-metal hand gesture detection using the same webcam as the Rinnegan.
// Detects 8 core gestures from a single RGB frame with no GPU required.
// Uses a simplified MediaPipe-inspired hand landmark approach.
//
// One camera. Eyes AND hands. Voice as the third channel.
// Three independent input systems — whichever works best, leads.
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── Gesture vocabulary ────────────────────────────────────────────────────────
typedef enum {
    GESTURE_NONE         = 0,
    GESTURE_OPEN_PALM    = 1,   // Open hand, palm forward — Confirm / Select
    GESTURE_FIST         = 2,   // Closed fist — Cancel / Stop
    GESTURE_SWIPE_LEFT   = 3,   // Horizontal swipe left — Previous app
    GESTURE_SWIPE_RIGHT  = 4,   // Horizontal swipe right — Next app
    GESTURE_SWIPE_UP     = 5,   // Vertical swipe up — Open dashboard
    GESTURE_SWIPE_DOWN   = 6,   // Vertical swipe down — Minimize
    GESTURE_PEACE        = 7,   // Two fingers up — Open BUBO dashboard
    GESTURE_THUMBS_UP    = 8,   // Thumbs up — Yes / Confirm
    GESTURE_POINT        = 9,   // Index finger pointing — Move cursor
    GESTURE_GRAB         = 10,  // Partial fist, moving — Drag window
    GESTURE_PINCH        = 11,  // Thumb + index pinch — Fine selection
    GESTURE_WAVE         = 12,  // Hand wave — Wake BUBO / attention
    GESTURE_COUNT        = 13
} gesture_id_t;

// ── Hand landmark (21 points, MediaPipe-compatible) ───────────────────────────
typedef struct {
    float x, y;     // Normalized 0.0–1.0 within the hand bounding box
    float z;        // Depth estimate (positive = closer to camera)
    float conf;     // Detection confidence 0.0–1.0
} hand_landmark_t;

#define HAND_LANDMARK_COUNT 21

// Landmark indices (MediaPipe standard)
#define HL_WRIST         0
#define HL_THUMB_CMC     1
#define HL_THUMB_MCP     2
#define HL_THUMB_IP      3
#define HL_THUMB_TIP     4
#define HL_INDEX_MCP     5
#define HL_INDEX_PIP     6
#define HL_INDEX_DIP     7
#define HL_INDEX_TIP     8
#define HL_MIDDLE_MCP    9
#define HL_MIDDLE_PIP    10
#define HL_MIDDLE_DIP    11
#define HL_MIDDLE_TIP    12
#define HL_RING_MCP      13
#define HL_RING_PIP      14
#define HL_RING_DIP      15
#define HL_RING_TIP      16
#define HL_PINKY_MCP     17
#define HL_PINKY_PIP     18
#define HL_PINKY_DIP     19
#define HL_PINKY_TIP     20

// ── Hand detection result ─────────────────────────────────────────────────────
typedef struct {
    bool              detected;
    hand_landmark_t   landmarks[HAND_LANDMARK_COUNT];
    float             confidence;

    // Bounding box in screen pixels
    int32_t           bbox_x, bbox_y, bbox_w, bbox_h;

    // Palm center in screen pixels
    int32_t           palm_x, palm_y;

    // Which hand
    bool              is_right_hand;
} hand_detection_t;

// ── Gesture event ─────────────────────────────────────────────────────────────
typedef struct {
    gesture_id_t      gesture;
    float             confidence;
    uint32_t          timestamp_ms;

    // For swipe gestures: velocity in pixels/ms
    float             velocity_x, velocity_y;

    // For point gesture: screen coordinates being pointed at
    int32_t           point_x, point_y;

    // For grab/drag: delta from last frame
    int32_t           delta_x, delta_y;

    // Which hand produced this gesture
    bool              is_right_hand;
} gesture_event_t;

// ── Handtrack context ─────────────────────────────────────────────────────────
#define HANDTRACK_MAX_HANDS  2
#define HANDTRACK_HISTORY    8   // Frames of history for swipe detection

typedef struct {
    // Detected hands this frame
    hand_detection_t  hands[HANDTRACK_MAX_HANDS];
    int32_t           hand_count;

    // Current gesture per hand
    gesture_event_t   gestures[HANDTRACK_MAX_HANDS];

    // Swipe detection: position history
    int32_t           history_x[HANDTRACK_MAX_HANDS][HANDTRACK_HISTORY];
    int32_t           history_y[HANDTRACK_MAX_HANDS][HANDTRACK_HISTORY];
    int32_t           history_idx;

    // Frame dimensions
    int32_t           frame_w, frame_h;

    // Confidence thresholds
    float             detect_threshold;   // Min confidence to report detection
    float             gesture_threshold;  // Min confidence to report gesture

    // Gesture cooldown (ms between repeated gesture events)
    uint32_t          cooldown_ms;
    uint32_t          last_gesture_ms[HANDTRACK_MAX_HANDS];

    // Enabled gestures bitmask
    uint32_t          enabled_gestures;   // 1 << gesture_id_t

    // Statistics
    uint32_t          frames_processed;
    uint32_t          gestures_detected;
} handtrack_ctx_t;

// ── Callback ──────────────────────────────────────────────────────────────────
typedef void (*handtrack_gesture_fn)(const gesture_event_t *event);

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize hand tracking
void handtrack_init(handtrack_ctx_t *ctx, int32_t frame_w, int32_t frame_h);

// Register gesture callback
void handtrack_set_callback(handtrack_ctx_t *ctx, handtrack_gesture_fn on_gesture);

// Enable or disable specific gestures
void handtrack_enable_gesture(handtrack_ctx_t *ctx, gesture_id_t g, bool enable);

// Process a raw YUYV frame (same format as Rinnegan)
// Returns number of gestures detected this frame
int32_t handtrack_process_frame(handtrack_ctx_t *ctx,
                                 const uint8_t *frame_yuyv,
                                 uint32_t now_ms);

// Get the name of a gesture as a string
const char *handtrack_gesture_name(gesture_id_t g);

// Get the current cursor position from pointing gesture
bool handtrack_get_point(const handtrack_ctx_t *ctx,
                          int32_t *out_x, int32_t *out_y);

// ── Gesture constants ─────────────────────────────────────────────────────────
#define HANDTRACK_SWIPE_MIN_VELOCITY   0.5f   // pixels/ms minimum for swipe
#define HANDTRACK_SWIPE_MIN_DISTANCE   80     // pixels minimum for swipe
#define HANDTRACK_PINCH_THRESHOLD      30     // pixels max distance for pinch
#define HANDTRACK_FIST_THRESHOLD       0.3f   // max finger extension ratio for fist
#define HANDTRACK_OPEN_THRESHOLD       0.7f   // min finger extension ratio for open palm
#define HANDTRACK_COOLDOWN_MS          400    // ms between repeated gestures
