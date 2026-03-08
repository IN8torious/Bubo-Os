/*
 * BUBO OS — Input Reference Map
 * include/bubo_input_map.h
 *
 * The single source of truth for every input constant in the system.
 * Like deepflow_colors.h but for input — every value has a name,
 * every name has a meaning. No magic numbers anywhere.
 *
 * The Many-Faced God principle:
 *   Six faces. One presence behind them all.
 *   Each face that comes online advances the Rinnegan boot animation.
 *   When all six tomoe spin — the eye is open. The system is alive.
 *
 *   Face 1: KEYBOARD     — the regular human face
 *   Face 2: CONTROLLER   — the gaming face
 *   Face 3: VOICE        — Landon's primary face (dysarthria-tuned)
 *   Face 4: GAZE         — the Rinnegan face (eye tracking)
 *   Face 5: GESTURE      — the hand face (MediaPipe)
 *   Face 6: ARBITER      — the unified face (all paths converge)
 *
 * Constitutional mandate: NO MAS DISADVANTAGED
 * Copyright (c) 2025 Nathan Brown — BUBO OS Community License v1.0
 */

#ifndef BUBO_INPUT_MAP_H
#define BUBO_INPUT_MAP_H

/* =============================================================================
 * BOOT ANIMATION SYNC
 * Each input face that initializes successfully advances the Rinnegan eye.
 * The tomoe (comma marks) spin one by one as each channel comes online.
 * When RINNEGAN_TOMOE_COUNT tomoe are active, the eye is fully open.
 * ============================================================================= */

#define RINNEGAN_TOMOE_COUNT        6       /* One per input face              */
#define RINNEGAN_TOMOE_KEYBOARD     0       /* Tomoe 1: keyboard ready         */
#define RINNEGAN_TOMOE_CONTROLLER   1       /* Tomoe 2: controllers scanned    */
#define RINNEGAN_TOMOE_VOICE        2       /* Tomoe 3: voice pipeline ready   */
#define RINNEGAN_TOMOE_GAZE         3       /* Tomoe 4: eye tracking calibrated*/
#define RINNEGAN_TOMOE_GESTURE      4       /* Tomoe 5: hand gesture ready     */
#define RINNEGAN_TOMOE_ARBITER      5       /* Tomoe 6: unified arbiter online */

/* Spin rate per tomoe (milliseconds per full rotation) */
#define RINNEGAN_SPIN_RATE_MS       800     /* Each tomoe spins at 800ms/rev   */
#define RINNEGAN_OPEN_HOLD_MS       1200    /* Hold fully open before clearing */

/* =============================================================================
 * INPUT CHANNEL IDs
 * Used throughout the system to identify which face an event came from.
 * ============================================================================= */

#define INPUT_CHANNEL_NONE          0x00    /* No channel — invalid event      */
#define INPUT_CHANNEL_KEYBOARD      0x01    /* PS/2 or USB keyboard            */
#define INPUT_CHANNEL_CONTROLLER    0x02    /* Gamepad / controller (1–4)      */
#define INPUT_CHANNEL_VOICE         0x03    /* Dysarthria speech engine        */
#define INPUT_CHANNEL_GAZE          0x04    /* Rinnegan eye tracking           */
#define INPUT_CHANNEL_GESTURE       0x05    /* MediaPipe hand gestures         */
#define INPUT_CHANNEL_ARBITER       0x06    /* Unified arbiter (resolved)      */

/* =============================================================================
 * CONTROLLER BUTTONS — semantic names
 * XInput-compatible layout. Every button named by what it does in BUBO OS.
 * ============================================================================= */

/* D-Pad — navigation in all menus and accessibility panels */
#define CTRL_NAV_UP                 (1 << 0)    /* Navigate up                 */
#define CTRL_NAV_DOWN               (1 << 1)    /* Navigate down               */
#define CTRL_NAV_LEFT               (1 << 2)    /* Navigate left               */
#define CTRL_NAV_RIGHT              (1 << 3)    /* Navigate right              */

