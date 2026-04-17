#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

// Backgrounds
#define ZV_COLOR_BG_MAIN            lv_color_hex(0x0A0E13)
#define ZV_COLOR_BG_PANEL           lv_color_hex(0x0D1822)
#define ZV_COLOR_BG_CARD            lv_color_hex(0x111C28)
#define ZV_COLOR_BG_PRESSED         lv_color_hex(0x08101A)

// Borders
#define ZV_COLOR_BORDER             lv_color_hex(0x2A3340)
#define ZV_COLOR_BORDER_FOCUS       lv_color_hex(0x00E676)

#define ZV_COLOR_ACCENT             lv_color_hex(0x1E88E5)
#define ZV_COLOR_ACCENT_DIM         lv_color_hex(0x0A1830)

#define ZV_COLOR_TERMINAL           lv_color_hex(0x00E676)

// Text
#define ZV_COLOR_TEXT_MAIN          lv_color_hex(0xEDEDED)
#define ZV_COLOR_TEXT_MID           lv_color_hex(0xB0B0B0)
#define ZV_COLOR_TEXT_MUTED         lv_color_hex(0x7A8A9A)

// Status
#define ZV_COLOR_SUCCESS            lv_color_hex(0x00E676)
#define ZV_COLOR_WARNING            lv_color_hex(0xFFB300)
#define ZV_COLOR_ERROR              lv_color_hex(0xEF5350)

#define ZV_COLOR_WHITE              ZV_COLOR_TEXT_MAIN
#define ZV_COLOR_BLACK              ZV_COLOR_BG_MAIN
#define ZV_COLOR_BUTTON             ZV_COLOR_ACCENT
#define ZV_COLOR_BUTTON_TEXT        lv_color_hex(0xFFFFFF)
#define ZV_COLOR_BG_BUTTON_PRESSED  ZV_COLOR_BG_PRESSED

#endif /* UI_THEME_H */
