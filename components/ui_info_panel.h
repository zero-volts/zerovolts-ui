#ifndef UI_INFO_PANEL_H
#define UI_INFO_PANEL_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *label;
    const char *value;
    lv_color_t value_color;
    bool has_value_color;
} kv_item_t;

typedef struct ui_info_panel ui_info_panel;

ui_info_panel *create_info_panel(lv_obj_t *parent, int width, int height);
void add_info_panel_item(ui_info_panel *panel, kv_item_t item);
void add_info_panel_header(ui_info_panel *panel, const char *text, lv_color_t color);
lv_obj_t *add_info_panel_custom_row(ui_info_panel *panel, const char *label);

void clear_info_panel(ui_info_panel *panel);

#ifdef __cplusplus
}
#endif

#endif /* UI_INFO_PANEL_H */