/* System buttons */
#define CTRL_MENU                   (1 << 4)    /* Open BUBO menu / pause      */
#define CTRL_BACK                   (1 << 5)    /* Go back / cancel            */
#define CTRL_STICK_LEFT_CLICK       (1 << 6)    /* Left stick press            */
#define CTRL_STICK_RIGHT_CLICK      (1 << 7)    /* Right stick press           */

/* Shoulder buttons */
#define CTRL_BUMPER_LEFT            (1 << 8)    /* LB — cycle left / prev item */
#define CTRL_BUMPER_RIGHT           (1 << 9)    /* RB — cycle right / next item*/

/* Guide / home */
#define CTRL_HOME                   (1 << 10)   /* Guide — summon BUBO         */

/* Face buttons — primary actions */
#define CTRL_ACTION_PRIMARY         (1 << 12)   /* A — confirm / select / jump */
#define CTRL_ACTION_BACK            (1 << 13)   /* B — cancel / back / dodge   */
#define CTRL_ACTION_SECONDARY       (1 << 14)   /* X — secondary action / reload*/
#define CTRL_ACTION_TERTIARY        (1 << 15)   /* Y — tertiary / interact     */

/* Aliases for gaming context */
#define CTRL_A                      CTRL_ACTION_PRIMARY
#define CTRL_B                      CTRL_ACTION_BACK
#define CTRL_X                      CTRL_ACTION_SECONDARY
#define CTRL_Y                      CTRL_ACTION_TERTIARY
#define CTRL_LB                     CTRL_BUMPER_LEFT
#define CTRL_RB                     CTRL_BUMPER_RIGHT
#define CTRL_START                  CTRL_MENU
#define CTRL_SELECT                 CTRL_BACK

/* =============================================================================
 * CONTROLLER PORTS — semantic names
 * ============================================================================= */

#define CTRL_PORT_NATHAN            0   /* Port 1 — Nathan, standard profile   */
#define CTRL_PORT_LANDON            1   /* Port 2 — Landon, ALWAYS accessibility*/
#define CTRL_PORT_GUEST_3           2   /* Port 3 — Guest player               */
#define CTRL_PORT_GUEST_4           3   /* Port 4 — Guest player               */
#define CTRL_PORT_COUNT             4   /* Total controller slots              */

/* =============================================================================
 * LANDON'S ACCESSIBILITY PROFILE — named constants
 * These are the tuned values for cerebral palsy accessibility.
 * Change them here and they change everywhere.
 * ============================================================================= */

/* Dead zones — larger than standard to absorb tremor */
#define LANDON_DEAD_ZONE_LEFT       6553    /* ~20% of axis range              */
#define LANDON_DEAD_ZONE_RIGHT      6553    /* ~20% of axis range              */
#define STANDARD_DEAD_ZONE          4096    /* ~12% — standard for other ports */

/* Sensitivity — 0.75x to prevent overshooting (fixed-point: 256 = 1.0x) */
#define LANDON_SENSITIVITY          192     /* 0.75x                           */
#define STANDARD_SENSITIVITY        256     /* 1.0x                            */

/* Trigger threshold — very light press (Landon may have reduced grip) */
#define LANDON_TRIGGER_THRESHOLD    15      /* 15/255 — feather touch          */
#define STANDARD_TRIGGER_THRESHOLD  30      /* 30/255 — standard               */

/* Turbo — A button auto-repeats so Landon does not need to mash */
#define LANDON_TURBO_BUTTONS        CTRL_A  /* A button gets turbo             */
#define LANDON_TURBO_RATE_MS        120     /* Fires every 120ms when held     */
#define STANDARD_TURBO_RATE_MS      150     /* Standard turbo rate             */

/* =============================================================================
 * KEYBOARD KEYS — semantic names
 * PS/2 Scan Code Set 1. Named by what they do in BUBO OS.
 * ============================================================================= */

/* The BUBO Key — Home key summons the companion */
#define KEY_BUBO                    0x47    /* Home key = BUBO key             */

