// BUBO OS — Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/rinnegan.c — The Eye-Gaze Input System
//
// Bare-metal implementation of the Timm & Barth pupil detection algorithm.
// No OpenCV. No dlib. No dynamic memory. Just raw math at ring 0.
//
// The Rinnegan sees the color. The color is the command.
// =============================================================================

#include "rinnegan.h"
#include "corvus_archivist.h"
#include "corvus_bubo.h"
#include "deepflow_colors.h"

// We need some basic math for the gradient calculations
// In a real bare-metal environment, these would be provided by a custom libm
static float df_sqrt(float x) {
    if (x <= 0) return 0;
    float z = x;
    for (int i = 0; i < 10; i++) {
        z -= (z*z - x) / (2*z);
    }
    return z;
}

// ── Internal State ────────────────────────────────────────────────────────────

static rinnegan_calibration_t current_calib;
static rinnegan_gaze_state_t  current_state;
static uint32_t               last_color_looked_at = 0;
static uint64_t               color_dwell_start_tick = 0;

// ── Initialization ────────────────────────────────────────────────────────────

void rinnegan_init(void) {
    current_calib.is_calibrated = false;
    
    // Default identity matrix for uncalibrated state
    current_calib.h[0] = 1.0f; current_calib.h[1] = 0.0f; current_calib.h[2] = 0.0f;
    current_calib.h[3] = 0.0f; current_calib.h[4] = 1.0f; current_calib.h[5] = 0.0f;
    current_calib.h[6] = 0.0f; current_calib.h[7] = 0.0f; current_calib.h[8] = 1.0f;

    current_state.screen_pos.x = 0;
    current_state.screen_pos.y = 0;
    current_state.color_hit = 0;
    current_state.dwell_time_ms = 0;
    current_state.is_selected = false;

    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "RINNEGAN", "initialized", false);
}

void rinnegan_start_calibration(void) {
    // In a full implementation, this triggers the UI to draw 9 points
    // and records the pupil position for each, then solves for the homography matrix.
    // For now, we simulate a successful calibration.
    current_calib.is_calibrated = true;
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, "RINNEGAN", "calibrating", false);
}

// ── The Math: Timm & Barth Pupil Detection ────────────────────────────────────
// This algorithm finds the center of a circular dark region by looking at
// the image gradients. It maximizes the dot product between the gradient vectors
// and the vectors pointing from the gradient location to the proposed center.

rinnegan_point_t rinnegan_find_pupil(const uint8_t* roi_gray, int width, int height) {
    rinnegan_point_t best_center = { width / 2, height / 2 };
    float max_objective = -1.0f;

    // Step 1: Compute gradients (sobel approximation)
    // We do this inline to save memory allocations
    
    // We only search the inner region to avoid edge effects
    for (int cy = 5; cy < height - 5; cy++) {
        for (int cx = 5; cy < width - 5; cx++) {
            
            float objective = 0.0f;
            
            // For each proposed center (cx, cy), evaluate all gradients
            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    
                    // Simple gradient
                    int idx = y * width + x;
                    float dx = (float)(roi_gray[idx + 1] - roi_gray[idx - 1]) / 2.0f;
                    float dy = (float)(roi_gray[idx + width] - roi_gray[idx - width]) / 2.0f;
                    
                    float mag = df_sqrt(dx*dx + dy*dy);
                    if (mag == 0) continue;
                    
                    // Normalize gradient
                    dx /= mag;
                    dy /= mag;
                    
                    // Vector from pixel to proposed center
                    float d_cx = (float)(cx - x);
                    float d_cy = (float)(cy - y);
                    float d_mag = df_sqrt(d_cx*d_cx + d_cy*d_cy);
                    
                    if (d_mag == 0) continue;
                    
                    d_cx /= d_mag;
                    d_cy /= d_mag;
                    
                    // Dot product
                    float dot = dx * d_cx + dy * d_cy;
                    
                    // Only positive dot products contribute (we want dark center)
                    if (dot > 0) {
                        // Weight by gradient magnitude and inverse distance
                        objective += (dot * dot) * mag;
                    }
                }
            }
            
            if (objective > max_objective) {
                max_objective = objective;
                best_center.x = cx;
                best_center.y = cy;
            }
        }
    }
    
    return best_center;
}

// ── Screen Mapping ────────────────────────────────────────────────────────────

rinnegan_point_t rinnegan_map_to_screen(rinnegan_point_t pupil_pos, const rinnegan_calibration_t* calib) {
    rinnegan_point_t screen_pos;
    
    // Apply 3x3 homography matrix
    float x = (float)pupil_pos.x;
    float y = (float)pupil_pos.y;
    
    float w = calib->h[6] * x + calib->h[7] * y + calib->h[8];
    if (w == 0) w = 1.0f; // Prevent division by zero
    
    screen_pos.x = (int)((calib->h[0] * x + calib->h[1] * y + calib->h[2]) / w);
    screen_pos.y = (int)((calib->h[3] * x + calib->h[4] * y + calib->h[5]) / w);
    
    return screen_pos;
}

// ── The Pipeline ──────────────────────────────────────────────────────────────

// Stub for reading the screen color at a coordinate.
// In the real kernel, this reads directly from the VGA/framebuffer memory.
static uint32_t read_screen_color(int x, int y) {
    (void)x; (void)y;
    // For now, simulate looking at an "Orange" solution region
    return DF_AGENT_MUGEN; // Orange-Red
}

rinnegan_gaze_state_t rinnegan_process_frame(const uint8_t* eye_roi_gray, uint64_t current_tick) {
    
    // 1. Find the pupil in the raw camera frame
    rinnegan_point_t pupil = rinnegan_find_pupil(eye_roi_gray, RINNEGAN_ROI_WIDTH, RINNEGAN_ROI_HEIGHT);
    
    // 2. Map pupil to screen coordinate
    current_state.screen_pos = rinnegan_map_to_screen(pupil, &current_calib);
    
    // 3. Read the semantic color at that coordinate
    uint32_t color_hit = read_screen_color(current_state.screen_pos.x, current_state.screen_pos.y);
    current_state.color_hit = color_hit;
    
    // 4. Dwell logic (is Landon staring at this color?)
    if (color_hit == last_color_looked_at) {
        // Calculate ms based on an assumed 18.2 Hz tick rate for bare metal
        // (1 tick ~= 55ms)
        uint64_t ticks_dwelled = current_tick - color_dwell_start_tick;
        current_state.dwell_time_ms = ticks_dwelled * 55;
        
        if (current_state.dwell_time_ms >= RINNEGAN_DWELL_THRESHOLD_MS && !current_state.is_selected) {
            current_state.is_selected = true;
            
            // Log the selection
            archivist_record(ARCHIVE_RECORD_VOICE_CMD, "rinnegan_select", "color_selected", false);
            
            // BUBO acknowledges the gaze selection if it's an action color
            if (color_hit == DF_AGENT_MUGEN || color_hit == DF_AGENT_VASH) {
                // bubo_speak("Got it.", current_tick);
            }
        }
    } else {
        // Gaze moved to a new color
        last_color_looked_at = color_hit;
        color_dwell_start_tick = current_tick;
        current_state.dwell_time_ms = 0;
        current_state.is_selected = false;
    }
    
    return current_state;
}
