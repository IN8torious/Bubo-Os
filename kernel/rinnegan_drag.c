// BUBO OS — RINNEGAN Drag System Implementation
// Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
//
// Dwell-Lock-Drag-Drop: eye-tracked window dragging and resizing.
// The Rinnegan doesn't just select — it moves the world.
// =============================================================================
#include "../include/rinnegan_drag.h"
#include "../include/deepflow_colors.h"
#include <stddef.h>

// ── Internal helpers ──────────────────────────────────────────────────────────
static rdrag_move_fn     s_on_move     = NULL;
static rdrag_resize_fn   s_on_resize   = NULL;
static rdrag_feedback_fn s_on_feedback = NULL;

static inline int32_t clamp(int32_t v, int32_t lo, int32_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline int32_t snap(int32_t v, int32_t grid) {
    if (grid <= 1) return v;
    return (v / grid) * grid;
}

// Hit-test a window to find which drag target the gaze is on
static rdrag_target_t hit_test(const rdrag_window_t *w, int32_t gx, int32_t gy) {
    if (!w->draggable && !w->resizable) return RDRAG_TARGET_NONE;

    int32_t x2 = w->x + w->w;
    int32_t y2 = w->y + w->h;
    int32_t cs = RDRAG_CORNER_SIZE;
    int32_t es = RDRAG_EDGE_SIZE;

    // Outside window entirely
    if (gx < w->x || gx > x2 || gy < w->y || gy > y2)
        return RDRAG_TARGET_NONE;

    if (w->resizable) {
        // Corners (checked first — higher priority)
        if (gx <= w->x + cs && gy <= w->y + cs) return RDRAG_TARGET_CORNER_TL;
        if (gx >= x2 - cs   && gy <= w->y + cs) return RDRAG_TARGET_CORNER_TR;
        if (gx <= w->x + cs && gy >= y2 - cs)   return RDRAG_TARGET_CORNER_BL;
        if (gx >= x2 - cs   && gy >= y2 - cs)   return RDRAG_TARGET_CORNER_BR;

        // Edges
        if (gy <= w->y + es) return RDRAG_TARGET_EDGE_T;
        if (gy >= y2 - es)   return RDRAG_TARGET_EDGE_B;
        if (gx <= w->x + es) return RDRAG_TARGET_EDGE_L;
        if (gx >= x2 - es)   return RDRAG_TARGET_EDGE_R;
    }

    // Title bar drag zone
    if (w->draggable && gy <= w->y + RDRAG_TITLEBAR_H)
        return RDRAG_TARGET_TITLEBAR;

    return RDRAG_TARGET_NONE;
}

// Find the topmost window under the gaze
static rdrag_window_t *find_window(rdrag_window_t *windows, int32_t count,
                                    int32_t gx, int32_t gy,
                                    rdrag_target_t *out_target) {
    // Iterate in reverse (topmost window last in array)
    for (int32_t i = count - 1; i >= 0; i--) {
        rdrag_target_t t = hit_test(&windows[i], gx, gy);
        if (t != RDRAG_TARGET_NONE) {
            *out_target = t;
            return &windows[i];
        }
    }
    *out_target = RDRAG_TARGET_NONE;
    return NULL;
}

// Apply move with bounds clamping
static void apply_move(rdrag_ctx_t *ctx, rdrag_window_t *windows,
                        int32_t count, int32_t new_x, int32_t new_y) {
    // Find the active window
    for (int32_t i = 0; i < count; i++) {
        if (windows[i].id != ctx->window_id) continue;
        rdrag_window_t *w = &windows[i];

        // Clamp to screen bounds (keep title bar visible)
        new_x = clamp(new_x, 0, ctx->screen_w - w->w);
        new_y = clamp(new_y, ctx->topbar_h,
                       ctx->screen_h - ctx->taskbar_h - RDRAG_TITLEBAR_H);

        if (ctx->snap_enabled) {
            new_x = snap(new_x, ctx->snap_grid);
            new_y = snap(new_y, ctx->snap_grid);
        }

        w->x = new_x;
        w->y = new_y;

        if (s_on_move) s_on_move(ctx->window_id, new_x, new_y);
        return;
    }
}

// Apply resize with min/max clamping
static void apply_resize(rdrag_ctx_t *ctx, rdrag_window_t *windows,
                          int32_t count, int32_t dx, int32_t dy) {
    for (int32_t i = 0; i < count; i++) {
        if (windows[i].id != ctx->window_id) continue;
        rdrag_window_t *w = &windows[i];

        int32_t new_x = ctx->anchor_win_x;
        int32_t new_y = ctx->anchor_win_y;
        int32_t new_w = ctx->anchor_win_w;
        int32_t new_h = ctx->anchor_win_h;

        switch (ctx->target) {
            case RDRAG_TARGET_CORNER_BR:
                new_w = ctx->anchor_win_w + dx;
                new_h = ctx->anchor_win_h + dy;
                break;
            case RDRAG_TARGET_CORNER_BL:
                new_x = ctx->anchor_win_x + dx;
                new_w = ctx->anchor_win_w - dx;
                new_h = ctx->anchor_win_h + dy;
                break;
            case RDRAG_TARGET_CORNER_TR:
                new_y = ctx->anchor_win_y + dy;
                new_w = ctx->anchor_win_w + dx;
                new_h = ctx->anchor_win_h - dy;
                break;
            case RDRAG_TARGET_CORNER_TL:
                new_x = ctx->anchor_win_x + dx;
                new_y = ctx->anchor_win_y + dy;
                new_w = ctx->anchor_win_w - dx;
                new_h = ctx->anchor_win_h - dy;
                break;
            case RDRAG_TARGET_EDGE_R:
                new_w = ctx->anchor_win_w + dx;
                break;
            case RDRAG_TARGET_EDGE_L:
                new_x = ctx->anchor_win_x + dx;
                new_w = ctx->anchor_win_w - dx;
                break;
            case RDRAG_TARGET_EDGE_B:
                new_h = ctx->anchor_win_h + dy;
                break;
            case RDRAG_TARGET_EDGE_T:
                new_y = ctx->anchor_win_y + dy;
                new_h = ctx->anchor_win_h - dy;
                break;
            default: break;
        }

        // Enforce min size
        if (w->min_w > 0 && new_w < w->min_w) new_w = w->min_w;
        if (w->min_h > 0 && new_h < w->min_h) new_h = w->min_h;
        // Enforce max size
        if (w->max_w > 0 && new_w > w->max_w) new_w = w->max_w;
        if (w->max_h > 0 && new_h > w->max_h) new_h = w->max_h;

        if (ctx->snap_enabled) {
            new_w = snap(new_w, ctx->snap_grid);
            new_h = snap(new_h, ctx->snap_grid);
        }

        w->x = new_x; w->y = new_y;
        w->w = new_w; w->h = new_h;

        if (s_on_resize) s_on_resize(ctx->window_id, new_x, new_y, new_w, new_h);
        return;
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
void rdrag_init(rdrag_ctx_t *ctx, int32_t screen_w, int32_t screen_h,
                int32_t taskbar_h, int32_t topbar_h) {
    for (int i = 0; i < (int)sizeof(rdrag_ctx_t); i++)
        ((uint8_t*)ctx)[i] = 0;

    ctx->state        = RDRAG_IDLE;
    ctx->screen_w     = screen_w;
    ctx->screen_h     = screen_h;
    ctx->taskbar_h    = taskbar_h;
    ctx->topbar_h     = topbar_h;
    ctx->dwell_lock_ms = RDRAG_LOCK_DWELL_MS;
    ctx->dwell_drop_ms = RDRAG_DROP_DWELL_MS;
    ctx->snap_enabled = true;
    ctx->snap_grid    = RDRAG_SNAP_GRID;
    ctx->window_id    = -1;
}

void rdrag_set_callbacks(rdrag_ctx_t *ctx,
                          rdrag_move_fn on_move,
                          rdrag_resize_fn on_resize,
                          rdrag_feedback_fn on_feedback) {
    s_on_move     = on_move;
    s_on_resize   = on_resize;
    s_on_feedback = on_feedback;
    (void)ctx;
}

void rdrag_update(rdrag_ctx_t *ctx,
                  int32_t gaze_x, int32_t gaze_y,
                  uint32_t now_ms,
                  bool blink_detected,
                  rdrag_window_t *windows, int32_t window_count) {

    ctx->gaze_x = gaze_x;
    ctx->gaze_y = gaze_y;

    // Blink = drop if dragging
    if (blink_detected &&
        (ctx->state == RDRAG_DRAGGING || ctx->state == RDRAG_LOCKED ||
         ctx->state == RDRAG_RESIZING)) {
        rdrag_force_drop(ctx);
        return;
    }

    switch (ctx->state) {

    case RDRAG_IDLE: {
        rdrag_target_t target;
        rdrag_window_t *w = find_window(windows, window_count,
                                         gaze_x, gaze_y, &target);
        if (w) {
            ctx->window_id = w->id;
            ctx->target    = target;
            ctx->dwell_start_ms = now_ms;

            if (target == RDRAG_TARGET_TITLEBAR)
                ctx->state = RDRAG_HOVER;
            else
                ctx->state = RDRAG_RESIZE_HOVER;

            if (s_on_feedback)
                s_on_feedback(ctx->window_id, ctx->state, RDRAG_COLOR_HOVER);
        }
        break;
    }

    case RDRAG_HOVER: {
        rdrag_target_t target;
        rdrag_window_t *w = find_window(windows, window_count,
                                         gaze_x, gaze_y, &target);
        if (!w || w->id != ctx->window_id || target != RDRAG_TARGET_TITLEBAR) {
            // Gaze left — back to idle
            ctx->state = RDRAG_IDLE;
            ctx->window_id = -1;
            if (s_on_feedback) s_on_feedback(-1, RDRAG_IDLE, 0);
            break;
        }
        // Still hovering — check dwell timer
        if ((now_ms - ctx->dwell_start_ms) >= ctx->dwell_lock_ms) {
            ctx->state = RDRAG_LOCKING;
        }
        break;
    }

    case RDRAG_LOCKING: {
        // Find anchor window
        for (int32_t i = 0; i < window_count; i++) {
            if (windows[i].id != ctx->window_id) continue;
            ctx->anchor_gaze_x = gaze_x;
            ctx->anchor_gaze_y = gaze_y;
            ctx->anchor_win_x  = windows[i].x;
            ctx->anchor_win_y  = windows[i].y;
            ctx->anchor_win_w  = windows[i].w;
            ctx->anchor_win_h  = windows[i].h;
            break;
        }
        ctx->state = RDRAG_DRAGGING;
        if (s_on_feedback)
            s_on_feedback(ctx->window_id, RDRAG_DRAGGING, RDRAG_COLOR_LOCKED);
        break;
    }

    case RDRAG_DRAGGING: {
        int32_t dx = gaze_x - ctx->anchor_gaze_x;
        int32_t dy = gaze_y - ctx->anchor_gaze_y;
        int32_t new_x = ctx->anchor_win_x + dx;
        int32_t new_y = ctx->anchor_win_y + dy;
        apply_move(ctx, windows, window_count, new_x, new_y);

        // Check if gaze has left the window entirely (drop zone)
        rdrag_target_t target;
        rdrag_window_t *w = find_window(windows, window_count,
                                         gaze_x, gaze_y, &target);
        if (!w || w->id != ctx->window_id) {
            if (ctx->drop_start_ms == 0) {
                ctx->drop_start_ms = now_ms;
                ctx->state = RDRAG_DROPPING;
                if (s_on_feedback)
                    s_on_feedback(ctx->window_id, RDRAG_DROPPING, RDRAG_COLOR_DROP_ZONE);
            }
        } else {
            ctx->drop_start_ms = 0;
        }
        break;
    }

    case RDRAG_DROPPING: {
        // Continue moving while in drop state
        int32_t dx = gaze_x - ctx->anchor_gaze_x;
        int32_t dy = gaze_y - ctx->anchor_gaze_y;
        apply_move(ctx, windows, window_count,
                   ctx->anchor_win_x + dx, ctx->anchor_win_y + dy);

        if ((now_ms - ctx->drop_start_ms) >= ctx->dwell_drop_ms) {
            rdrag_force_drop(ctx);
        }
        break;
    }

    case RDRAG_RESIZE_HOVER: {
        rdrag_target_t target;
        rdrag_window_t *w = find_window(windows, window_count,
                                         gaze_x, gaze_y, &target);
        if (!w || w->id != ctx->window_id) {
            ctx->state = RDRAG_IDLE;
            ctx->window_id = -1;
            if (s_on_feedback) s_on_feedback(-1, RDRAG_IDLE, 0);
            break;
        }
        if ((now_ms - ctx->dwell_start_ms) >= ctx->dwell_lock_ms) {
            ctx->state = RDRAG_RESIZE_LOCKING;
        }
        break;
    }

    case RDRAG_RESIZE_LOCKING: {
        for (int32_t i = 0; i < window_count; i++) {
            if (windows[i].id != ctx->window_id) continue;
            ctx->anchor_gaze_x = gaze_x;
            ctx->anchor_gaze_y = gaze_y;
            ctx->anchor_win_x  = windows[i].x;
            ctx->anchor_win_y  = windows[i].y;
            ctx->anchor_win_w  = windows[i].w;
            ctx->anchor_win_h  = windows[i].h;
            break;
        }
        ctx->state = RDRAG_RESIZING;
        if (s_on_feedback)
            s_on_feedback(ctx->window_id, RDRAG_RESIZING, RDRAG_COLOR_RESIZE);
        break;
    }

    case RDRAG_RESIZING: {
        int32_t dx = gaze_x - ctx->anchor_gaze_x;
        int32_t dy = gaze_y - ctx->anchor_gaze_y;
        apply_resize(ctx, windows, window_count, dx, dy);
        break;
    }

    default:
        break;
    }
}

void rdrag_cancel(rdrag_ctx_t *ctx) {
    if (ctx->window_id >= 0 && s_on_feedback)
        s_on_feedback(ctx->window_id, RDRAG_IDLE, 0);
    ctx->state = RDRAG_IDLE;
    ctx->window_id = -1;
    ctx->drop_start_ms = 0;
}

void rdrag_force_drop(rdrag_ctx_t *ctx) {
    if (ctx->window_id >= 0 && s_on_feedback)
        s_on_feedback(ctx->window_id, RDRAG_IDLE, 0);
    ctx->state = RDRAG_IDLE;
    ctx->window_id = -1;
    ctx->drop_start_ms = 0;
}

rdrag_state_t rdrag_get_state(const rdrag_ctx_t *ctx) {
    return ctx->state;
}

int32_t rdrag_get_active_window(const rdrag_ctx_t *ctx) {
    return ctx->window_id;
}

void rdrag_render_feedback(const rdrag_ctx_t *ctx,
                            rdrag_window_t *windows, int32_t window_count) {
    if (ctx->window_id < 0 || ctx->state == RDRAG_IDLE) return;

    uint32_t color = 0;
    switch (ctx->state) {
        case RDRAG_HOVER:
        case RDRAG_RESIZE_HOVER:
            color = RDRAG_COLOR_HOVER; break;
        case RDRAG_LOCKING:
        case RDRAG_LOCKED:
        case RDRAG_DRAGGING:
        case RDRAG_RESIZE_LOCKING:
            color = RDRAG_COLOR_LOCKED; break;
        case RDRAG_DROPPING:
            color = RDRAG_COLOR_DROP_ZONE; break;
        case RDRAG_RESIZING:
            color = RDRAG_COLOR_RESIZE; break;
        default: return;
    }

    // Draw a 3px border around the active window in the feedback color
    for (int32_t i = 0; i < window_count; i++) {
        if (windows[i].id != ctx->window_id) continue;
        rdrag_window_t *w = &windows[i];
        // The actual pixel drawing is done by the framebuffer layer
        // We call the feedback callback with the color and let desktop.c draw it
        if (s_on_feedback) s_on_feedback(w->id, ctx->state, color);
        return;
    }
}
