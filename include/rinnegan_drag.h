// BUBO OS — RINNEGAN Drag System
// Copyright (c) 2025 Nathan Brown & Manus AI. MIT License.
//
// Eye-tracked window dragging and resizing.
// The Rinnegan doesn't just select — it moves the world.
//
// Dwell-Lock-Drag-Drop protocol:
//   1. LOCK   — gaze dwells on a draggable region for 300ms → window locks to gaze
//   2. DRAG   — gaze moves → window follows in real time
//   3. DROP   — gaze leaves drop zone OR blink detected → window releases
//
// Color feedback (Akatsuki palette):
//   Amber  (#FFB300) — title bar when gaze is hovering (ready to lock)
//   Gold   (#FFD700) — window border while dragging (in flight)
//   Crimson(#CC0000) — border when over a valid drop zone (ready to drop)
//
// Resize variant:
//   Gaze on window corner → corner glows gold → 300ms dwell locks resize handle
//   Move eyes outward/inward → window grows/shrinks
//   Blink or look away → release
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── Drag state machine ────────────────────────────────────────────────────────
typedef enum {
    RDRAG_IDLE       = 0,   // No gaze interaction
    RDRAG_HOVER      = 1,   // Gaze on draggable region — amber highlight
    RDRAG_LOCKING    = 2,   // Dwell timer running — counting to lock
    RDRAG_LOCKED     = 3,   // Window locked to gaze — gold border
    RDRAG_DRAGGING   = 4,   // Gaze moving — window following
    RDRAG_DROPPING   = 5,   // Gaze on drop zone — crimson border
    RDRAG_RESIZE_HOVER   = 6,  // Gaze on resize corner
    RDRAG_RESIZE_LOCKING = 7,  // Dwell timer for resize
    RDRAG_RESIZING       = 8,  // Actively resizing
} rdrag_state_t;

// ── Drag target types ─────────────────────────────────────────────────────────
typedef enum {
    RDRAG_TARGET_NONE      = 0,
    RDRAG_TARGET_TITLEBAR  = 1,   // Drag the whole window
    RDRAG_TARGET_CORNER_TL = 2,   // Resize from top-left
    RDRAG_TARGET_CORNER_TR = 3,   // Resize from top-right
    RDRAG_TARGET_CORNER_BL = 4,   // Resize from bottom-left
    RDRAG_TARGET_CORNER_BR = 5,   // Resize from bottom-right
    RDRAG_TARGET_EDGE_T    = 6,   // Resize from top edge
    RDRAG_TARGET_EDGE_B    = 7,   // Resize from bottom edge
    RDRAG_TARGET_EDGE_L    = 8,   // Resize from left edge
    RDRAG_TARGET_EDGE_R    = 9,   // Resize from right edge
} rdrag_target_t;

// ── Window descriptor (passed in from desktop) ────────────────────────────────
typedef struct {
    int32_t  id;            // Window ID
    int32_t  x, y;         // Current position
    int32_t  w, h;         // Current size
    int32_t  min_w, min_h; // Minimum allowed size
    int32_t  max_w, max_h; // Maximum allowed size (0 = no limit)
    bool     draggable;    // Can be dragged
    bool     resizable;    // Can be resized
} rdrag_window_t;

// ── Drag context ──────────────────────────────────────────────────────────────
typedef struct {
    rdrag_state_t   state;

    // Target window
    int32_t         window_id;
    rdrag_target_t  target;

    // Gaze anchor point (where gaze was when lock started)
    int32_t         anchor_gaze_x, anchor_gaze_y;

    // Window position at lock time
    int32_t         anchor_win_x, anchor_win_y;
    int32_t         anchor_win_w, anchor_win_h;

    // Current gaze position
    int32_t         gaze_x, gaze_y;

    // Dwell timer (milliseconds)
    uint32_t        dwell_start_ms;
    uint32_t        dwell_lock_ms;    // How long to dwell before locking (default 300)
    uint32_t        dwell_drop_ms;    // How long off-target before dropping (default 200)

    // Drop zone tracking
    uint32_t        drop_start_ms;

    // Visual feedback color to apply this frame
    uint32_t        feedback_color;   // 0 = no override
    bool            show_feedback;

    // Snap-to-grid (optional)
    bool            snap_enabled;
    int32_t         snap_grid;        // Pixel grid size (e.g. 8)

    // Screen bounds
    int32_t         screen_w, screen_h;
    int32_t         taskbar_h;        // Don't drag into taskbar
    int32_t         topbar_h;         // Don't drag into top bar
} rdrag_ctx_t;

// ── Callback types ────────────────────────────────────────────────────────────
// Called when a window should move to a new position
typedef void (*rdrag_move_fn)(int32_t window_id, int32_t new_x, int32_t new_y);

// Called when a window should resize
typedef void (*rdrag_resize_fn)(int32_t window_id, int32_t new_x, int32_t new_y,
                                 int32_t new_w, int32_t new_h);

// Called when drag state changes (for visual feedback)
typedef void (*rdrag_feedback_fn)(int32_t window_id, rdrag_state_t state,
                                   uint32_t color);

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize the drag system
void rdrag_init(rdrag_ctx_t *ctx, int32_t screen_w, int32_t screen_h,
                int32_t taskbar_h, int32_t topbar_h);

// Register callbacks
void rdrag_set_callbacks(rdrag_ctx_t *ctx,
                          rdrag_move_fn   on_move,
                          rdrag_resize_fn on_resize,
                          rdrag_feedback_fn on_feedback);

// Feed a new gaze sample into the drag system
// Call this every frame from the Rinnegan pipeline
// windows[] is the current list of all windows on screen
void rdrag_update(rdrag_ctx_t *ctx,
                  int32_t gaze_x, int32_t gaze_y,
                  uint32_t now_ms,
                  bool blink_detected,
                  rdrag_window_t *windows, int32_t window_count);

// Force-cancel any active drag (e.g. voice command "cancel")
void rdrag_cancel(rdrag_ctx_t *ctx);

// Force-drop at current position (e.g. voice command "drop")
void rdrag_force_drop(rdrag_ctx_t *ctx);

// Query current state
rdrag_state_t rdrag_get_state(const rdrag_ctx_t *ctx);
int32_t       rdrag_get_active_window(const rdrag_ctx_t *ctx);

// Render drag feedback overlay (call after desktop_render)
// Draws amber/gold/crimson border on the active window
void rdrag_render_feedback(const rdrag_ctx_t *ctx,
                            rdrag_window_t *windows, int32_t window_count);

// ── Timing constants ──────────────────────────────────────────────────────────
#define RDRAG_LOCK_DWELL_MS     300   // ms to dwell before locking
#define RDRAG_DROP_DWELL_MS     200   // ms off-target before dropping
#define RDRAG_CORNER_SIZE       20    // px — corner resize hit zone
#define RDRAG_EDGE_SIZE         8     // px — edge resize hit zone
#define RDRAG_TITLEBAR_H        24    // px — title bar drag zone height
#define RDRAG_SNAP_GRID         8     // px — snap grid size

// ── Feedback colors ───────────────────────────────────────────────────────────
#define RDRAG_COLOR_HOVER       0xFFFFB300   // Amber — ready to lock
#define RDRAG_COLOR_LOCKED      0xFFFFD700   // Gold — locked, in flight
#define RDRAG_COLOR_DROP_ZONE   0xFFCC0000   // Crimson — valid drop zone
#define RDRAG_COLOR_RESIZE      0xFF9D00FF   // Purple — resize active (Rinnegan)
