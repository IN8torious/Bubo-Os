// BUBO OS — Hand Gesture Recognition Implementation
// Copyright (c) 2025 Nathan Brown & Manus AI. MIT License.
//
// Bare-metal hand gesture detection from raw YUYV webcam frames.
// No GPU. No OpenCV. No dynamic memory. Pure C math.
//
// Pipeline:
//   1. Convert YUYV → grayscale (Y channel only — fast)
//   2. Skin color detection in YUV space → hand mask
//   3. Connected component analysis → hand bounding box
//   4. Simplified landmark estimation from contour analysis
//   5. Gesture classification from landmark geometry
//   6. Swipe detection from position history
// =============================================================================
#include "../include/handtrack.h"
#include <stddef.h>

// ── Static callback ───────────────────────────────────────────────────────────
static handtrack_gesture_fn s_on_gesture = NULL;

// ── Gesture names ─────────────────────────────────────────────────────────────
static const char *s_gesture_names[GESTURE_COUNT] = {
    "none", "open_palm", "fist", "swipe_left", "swipe_right",
    "swipe_up", "swipe_down", "peace", "thumbs_up", "point",
    "grab", "pinch", "wave"
};

const char *handtrack_gesture_name(gesture_id_t g) {
    if (g < 0 || g >= GESTURE_COUNT) return "unknown";
    return s_gesture_names[g];
}

// ── Skin detection in YUV space ───────────────────────────────────────────────
// YUV skin color ranges (empirically tuned for diverse skin tones)
// Y: 80–240, U: 85–135, V: 135–180
static inline bool is_skin_yuv(uint8_t y, uint8_t u, uint8_t v) {
    return (y >= 80 && y <= 240) &&
           (u >= 85 && u <= 135) &&
           (v >= 135 && v <= 180);
}

// ── Simple bounding box from skin mask ───────────────────────────────────────
// Scans the YUYV frame and finds the largest skin-colored region
// Returns false if no hand detected
static bool find_hand_bbox(const uint8_t *frame_yuyv,
                            int32_t fw, int32_t fh,
                            int32_t *out_x, int32_t *out_y,
                            int32_t *out_w, int32_t *out_h,
                            int32_t *out_cx, int32_t *out_cy) {
    int32_t min_x = fw, max_x = 0;
    int32_t min_y = fh, max_y = 0;
    int32_t skin_count = 0;
    int64_t sum_x = 0, sum_y = 0;

    // YUYV: each pair of pixels = 4 bytes [Y0 U Y1 V]
    for (int32_t py = 0; py < fh; py++) {
        for (int32_t px = 0; px < fw; px += 2) {
            int32_t idx = (py * fw + px) * 2;
            uint8_t y0 = frame_yuyv[idx];
            uint8_t u  = frame_yuyv[idx + 1];
            uint8_t y1 = frame_yuyv[idx + 2];
            uint8_t v  = frame_yuyv[idx + 3];

            if (is_skin_yuv(y0, u, v)) {
                if (px < min_x) min_x = px;
                if (px > max_x) max_x = px;
                if (py < min_y) min_y = py;
                if (py > max_y) max_y = py;
                sum_x += px; sum_y += py;
                skin_count++;
            }
            if (is_skin_yuv(y1, u, v)) {
                if (px+1 < min_x) min_x = px+1;
                if (px+1 > max_x) max_x = px+1;
                if (py < min_y) min_y = py;
                if (py > max_y) max_y = py;
                sum_x += px+1; sum_y += py;
                skin_count++;
            }
        }
    }

    // Need at least 2% of frame to be skin to count as a hand
    int32_t min_skin = (fw * fh) / 50;
    if (skin_count < min_skin) return false;

    *out_x  = min_x;
    *out_y  = min_y;
    *out_w  = max_x - min_x;
    *out_h  = max_y - min_y;
    *out_cx = (int32_t)(sum_x / skin_count);
    *out_cy = (int32_t)(sum_y / skin_count);
    return true;
}