/* Navigation */
#define KEY_NAV_END                 0x4F    /* End                             */
#define KEY_NAV_PAGE_UP             0x49    /* Page Up                         */
#define KEY_NAV_PAGE_DOWN           0x51    /* Page Down                       */
#define KEY_NAV_INSERT              0x52    /* Insert                          */
#define KEY_NAV_DELETE              0x53    /* Delete                          */
#define KEY_NAV_UP                  0x48    /* Arrow Up                        */
#define KEY_NAV_DOWN                0x50    /* Arrow Down                      */
#define KEY_NAV_LEFT                0x4B    /* Arrow Left                      */
#define KEY_NAV_RIGHT               0x4D    /* Arrow Right                     */

/* Function keys */
#define KEY_F1                      0x3B    /* F1  — help                      */
#define KEY_F2                      0x3C    /* F2  — rename                    */
#define KEY_F3                      0x3D    /* F3  — search                    */
#define KEY_F4                      0x3E    /* F4  — properties                */
#define KEY_F5                      0x3F    /* F5  — refresh                   */
#define KEY_F6                      0x40    /* F6  — focus bar                 */
#define KEY_F7                      0x41    /* F7  — spell check               */
#define KEY_F8                      0x42    /* F8  — extended mode             */
#define KEY_F9                      0x43    /* F9  — send/execute              */
#define KEY_F10                     0x44    /* F10 — menu bar                  */
#define KEY_F11                     0x57    /* F11 — fullscreen                */
#define KEY_F12                     0x58    /* F12 — developer / debug         */

/* Modifiers */
#define KEY_MOD_SHIFT_LEFT          0x2A
#define KEY_MOD_SHIFT_RIGHT         0x36
#define KEY_MOD_CTRL                0x1D
#define KEY_MOD_ALT                 0x38
#define KEY_MOD_CAPS_LOCK           0x3A
#define KEY_MOD_NUM_LOCK            0x45

/* Common keys */
#define KEY_ESCAPE                  0x01    /* Escape — cancel / close         */
#define KEY_BACKSPACE               0x0E    /* Backspace — delete back         */
#define KEY_TAB                     0x0F    /* Tab — next field / indent       */
#define KEY_ENTER                   0x1C    /* Enter — confirm                 */
#define KEY_SPACE                   0x39    /* Space — select / jump / pause   */

/* Key flags */
#define KEY_FLAG_RELEASE            0x80    /* Bit 7 set = key released        */
#define KEY_FLAG_EXTENDED           0xE0    /* Extended key prefix byte        */

/* =============================================================================
 * BUBO KEY SHORTCUTS — Home + key combinations
 * The BUBO key (Home) is the system key. No Ctrl+Alt+Del.
 * Press Home alone = summon companion.
 * Hold Home + letter = shortcut.
 * ============================================================================= */

/* Shortcut IDs — what the arbiter dispatches */
#define BUBO_SHORTCUT_NONE          0x00    /* No shortcut                     */
#define BUBO_SHORTCUT_SUMMON        0x01    /* Home alone    — summon BUBO     */
#define BUBO_SHORTCUT_VOICE         0x02    /* Home + V      — voice input     */
#define BUBO_SHORTCUT_GAZE          0x03    /* Home + G      — eye gaze        */
#define BUBO_SHORTCUT_DASHBOARD     0x04    /* Home + D      — dashboard       */
#define BUBO_SHORTCUT_GAMING        0x05    /* Home + X      — gaming mode     */
#define BUBO_SHORTCUT_ACCESSIBILITY 0x06    /* Home + A      — a11y panel      */
#define BUBO_SHORTCUT_SCREENSHOT    0x07    /* Home + S      — screenshot      */
#define BUBO_SHORTCUT_CLOSE         0x08    /* Home + W      — close window    */
#define BUBO_SHORTCUT_FULLSCREEN    0x09    /* Home + F      — fullscreen      */
#define BUBO_SHORTCUT_VOLUME_UP     0x0A    /* Home + Up     — volume up       */
#define BUBO_SHORTCUT_VOLUME_DOWN   0x0B    /* Home + Down   — volume down     */
#define BUBO_SHORTCUT_MUTE          0x0C    /* Home + M      — mute            */
#define BUBO_SHORTCUT_REBOOT        0x0D    /* Home + R      — reboot (safe)   */
#define BUBO_SHORTCUT_SHUTDOWN      0x0E    /* Home + Q      — shutdown (safe) */

