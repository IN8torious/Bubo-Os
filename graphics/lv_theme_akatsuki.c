// BUBO OS — LVGL Akatsuki Theme
// Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
//
// Three colors. That's all the Akatsuki ever needed.
// Black. Crimson. Gold.
// =============================================================================

#include "lv_theme_akatsuki.h"
#include "../lvgl/lvgl.h"
#include "../lvgl/src/themes/lv_theme_private.h"  /* full lv_theme_t struct for LVGL v9 */

// ── Akatsuki Palette ──────────────────────────────────────────────────────────
#define AK_BLACK        lv_color_hex(0x0A0000)   // Near-black with red tint
#define AK_PANEL        lv_color_hex(0x150000)   // Card / panel background
#define AK_CRIMSON      lv_color_hex(0xCC0000)   // Primary crimson
#define AK_BLOOD        lv_color_hex(0x8B0000)   // Darker blood red
#define AK_GOLD         lv_color_hex(0xFFD700)   // Gold text and accents
#define AK_DIM          lv_color_hex(0x881111)   // Dimmed red for secondary text
#define AK_WHITE        lv_color_hex(0xFFFFFF)   // Pure white for critical alerts
#define AK_TRANSPARENT  lv_color_hex(0x000000)

// ── Static styles ─────────────────────────────────────────────────────────────
static lv_style_t style_screen;
static lv_style_t style_card;
static lv_style_t style_card_focused;
static lv_style_t style_btn;
static lv_style_t style_btn_pressed;
static lv_style_t style_label_title;
static lv_style_t style_label_body;
static lv_style_t style_label_gold;
static lv_style_t style_bar_bg;
static lv_style_t style_bar_indicator;
static lv_style_t style_bar_indicator_load;
static lv_style_t style_mandate_banner;
static lv_style_t style_taskbar;
static lv_style_t style_taskbar_btn;

static bool g_inited = false;

