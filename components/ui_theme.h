#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

/* =========================
 * Base
 * ========================= */
#define ZV_COLOR_BLACK        lv_color_hex(0x0B0F14)
#define ZV_COLOR_WHITE        lv_color_hex(0xEDEDED)

/* =========================
 * Zero-Volts Brand
 * ========================= */
#define ZV_COLOR_TERMINAL     lv_color_hex(0x00E676)
#define ZV_COLOR_BG_MAIN      lv_color_hex(0x0B0F14)
#define ZV_COLOR_BG_PANEL     lv_color_hex(0x0D1B2A)

/* =========================
 * Interacción
 * ========================= */
#define ZV_COLOR_BUTTON       lv_color_hex(0x1E88E5)
#define ZV_COLOR_BUTTON_TEXT  ZV_COLOR_WHITE
#define ZV_COLOR_BG_BUTTON_PRESSED     lv_color_hex(0x101826)

/* =========================
 * Texto
 * ========================= */
#define ZV_COLOR_TEXT_MAIN    ZV_COLOR_WHITE
#define ZV_COLOR_TEXT_MUTED   lv_color_hex(0x9E9E9E)

/* =========================
 * Estados
 * ========================= */
#define ZV_COLOR_SUCCESS      ZV_COLOR_TERMINAL
#define ZV_COLOR_WARNING      lv_color_hex(0xFFC107)
#define ZV_COLOR_ERROR        lv_color_hex(0xE53935)

#define ZV_COLOR_BORDER      lv_color_hex(0x2A3340)

#endif /* UI_THEME_H */
