/*
 * BUBO OS — Boot Initialization System
 * kernel/bubo_boot.c
 *
 * Parallel initialization with non-blocking Rinnegan boot animation.
 *
 * The Rinnegan eye has six tomoe — one per input face.
 * Each tomoe lights when its face finishes initializing.
 * The animation renders from a timer interrupt — it never blocks init.
 * When all six tomoe spin, the eye opens. Boot is complete.
 *
 * If a tomoe stays dark — that face failed. You know immediately.
 * No black screen. No cryptic error. The eye tells you.
 *
 * Boot sequence order (dependency graph — parallel where possible):
 *
 *   PHASE 0 (serial — must be first):
 *     GDT → IDT → Memory Manager → Scheduler
 *
 *   PHASE 1 (parallel — no dependencies on each other):
 *     Keyboard init     → lights RINNEGAN_TOMOE_KEYBOARD
 *     Controller init   → lights RINNEGAN_TOMOE_CONTROLLER
 *     VFS init          → (internal, no tomoe)
 *     ARCHIVIST init    → (internal, no tomoe)
 *
 *   PHASE 2 (parallel — depends on phase 1):
 *     Voice pipeline    → lights RINNEGAN_TOMOE_VOICE
 *     Rinnegan gaze     → lights RINNEGAN_TOMOE_GAZE
 *     Gesture engine    → lights RINNEGAN_TOMOE_GESTURE
 *
 *   PHASE 3 (serial — depends on all above):
 *     Vera Workflow init → lights RINNEGAN_TOMOE_ARBITER
 *     Desktop launch    → eye opens, animation holds, desktop appears
 *
 * Target: under 3 seconds from power button to desktop.
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#include "../include/vera_workflow.h"
#include "../include/bubo_input_map.h"
#include "../include/deepflow_colors.h"

/* ── Boot color aliases — map semantic names to deepflow_colors.h ── */
#define COLOR_VOID          DF_BG_DEEP          /* Near-black — unlit background  */
#define COLOR_LANDON_PURPLE DF_USER_LANDON      /* Deep purple — Landon's tomoe   */
#define COLOR_DEPTH_2       DF_RED_DIM          /* Dimmed red — partial progress  */
#define COLOR_DEPTH_3       DF_RED_MID          /* Mid red — active spin          */

/* =============================================================================
 * Framebuffer — provided by VESA init before boot sequence starts
 * ============================================================================= */

extern uint32_t *fb_base;       /* Framebuffer base address                    */
extern uint32_t  fb_width;      /* Screen width in pixels                      */
extern uint32_t  fb_height;     /* Screen height in pixels                     */
extern uint32_t  fb_pitch;      /* Bytes per row                               */

/* =============================================================================
 * Rinnegan animation state — updated by timer interrupt, read by renderer
 * ============================================================================= */

typedef struct {
    /* Tomoe spin angles (degrees * 100 for integer math) */
    int32_t  tomoe_angle[RINNEGAN_TOMOE_COUNT];

    /* Which tomoe are currently lit */
    uint8_t  tomoe_lit[RINNEGAN_TOMOE_COUNT];

    /* Overall eye open progress (0–100) */
    uint8_t  eye_open_pct;

    /* Animation phase */
    uint8_t  phase_complete;    /* 1 = all lit, holding open */
    uint32_t hold_start_ms;

    /* Render position (center of screen) */
    int32_t  center_x;
    int32_t  center_y;
    int32_t  eye_radius;        /* Outer ring radius in pixels */
} rinnegan_anim_t;

static rinnegan_anim_t rinnegan;
static volatile uint32_t boot_tick_ms = 0;

/* =============================================================================
 * Integer math helpers (no FPU in early boot)
 * ============================================================================= */

