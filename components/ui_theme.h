#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

// Backgrounds
#define ZV_COLOR_BG_MAIN            lv_color_hex(0x0C1117)
#define ZV_COLOR_BG_PANEL           lv_color_hex(0x151D26)
#define ZV_COLOR_BG_CARD            lv_color_hex(0x1A2430)
#define ZV_COLOR_BG_PRESSED         lv_color_hex(0x202E3B)

// Borders
#define ZV_COLOR_BORDER             lv_color_hex(0x334150)
#define ZV_COLOR_BORDER_FOCUS       lv_color_hex(0x56DCA1)

#define ZV_COLOR_ACCENT             lv_color_hex(0x8DBBD5)
#define ZV_COLOR_ACCENT_DIM         lv_color_hex(0x192D3B)

#define ZV_COLOR_TERMINAL           lv_color_hex(0x56DCA1)

// Text
#define ZV_COLOR_TEXT_MAIN          lv_color_hex(0xE9EFF4)
#define ZV_COLOR_TEXT_MID           lv_color_hex(0xBDC9D4)
#define ZV_COLOR_TEXT_MUTED         lv_color_hex(0x92A3B3)

// Status
#define ZV_COLOR_SUCCESS            lv_color_hex(0x56DCA1)
#define ZV_COLOR_WARNING            lv_color_hex(0xEFB56B)
#define ZV_COLOR_ERROR              lv_color_hex(0xFF8188)

#define ZV_COLOR_WHITE              ZV_COLOR_TEXT_MAIN
#define ZV_COLOR_BLACK              ZV_COLOR_BG_MAIN
#define ZV_COLOR_BUTTON             ZV_COLOR_ACCENT
#define ZV_COLOR_BUTTON_TEXT        lv_color_hex(0xFFFFFF)
#define ZV_COLOR_BG_BUTTON_PRESSED  ZV_COLOR_BG_PRESSED

#endif /* UI_THEME_H */
