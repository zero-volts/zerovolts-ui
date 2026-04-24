#include "ui_info_panel.h"
#include "components/ui_theme.h"

#include <stdlib.h>

struct ui_info_panel {
    lv_obj_t *main_layout;
    int item_count;
};

ui_info_panel *create_info_panel(lv_obj_t *parent, int width, int height)
{
    ui_info_panel *panel = (ui_info_panel *)malloc (sizeof (ui_info_panel));
    lv_obj_t *root = lv_obj_create(parent);
    panel->main_layout = root;

    lv_obj_set_width(root, width);
    lv_obj_set_height(root, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(root, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_border_color(root, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(root, 1, 0);
    lv_obj_set_style_radius(root, 10, 0);
    lv_obj_set_style_pad_all(root, 5, 0);
    lv_obj_set_style_pad_row(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    panel->item_count = 0;
    return panel;
}

void add_info_panel_item(ui_info_panel *panel, kv_item_t item)
{
    if (panel->item_count > 0) {
        lv_obj_t *prev = lv_obj_get_child(panel->main_layout, panel->item_count - 1);
        lv_obj_set_style_border_width(prev, 1, 0);
        lv_obj_set_style_border_color(prev, ZV_COLOR_BORDER, 0);
        lv_obj_set_style_border_side(prev, LV_BORDER_SIDE_BOTTOM, 0);
    }

    lv_obj_t *item_container = lv_obj_create(panel->main_layout);
    lv_obj_set_size(item_container, LV_PCT(100), 40);
    lv_obj_set_style_bg_opa(item_container, LV_OPA_TRANSP, 0);

    // el nuevo último item parte sin borde
    lv_obj_set_style_border_width(item_container, 0, 0);
    lv_obj_set_style_border_side(item_container, LV_BORDER_SIDE_BOTTOM, 0);

    lv_obj_clear_flag(item_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(item_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(item_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        item_container,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t *key = lv_label_create(item_container);
    lv_label_set_text(key, item.label);
    lv_obj_set_style_text_color(key, ZV_COLOR_TEXT_MUTED, 0);

    lv_obj_t *value = lv_label_create(item_container);
    lv_label_set_text(value, item.value);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);

    if (item.has_value_color)
        lv_obj_set_style_text_color(value, item.value_color, 0);
    else    
        lv_obj_set_style_text_color(value, ZV_COLOR_TEXT_MAIN, 0);

    panel->item_count += 1;
}

void clear_info_panel(ui_info_panel *panel)
{
    if (!panel || !panel->main_layout)
        return;

    lv_obj_clean(panel->main_layout);
    panel->item_count = 0;
}