/* Fixed-point sin/cos table (256 entries, scaled by 1000) */
static const int16_t sin_table[256] = {
       0,   25,   49,   74,   98,  122,  147,  171,  195,  219,  243,  267,
     290,  314,  337,  360,  383,  405,  428,  450,  471,  493,  514,  535,
     556,  576,  596,  615,  634,  653,  672,  690,  707,  724,  741,  757,
     773,  788,  803,  818,  831,  845,  857,  870,  882,  893,  904,  914,
     924,  933,  942,  950,  957,  964,  970,  976,  981,  985,  989,  993,
     995,  998,  999, 1000, 1000, 1000,  999,  998,  995,  993,  989,  985,
     981,  976,  970,  964,  957,  950,  942,  933,  924,  914,  904,  893,
     882,  870,  857,  845,  831,  818,  803,  788,  773,  757,  741,  724,
     707,  690,  672,  653,  634,  615,  596,  576,  556,  535,  514,  493,
     471,  450,  428,  405,  383,  360,  337,  314,  290,  267,  243,  219,
     195,  171,  147,  122,   98,   74,   49,   25,    0,  -25,  -49,  -74,
     -98, -122, -147, -171, -195, -219, -243, -267, -290, -314, -337, -360,
    -383, -405, -428, -450, -471, -493, -514, -535, -556, -576, -596, -615,
    -634, -653, -672, -690, -707, -724, -741, -757, -773, -788, -803, -818,
    -831, -845, -857, -870, -882, -893, -904, -914, -924, -933, -942, -950,
    -957, -964, -970, -976, -981, -985, -989, -993, -995, -998, -999,-1000,
   -1000,-1000, -999, -998, -995, -993, -989, -985, -981, -976, -970, -964,
    -957, -950, -942, -933, -924, -914, -904, -893, -882, -870, -857, -845,
    -831, -818, -803, -788, -773, -757, -741, -724, -707, -690, -672, -653,
    -634, -615, -596, -576, -556, -535, -514, -493, -471, -450, -428, -405,
    -383, -360, -337, -314, -290, -267, -243, -219, -195, -171, -147, -122,
     -98,  -74,  -49,  -25
};

static inline int32_t isin(int32_t deg) {
    return sin_table[((deg % 360) + 360) % 360];
}

static inline int32_t icos(int32_t deg) {
    return sin_table[((deg + 90) % 360 + 360) % 360];
}

/* =============================================================================
 * Framebuffer pixel drawing
 * ============================================================================= */

static inline void fb_put_pixel(int32_t x, int32_t y, uint32_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= fb_width || (uint32_t)y >= fb_height) return;
    fb_base[y * (fb_pitch / 4) + x] = color;
}

static void fb_draw_circle(int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    /* Midpoint circle algorithm */
    int32_t x = r, y = 0, err = 0;
    while (x >= y) {
        fb_put_pixel(cx + x, cy + y, color);
        fb_put_pixel(cx + y, cy + x, color);
        fb_put_pixel(cx - y, cy + x, color);
        fb_put_pixel(cx - x, cy + y, color);
        fb_put_pixel(cx - x, cy - y, color);
        fb_put_pixel(cx - y, cy - x, color);
        fb_put_pixel(cx + y, cy - x, color);
        fb_put_pixel(cx + x, cy - y, color);
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x--; err += 1 - 2 * x; }
    }
}

static void fb_fill_circle(int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    for (int32_t dy = -r; dy <= r; dy++) {
        int32_t dx_max = 0;
        int32_t dy2 = dy * dy;
        int32_t r2  = r * r;
        /* Find x extent at this y */
        for (int32_t dx = r; dx >= 0; dx--) {
            if (dx * dx + dy2 <= r2) { dx_max = dx; break; }
        }
        for (int32_t dx = -dx_max; dx <= dx_max; dx++) {
            fb_put_pixel(cx + dx, cy + dy, color);
        }
    }
}

/* =============================================================================
 * Rinnegan renderer — called from timer interrupt (non-blocking)
 * ============================================================================= */

