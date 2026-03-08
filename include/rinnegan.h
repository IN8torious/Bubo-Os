// BUBO OS — Copyright (c) 2025 Nathan Brown & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// include/rinnegan.h — The Eye-Gaze Input System
//
// The Rinnegan. The eye that sees the world not as it is, but as it could be.
// This is the bare-metal port of the Timm & Barth gradient pupil detection
// algorithm, mapped to the Deep Flow semantic color system.
//
// Landon looks at a color. The system knows what the color means.
// The gaze becomes the input. The hands are no longer required.
// =============================================================================

#ifndef RINNEGAN_H
#define RINNEGAN_H

#include <stdint.h>
#include <stdbool.h>

// ── Screen and Gaze Types ─────────────────────────────────────────────────────

typedef struct {
    int32_t x;
    int32_t y;
} rinnegan_point_t;

typedef struct {
    rinnegan_point_t screen_pos;     // Where on the screen the user is looking
    uint32_t         color_hit;      // The semantic color at that coordinate
    uint64_t         dwell_time_ms;  // How long the gaze has rested on this color
    bool             is_selected;    // True if dwell time > threshold
} rinnegan_gaze_state_t;

// ── Configuration ─────────────────────────────────────────────────────────────

#define RINNEGAN_DWELL_THRESHOLD_MS 300  // 300ms stare = click/select
#define RINNEGAN_ROI_WIDTH          64   // Eye region of interest width
#define RINNEGAN_ROI_HEIGHT         64   // Eye region of interest height

// ── Calibration Matrix ────────────────────────────────────────────────────────
// Maps 2D pupil coordinates to 2D screen coordinates using a homography matrix
// generated during the 9-point calibration phase.
typedef struct {
    float h[9]; // 3x3 homography matrix flattened
    bool  is_calibrated;
} rinnegan_calibration_t;

// ── Core API ──────────────────────────────────────────────────────────────────

// Initialize the Rinnegan system.
void rinnegan_init(void);

// Begin the 9-point calibration sequence.
void rinnegan_start_calibration(void);

// Process a raw grayscale eye frame from the webcam.
// Uses the Timm & Barth gradient method to find the pupil center,
// maps it to screen coordinates, and checks the color at that location.
// Returns the current gaze state.
rinnegan_gaze_state_t rinnegan_process_frame(const uint8_t* eye_roi_gray, uint64_t current_tick);

// ── Math & Vision Algorithms (Internal/Exposed for testing) ───────────────────

// Timm & Barth objective function for pupil center localization
// Computes the center of the pupil given a grayscale ROI
rinnegan_point_t rinnegan_find_pupil(const uint8_t* roi_gray, int width, int height);

// Map a pupil coordinate to a screen coordinate using the calibration matrix
rinnegan_point_t rinnegan_map_to_screen(rinnegan_point_t pupil_pos, const rinnegan_calibration_t* calib);

#endif // RINNEGAN_H
