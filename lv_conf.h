/**
 * lv_conf.h — BUBO OS LVGL Configuration
 * ─────────────────────────────────────────────────────────────────────────────
 * Tuned for bare-metal x86-64 freestanding build.
 * No libc. No malloc. LVGL built-in memory pool only.
 * 32-bit XRGB8888 color depth to match the VESA framebuffer.
 *
 * NO MAS DISADVANTAGED
 * ─────────────────────────────────────────────────────────────────────────────
 */

/* clang-format off */
#if 1   /* enabled */
#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
/** 32-bit XRGB8888 — matches BUBO OS VESA framebuffer */
#define LV_COLOR_DEPTH 32

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/
/** Use LVGL's built-in malloc — no libc required */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

/*====================
   MEMORY SETTINGS
 *====================*/
/** 512 KB internal heap for LVGL widgets and buffers */
#define LV_MEM_SIZE             (512U * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
/** Default display refresh period in milliseconds */
#define LV_DEF_REFR_PERIOD      16   /* ~60 fps */

/** Dot-per-inch of the display — used for scaling */
#define LV_DPI_DEF              130

/*====================
   FEATURE SETTINGS
 *====================*/
/** Enable the animation system */
#define LV_USE_ANIM             1

/** Enable shadow drawing */
#define LV_USE_SHADOW           1

/** Enable outline drawing */
#define LV_USE_OUTLINE          1

/** Enable pattern drawing */
#define LV_USE_PATTERN          0

/** Enable value string drawing */
#define LV_USE_VALUE_STR        0

/** Enable blend modes */
#define LV_USE_BLEND_MODES      1

/** Enable opacity */
#define LV_USE_OPA_SCALE        1

/** Enable image zoom and rotation */
#define LV_USE_IMG_TRANSFORM    1

/** Enable group (keyboard/encoder navigation) */
#define LV_USE_GROUP            1

/** Enable GPU (no GPU in bare metal — disabled) */
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_STM32_DMA2D  0

/*====================
   FONT SETTINGS
 *====================*/
/** Built-in fonts — keep small for bare-metal */
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   0
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   0
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   1

/** Default font */
#define LV_FONT_DEFAULT         &lv_font_montserrat_16

/** Enable built-in symbols */
#define LV_USE_FONT_SUBPX       1

/*====================
   WIDGET SETTINGS
 *====================*/
#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_BTNMATRIX        1
#define LV_USE_CANVAS           1
#define LV_USE_CHECKBOX         1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_LABEL            1
#define LV_USE_LINE             1
#define LV_USE_ROLLER           1
#define LV_USE_SLIDER           1
#define LV_USE_SWITCH           1
#define LV_USE_TEXTAREA         1
#define LV_USE_TABLE            1

/*====================
   THEME SETTINGS
 *====================*/
/** Enable the default theme — BUBO replaces with lv_theme_akatsuki */
#define LV_USE_THEME_DEFAULT    1
#define LV_USE_THEME_BASIC      1
#define LV_USE_THEME_MONO       0

/*====================
   LAYOUT SETTINGS
 *====================*/
#define LV_USE_FLEX             1
#define LV_USE_GRID             1

/*====================
   FREESTANDING HEADERS
   Redirect LVGL's libc header includes to our bare-metal stubs.
   Without this, -nostdinc causes fatal errors on inttypes.h etc.
 *====================*/
#define LV_STDINT_INCLUDE   <stdint.h>
#define LV_STDDEF_INCLUDE   <stddef.h>
#define LV_STDBOOL_INCLUDE  <stdbool.h>
/* Provide minimal stubs for headers LVGL wants but we cannot supply */
#define LV_INTTYPES_INCLUDE "../include/lv_inttypes_stub.h"
#define LV_LIMITS_INCLUDE   "../include/lv_limits_stub.h"
#define LV_STDARG_INCLUDE   "../include/lv_stdarg_stub.h"

/*====================
   LOGGING
 *====================*/
/** Disable logging in bare-metal (no printf) */
#define LV_USE_LOG              0

/*====================
   ASSERT
 *====================*/
#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#endif /* LV_CONF_H */
#endif /* #if 1 */