void bubo_boot_render_rinnegan(void) {
    if (!fb_base) return;

    int32_t cx = rinnegan.center_x;
    int32_t cy = rinnegan.center_y;
    int32_t R  = rinnegan.eye_radius;

    /* Clear boot area (black background) */
    for (int32_t y = cy - R - 20; y <= cy + R + 20; y++) {
        for (int32_t x = cx - R - 20; x <= cx + R + 20; x++) {
            fb_put_pixel(x, y, COLOR_VOID);
        }
    }

    /* Draw outer ring — deep purple (Rinnegan color) */
    uint32_t ring_color = COLOR_LANDON_PURPLE;
    fb_draw_circle(cx, cy, R,     ring_color);
    fb_draw_circle(cx, cy, R - 1, ring_color);
    fb_draw_circle(cx, cy, R - 2, ring_color);

    /* Draw inner rings */
    fb_draw_circle(cx, cy, R * 2 / 3, ring_color);
    fb_draw_circle(cx, cy, R / 3,     ring_color);

    /* Draw six tomoe — comma-shaped marks that spin */
    for (int i = 0; i < RINNEGAN_TOMOE_COUNT; i++) {
        /* Each tomoe starts 60 degrees apart, then spins */
        int32_t base_angle = i * 60;
        int32_t angle      = (base_angle + rinnegan.tomoe_angle[i] / 100) % 360;

        /* Tomoe orbit radius: between inner and outer ring */
        int32_t orbit_r = R * 5 / 6;

        /* Tomoe center position */
        int32_t tx = cx + (orbit_r * icos(angle)) / 1000;
        int32_t ty = cy + (orbit_r * isin(angle)) / 1000;

        /* Tomoe color: lit = Landon purple, unlit = dark gray */
        uint32_t tomoe_color = rinnegan.tomoe_lit[i]
                               ? COLOR_LANDON_PURPLE
                               : COLOR_DEPTH_3;

        /* Draw tomoe body (small filled circle) */
        int32_t tomoe_r = R / 8;
        fb_fill_circle(tx, ty, tomoe_r, tomoe_color);

        /* Draw tomoe tail (small circle offset from body) */
        int32_t tail_angle = (angle + 30) % 360;
        int32_t tail_x = tx + (tomoe_r * icos(tail_angle)) / 1000;
        int32_t tail_y = ty + (tomoe_r * isin(tail_angle)) / 1000;
        fb_fill_circle(tail_x, tail_y, tomoe_r / 2, tomoe_color);
    }

    /* Draw center pupil */
    uint32_t pupil_color = rinnegan.phase_complete
                           ? COLOR_LANDON_PURPLE
                           : COLOR_DEPTH_2;
    fb_fill_circle(cx, cy, R / 5, pupil_color);

    /* Draw "BUBO OS" text hint below eye (simple pixel art — TODO: font renderer) */
    /* For now: just a thin line to indicate loading progress */
    int32_t lit_count = 0;
    for (int i = 0; i < RINNEGAN_TOMOE_COUNT; i++) {
        if (rinnegan.tomoe_lit[i]) lit_count++;
    }
    int32_t bar_width = (R * 2 * lit_count) / RINNEGAN_TOMOE_COUNT;
    int32_t bar_y     = cy + R + 12;
    for (int32_t bx = cx - R; bx < cx - R + bar_width; bx++) {
        fb_put_pixel(bx, bar_y,     COLOR_LANDON_PURPLE);
        fb_put_pixel(bx, bar_y + 1, COLOR_LANDON_PURPLE);
    }
    /* Background of bar */
    for (int32_t bx = cx - R + bar_width; bx <= cx + R; bx++) {
        fb_put_pixel(bx, bar_y,     COLOR_DEPTH_2);
        fb_put_pixel(bx, bar_y + 1, COLOR_DEPTH_2);
    }
}

/* =============================================================================
 * Timer tick — called by PIT interrupt every ~10ms
 * Advances the Rinnegan animation. Never blocks.
 * ============================================================================= */