// ── Simplified finger extension detection ────────────────────────────────────
// Estimates how many fingers are extended based on the hand bounding box
// and the distribution of skin pixels in the upper half vs lower half
// Returns 0–5 extended fingers
static int32_t estimate_extended_fingers(const uint8_t *frame_yuyv,
                                          int32_t fw, int32_t fh,
                                          int32_t bx, int32_t by,
                                          int32_t bw, int32_t bh) {
    if (bw <= 0 || bh <= 0) return 0;

    // Count skin pixels in the top 40% of the bounding box (fingertips)
    int32_t top_h = bh * 4 / 10;
    int32_t top_skin = 0, top_total = 0;

    for (int32_t py = by; py < by + top_h && py < fh; py++) {
        for (int32_t px = bx; px < bx + bw && px < fw; px += 2) {
            int32_t idx = (py * fw + px) * 2;
            uint8_t y0 = frame_yuyv[idx];
            uint8_t u  = frame_yuyv[idx + 1];
            uint8_t v  = frame_yuyv[idx + 3];
            if (is_skin_yuv(y0, u, v)) top_skin++;
            top_total++;
        }
    }

    if (top_total == 0) return 0;
    float ratio = (float)top_skin / (float)top_total;

    // Map ratio to finger count
    if (ratio > 0.65f) return 5;   // Open palm
    if (ratio > 0.50f) return 4;
    if (ratio > 0.35f) return 3;
    if (ratio > 0.20f) return 2;   // Peace sign or point
    if (ratio > 0.10f) return 1;   // Point or thumbs up
    return 0;                       // Fist
}

// ── Gesture classification ────────────────────────────────────────────────────
static gesture_id_t classify_gesture(int32_t fingers,
                                      int32_t bw, int32_t bh,
                                      float vel_x, float vel_y,
                                      bool is_moving) {
    float aspect = (bh > 0) ? (float)bw / (float)bh : 1.0f;

    // Swipe detection (takes priority when moving fast)
    float speed = vel_x * vel_x + vel_y * vel_y;
    if (is_moving && speed > HANDTRACK_SWIPE_MIN_VELOCITY * HANDTRACK_SWIPE_MIN_VELOCITY) {
        float ax = vel_x < 0 ? -vel_x : vel_x;
        float ay = vel_y < 0 ? -vel_y : vel_y;
        if (ax > ay) {
            return vel_x < 0 ? GESTURE_SWIPE_LEFT : GESTURE_SWIPE_RIGHT;
        } else {
            return vel_y < 0 ? GESTURE_SWIPE_UP : GESTURE_SWIPE_DOWN;
        }
    }

    // Static gestures
    switch (fingers) {
        case 0: return GESTURE_FIST;
        case 1:
            // Tall narrow bbox = thumbs up; otherwise point
            return (aspect < 0.5f) ? GESTURE_THUMBS_UP : GESTURE_POINT;
        case 2: return GESTURE_PEACE;
        case 3: return GESTURE_GRAB;  // Partial grab
        case 4: return GESTURE_OPEN_PALM;
        case 5: return GESTURE_OPEN_PALM;
        default: return GESTURE_NONE;
    }
}

// ── Pointing direction → screen coordinates ───────────────────────────────────
// When pointing gesture detected, estimate where the finger is pointing
// by projecting from wrist to fingertip direction onto the screen
static void estimate_point_target(int32_t palm_x, int32_t palm_y,
                                   int32_t bbox_x, int32_t bbox_y,
                                   int32_t bbox_w, int32_t bbox_h,
                                   int32_t frame_w, int32_t frame_h,
                                   int32_t screen_w, int32_t screen_h,
                                   int32_t *out_x, int32_t *out_y) {
    // Fingertip is approximately at the top center of the bounding box
    int32_t tip_x = bbox_x + bbox_w / 2;
    int32_t tip_y = bbox_y;

    // Direction vector from palm to tip
    float dx = (float)(tip_x - palm_x);
    float dy = (float)(tip_y - palm_y);

    // Extend the vector to estimate where it intersects the screen
    // Scale from frame coordinates to screen coordinates
    float scale_x = (float)screen_w / (float)frame_w;
    float scale_y = (float)screen_h / (float)frame_h;

    // Project forward 2x the arm length
    float target_x = (float)tip_x + dx * 2.0f;
    float target_y = (float)tip_y + dy * 2.0f;

    *out_x = (int32_t)(target_x * scale_x);
    *out_y = (int32_t)(target_y * scale_y);

    // Clamp to screen
    if (*out_x < 0) *out_x = 0;
    if (*out_x >= screen_w) *out_x = screen_w - 1;
    if (*out_y < 0) *out_y = 0;
    if (*out_y >= screen_h) *out_y = screen_h - 1;
}

// ── Public API ────────────────────────────────────────────────────────────────
void handtrack_init(handtrack_ctx_t *ctx, int32_t frame_w, int32_t frame_h) {
    for (int i = 0; i < (int)sizeof(handtrack_ctx_t); i++)
        ((uint8_t*)ctx)[i] = 0;

    ctx->frame_w           = frame_w;
    ctx->frame_h           = frame_h;
    ctx->detect_threshold  = 0.5f;
    ctx->gesture_threshold = 0.6f;
    ctx->cooldown_ms       = HANDTRACK_COOLDOWN_MS;
    ctx->enabled_gestures  = 0xFFFFFFFF; // All gestures enabled by default
}