// ── Apply theme to objects ─────────────────────────────────────────────────────
static void theme_apply(lv_theme_t *th, lv_obj_t *obj)
{
    (void)th;
    lv_obj_class_t const *cls = lv_obj_get_class(obj);

    if (cls == &lv_obj_class) {
        // Screen
        if (lv_obj_get_parent(obj) == NULL) {
            lv_obj_add_style(obj, &style_screen, 0);
        } else {
            // Generic panel/card
            lv_obj_add_style(obj, &style_card, 0);
            lv_obj_add_style(obj, &style_card_focused, LV_STATE_FOCUSED);
        }
    } else if (cls == &lv_label_class) {
        lv_obj_add_style(obj, &style_label_body, 0);
    } else if (cls == &lv_button_class) {  /* LVGL v9: renamed from lv_btn_class */
        lv_obj_add_style(obj, &style_btn, 0);
        lv_obj_add_style(obj, &style_btn_pressed, LV_STATE_PRESSED);
    } else if (cls == &lv_bar_class) {
        lv_obj_add_style(obj, &style_bar_bg, LV_PART_MAIN);
        lv_obj_add_style(obj, &style_bar_indicator, LV_PART_INDICATOR);
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────
static lv_theme_t g_theme;

lv_theme_t *lv_theme_akatsuki_init(lv_display_t *disp)
{
    if (g_inited) return &g_theme;
    g_inited = true;

    // Screen
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, AK_BLACK);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen, AK_DIM);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);

    // Card / Panel
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, AK_PANEL);
    lv_style_set_bg_opa(&style_card, LV_OPA_90);
    lv_style_set_border_color(&style_card, AK_CRIMSON);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, 6);
    lv_style_set_shadow_color(&style_card, AK_CRIMSON);
    lv_style_set_shadow_width(&style_card, 8);
    lv_style_set_shadow_opa(&style_card, LV_OPA_40);
    lv_style_set_text_color(&style_card, AK_DIM);
    lv_style_set_pad_all(&style_card, 8);

    // Card focused
    lv_style_init(&style_card_focused);
    lv_style_set_border_color(&style_card_focused, AK_GOLD);
    lv_style_set_shadow_color(&style_card_focused, AK_GOLD);
    lv_style_set_shadow_opa(&style_card_focused, LV_OPA_60);

    // Button
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, AK_BLOOD);
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_color(&style_btn, AK_CRIMSON);
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_radius(&style_btn, 20);  // Pill shape
    lv_style_set_text_color(&style_btn, AK_GOLD);
    lv_style_set_shadow_color(&style_btn, AK_CRIMSON);
    lv_style_set_shadow_width(&style_btn, 10);
    lv_style_set_shadow_opa(&style_btn, LV_OPA_50);
    lv_style_set_pad_hor(&style_btn, 16);
    lv_style_set_pad_ver(&style_btn, 8);

    // Button pressed
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, AK_CRIMSON);
    lv_style_set_shadow_opa(&style_btn_pressed, LV_OPA_80);

    // Label — body text
    lv_style_init(&style_label_body);
    lv_style_set_text_color(&style_label_body, AK_DIM);

    // Label — gold (for titles, names)
    lv_style_init(&style_label_gold);
    lv_style_set_text_color(&style_label_gold, AK_GOLD);

    // Label — title
    lv_style_init(&style_label_title);
    lv_style_set_text_color(&style_label_title, AK_CRIMSON);

    // Bar background
    lv_style_init(&style_bar_bg);
    lv_style_set_bg_color(&style_bar_bg, AK_BLACK);
    lv_style_set_border_color(&style_bar_bg, AK_BLOOD);
    lv_style_set_border_width(&style_bar_bg, 1);
    lv_style_set_radius(&style_bar_bg, 2);
    lv_style_set_pad_all(&style_bar_bg, 0);

    // Bar indicator — HP (crimson)
    lv_style_init(&style_bar_indicator);
    lv_style_set_bg_color(&style_bar_indicator, AK_CRIMSON);
    lv_style_set_bg_opa(&style_bar_indicator, LV_OPA_COVER);
    lv_style_set_radius(&style_bar_indicator, 2);

    // Bar indicator — Load (blood red)
    lv_style_init(&style_bar_indicator_load);
    lv_style_set_bg_color(&style_bar_indicator_load, AK_BLOOD);
    lv_style_set_bg_opa(&style_bar_indicator_load, LV_OPA_COVER);
    lv_style_set_radius(&style_bar_indicator_load, 2);

    // Mandate banner
    lv_style_init(&style_mandate_banner);
    lv_style_set_bg_color(&style_mandate_banner, AK_CRIMSON);
    lv_style_set_bg_opa(&style_mandate_banner, LV_OPA_COVER);
    lv_style_set_border_color(&style_mandate_banner, AK_GOLD);
    lv_style_set_border_width(&style_mandate_banner, 1);
    lv_style_set_radius(&style_mandate_banner, 4);
    lv_style_set_text_color(&style_mandate_banner, AK_GOLD);
    lv_style_set_text_align(&style_mandate_banner, LV_TEXT_ALIGN_CENTER);

    // Taskbar
    lv_style_init(&style_taskbar);
    lv_style_set_bg_color(&style_taskbar, AK_PANEL);
    lv_style_set_bg_opa(&style_taskbar, LV_OPA_90);
    lv_style_set_border_color(&style_taskbar, AK_BLOOD);
    lv_style_set_border_width(&style_taskbar, 1);
    lv_style_set_radius(&style_taskbar, 30);
    lv_style_set_shadow_color(&style_taskbar, AK_CRIMSON);
    lv_style_set_shadow_width(&style_taskbar, 12);
    lv_style_set_shadow_opa(&style_taskbar, LV_OPA_30);
    lv_style_set_pad_all(&style_taskbar, 8);

    // Init theme struct
    lv_theme_init(&g_theme, NULL, NULL, theme_apply, lv_display_get_dpi(disp));
    lv_display_set_theme(disp, &g_theme);

    return &g_theme;
}

// ── Helper: apply gold label style ───────────────────────────────────────────
void lv_label_set_akatsuki_gold(lv_obj_t *label)
{
    lv_obj_add_style(label, &style_label_gold, 0);
}

// ── Helper: apply title style ─────────────────────────────────────────────────
void lv_label_set_akatsuki_title(lv_obj_t *label)
{
    lv_obj_add_style(label, &style_label_title, 0);
}

// ── Helper: apply mandate banner style ───────────────────────────────────────
void lv_obj_set_akatsuki_mandate(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_mandate_banner, 0);
}

// ── Helper: apply load bar style ─────────────────────────────────────────────
void lv_bar_set_akatsuki_load(lv_obj_t *bar)
{
    lv_obj_add_style(bar, &style_bar_indicator_load, LV_PART_INDICATOR);
}