/* =============================================================================
 * VOICE INTENT CODES — what the dysarthria engine resolves to
 * These are the same intent codes the gaze and gesture systems also produce.
 * All five input faces resolve to the same intent vocabulary.
 * ============================================================================= */

#define INTENT_NONE                 0x00
#define INTENT_SELECT               0x01    /* "select" / look at item / tap   */
#define INTENT_CONFIRM              0x02    /* "yes" / blink / A button        */
#define INTENT_CANCEL               0x03    /* "no" / look away / B button     */
#define INTENT_NAVIGATE_UP          0x04    /* "up" / gaze up / dpad up        */
#define INTENT_NAVIGATE_DOWN        0x05    /* "down" / gaze down / dpad down  */
#define INTENT_NAVIGATE_LEFT        0x06    /* "left" / gaze left / dpad left  */
#define INTENT_NAVIGATE_RIGHT       0x07    /* "right" / gaze right / dpad right*/
#define INTENT_SUMMON_BUBO          0x08    /* "hey" / Home key / guide button */
#define INTENT_VOICE_ON             0x09    /* "listen" / Home+V               */
#define INTENT_GAZE_ON              0x0A    /* "eyes" / Home+G                 */
#define INTENT_DASHBOARD            0x0B    /* "home" / Home+D                 */
#define INTENT_GAMING               0x0C    /* "game" / Home+X                 */
#define INTENT_ACCESSIBILITY        0x0D    /* "settings" / Home+A             */
#define INTENT_SCREENSHOT           0x0E    /* "screenshot" / Home+S           */
#define INTENT_CLOSE                0x0F    /* "close" / Home+W                */
#define INTENT_FULLSCREEN           0x10    /* "fullscreen" / Home+F           */
#define INTENT_VOLUME_UP            0x11    /* "louder" / Home+Up              */
#define INTENT_VOLUME_DOWN          0x12    /* "quieter" / Home+Down           */
#define INTENT_MUTE                 0x13    /* "mute" / Home+M                 */
#define INTENT_SCROLL_UP            0x14    /* "scroll up" / gaze scroll up    */
#define INTENT_SCROLL_DOWN          0x15    /* "scroll down" / gaze scroll down*/
#define INTENT_DRAG_START           0x16    /* dwell 300ms on target           */
#define INTENT_DRAG_END             0x17    /* blink to drop / release         */
#define INTENT_SWIPE_LEFT           0x18    /* hand swipe left                 */
#define INTENT_SWIPE_RIGHT          0x19    /* hand swipe right                */
#define INTENT_GRAB                 0x1A    /* hand close / grab gesture       */
#define INTENT_PUSH                 0x1B    /* hand push / dismiss gesture     */

/* =============================================================================
 * GAZE / DWELL PARAMETERS — Rinnegan eye tracking
 * ============================================================================= */

#define GAZE_DWELL_DRAG_MS          300     /* Hold gaze 300ms to start drag   */
#define GAZE_DWELL_SELECT_MS        500     /* Hold gaze 500ms to select       */
#define GAZE_BLINK_DROP_MS          150     /* Blink duration to drop drag     */
#define GAZE_SMOOTHING_FRAMES       5       /* Frames to average for smoothing */
#define GAZE_DEAD_ZONE_PX           8       /* Pixel dead zone for small moves */

/* =============================================================================
 * GESTURE PARAMETERS — MediaPipe hand tracking
 * ============================================================================= */

#define GESTURE_SWIPE_MIN_PX        80      /* Min pixels to register a swipe  */
#define GESTURE_GRAB_THRESHOLD      0.7f    /* Finger curl ratio for grab      */
#define GESTURE_PUSH_Z_THRESHOLD    0.15f   /* Z-depth change for push         */
#define GESTURE_CONFIDENCE_MIN      0.75f   /* Min confidence to fire event    */

#endif /* BUBO_INPUT_MAP_H */