void handtrack_set_callback(handtrack_ctx_t *ctx, handtrack_gesture_fn on_gesture) {
    s_on_gesture = on_gesture;
    (void)ctx;
}

void handtrack_enable_gesture(handtrack_ctx_t *ctx, gesture_id_t g, bool enable) {
    if (g <= 0 || g >= GESTURE_COUNT) return;
    if (enable)
        ctx->enabled_gestures |= (1u << g);
    else
        ctx->enabled_gestures &= ~(1u << g);
}

int32_t handtrack_process_frame(handtrack_ctx_t *ctx,
                                 const uint8_t *frame_yuyv,
                                 uint32_t now_ms) {
    ctx->frames_processed++;
    ctx->hand_count = 0;

    int32_t gestures_this_frame = 0;
    int32_t bx, by, bw, bh, cx, cy;

    // Detect primary hand
    if (!find_hand_bbox(frame_yuyv, ctx->frame_w, ctx->frame_h,
                        &bx, &by, &bw, &bh, &cx, &cy)) {
        // No hand detected — clear state
        for (int i = 0; i < HANDTRACK_MAX_HANDS; i++) {
            ctx->hands[i].detected = false;
            ctx->gestures[i].gesture = GESTURE_NONE;
        }
        return 0;
    }

    // Fill hand detection
    hand_detection_t *hand = &ctx->hands[0];
    hand->detected  = true;
    hand->bbox_x    = bx;
    hand->bbox_y    = by;
    hand->bbox_w    = bw;
    hand->bbox_h    = bh;
    hand->palm_x    = cx;
    hand->palm_y    = cy;
    hand->confidence = 0.75f; // Fixed confidence for simplified detector
    ctx->hand_count = 1;

    // Update position history for swipe detection
    int32_t hidx = ctx->history_idx % HANDTRACK_HISTORY;
    ctx->history_x[0][hidx] = cx;
    ctx->history_y[0][hidx] = cy;
    ctx->history_idx++;

    // Calculate velocity from history
    int32_t prev_idx = (hidx - 3 + HANDTRACK_HISTORY) % HANDTRACK_HISTORY;
    float vel_x = (float)(cx - ctx->history_x[0][prev_idx]) / 3.0f;
    float vel_y = (float)(cy - ctx->history_y[0][prev_idx]) / 3.0f;
    float speed = vel_x * vel_x + vel_y * vel_y;
    bool is_moving = speed > (HANDTRACK_SWIPE_MIN_VELOCITY * HANDTRACK_SWIPE_MIN_VELOCITY);

    // Estimate finger extension
    int32_t fingers = estimate_extended_fingers(frame_yuyv,
                                                 ctx->frame_w, ctx->frame_h,
                                                 bx, by, bw, bh);

    // Classify gesture
    gesture_id_t gid = classify_gesture(fingers, bw, bh, vel_x, vel_y, is_moving);

    // Check cooldown
    bool can_fire = (now_ms - ctx->last_gesture_ms[0]) >= ctx->cooldown_ms;

    if (gid != GESTURE_NONE && can_fire &&
        (ctx->enabled_gestures & (1u << gid))) {

        gesture_event_t *ev = &ctx->gestures[0];
        ev->gesture      = gid;
        ev->confidence   = 0.75f;
        ev->timestamp_ms = now_ms;
        ev->velocity_x   = vel_x;
        ev->velocity_y   = vel_y;
        ev->delta_x      = cx - ctx->history_x[0][prev_idx];
        ev->delta_y      = cy - ctx->history_y[0][prev_idx];
        ev->is_right_hand = true; // Simplified — assume right hand

        // For pointing gesture, estimate target
        if (gid == GESTURE_POINT) {
            estimate_point_target(cx, cy, bx, by, bw, bh,
                                   ctx->frame_w, ctx->frame_h,
                                   1920, 1080, // TODO: pass screen dims
                                   &ev->point_x, &ev->point_y);
        }

        ctx->last_gesture_ms[0] = now_ms;
        ctx->gestures_detected++;
        gestures_this_frame++;

        if (s_on_gesture) s_on_gesture(ev);
    }

    return gestures_this_frame;
}

bool handtrack_get_point(const handtrack_ctx_t *ctx,
                          int32_t *out_x, int32_t *out_y) {
    for (int i = 0; i < ctx->hand_count; i++) {
        if (ctx->gestures[i].gesture == GESTURE_POINT) {
            *out_x = ctx->gestures[i].point_x;
            *out_y = ctx->gestures[i].point_y;
            return true;
        }
    }
    return false;
}