void bubo_boot_tick(void) {
    boot_tick_ms += 10;

    /* Advance spinning tomoe angles */
    for (int i = 0; i < RINNEGAN_TOMOE_COUNT; i++) {
        if (rinnegan.tomoe_lit[i]) {
            /* Lit tomoe spin at full rate */
            rinnegan.tomoe_angle[i] += (36000 / RINNEGAN_SPIN_RATE_MS) * 10;
            if (rinnegan.tomoe_angle[i] >= 36000) {
                rinnegan.tomoe_angle[i] -= 36000;
            }
        }
        /* Unlit tomoe do not spin */
    }

    /* Sync tomoe lit state from Vera face state */
    const vera_face_state_t *faces = vera_get_face_state();
    for (int i = 0; i < RINNEGAN_TOMOE_COUNT; i++) {
        rinnegan.tomoe_lit[i] = faces->tomoe_active[i];
    }

    /* Check if all lit */
    if (faces->all_faces_ready && !rinnegan.phase_complete) {
        rinnegan.phase_complete = 1;
        rinnegan.hold_start_ms  = boot_tick_ms;
        rinnegan.eye_open_pct   = 100;
    }

    /* Render frame */
    bubo_boot_render_rinnegan();
}

/* =============================================================================
 * Boot initialization — called from kernel main after VESA framebuffer ready
 * ============================================================================= */

void bubo_boot_init(void) {
    /* Set up animation center */
    rinnegan.center_x   = (int32_t)fb_width  / 2;
    rinnegan.center_y   = (int32_t)fb_height  / 2;
    rinnegan.eye_radius = (fb_height < fb_width ? fb_height : fb_width) / 6;

    for (int i = 0; i < RINNEGAN_TOMOE_COUNT; i++) {
        rinnegan.tomoe_angle[i] = 0;
        rinnegan.tomoe_lit[i]   = 0;
    }
    rinnegan.eye_open_pct   = 0;
    rinnegan.phase_complete = 0;

    /* Initialize Vera first — she coordinates everything */
    vera_init();

    /*
     * PHASE 1 — parallel initialization
     * In a real multi-core kernel, these would run on separate cores.
     * On single-core, they run sequentially but each reports to Vera
     * as soon as it finishes, so the animation updates live.
     *
     * Each init function calls vera_face_online(tomoe_index) when done.
     */

    /* Keyboard */
    /* bubo_keyboard_init() is called from arch/x86/irq.c — it calls: */
    /* vera_face_online(RINNEGAN_TOMOE_KEYBOARD);                      */

    /* Controllers */
    /* bubo_controller_init() calls:                                   */
    /* vera_face_online(RINNEGAN_TOMOE_CONTROLLER);                    */

    /* Voice pipeline */
    /* dysarthria_engine_init() calls:                                 */
    /* vera_face_online(RINNEGAN_TOMOE_VOICE);                         */

    /* Rinnegan gaze */
    /* rinnegan_init() calls:                                          */
    /* vera_face_online(RINNEGAN_TOMOE_GAZE);                          */

    /* Gesture engine */
    /* handtrack_init() calls:                                         */
    /* vera_face_online(RINNEGAN_TOMOE_GESTURE);                       */

    /*
     * PHASE 3 — Vera arbiter comes online last
     * She is the sixth tomoe. When she lights, the eye opens.
     */
    vera_face_online(RINNEGAN_TOMOE_ARBITER);

    /*
     * Boot animation continues until all tomoe are lit.
     * The PIT timer calls bubo_boot_tick() every 10ms.
     * When rinnegan.phase_complete == 1 and hold time has elapsed,
     * the desktop launcher takes over.
     */
}

/* =============================================================================
 * Boot complete check — called by main loop
 * Returns 1 when animation has held fully open and desktop should launch.
 * ============================================================================= */

int bubo_boot_complete(void) {
    if (!rinnegan.phase_complete) return 0;
    return (boot_tick_ms - rinnegan.hold_start_ms) >= RINNEGAN_OPEN_HOLD_MS;
}
