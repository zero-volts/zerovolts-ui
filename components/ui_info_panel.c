#include "ui_info_panel.h"
#include "components/ui_theme.h"

#include <stdlib.h>

struct ui_info_panel {
    lv_obj_t *main_layout;
    int item_count;
};

static void apply_divider_to_prev(ui_info_panel *panel)
{
    if (panel->item_count <= 0)
        return;

    lv_obj_t *prev = lv_obj_get_child(panel->main_layout, panel->item_count - 1);
    if (!prev)
        return;

    lv_obj_set_style_border_width(prev, 1, 0);
    lv_obj_set_style_border_color(prev, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_border_side(prev, LV_BORDER_SIDE_BOTTOM, 0);
}

static lv_obj_t *create_info_panel_row_base(ui_info_panel *panel, const char *label)
{
    if (!panel || !panel->main_layout)
        return NULL;

    apply_divider_to_prev(panel);

    lv_obj_t *row = lv_obj_create(panel->main_layout);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_ver(row, 4, 0);
    lv_obj_set_style_pad_hor(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        row,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t *key = lv_label_create(row);
    lv_label_set_text(key, label ? label : "");
    lv_obj_set_style_text_color(key, ZV_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(key, &lv_font_montserrat_12, 0);

    panel->item_count += 1;

    return row;
}

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
    lv_obj_t *row = create_info_panel_row_base(panel, item.label);
    if (!row)
        return;

    lv_obj_set_height(row, 40);
    lv_obj_t *value = lv_label_create(row);
    lv_label_set_text(value, item.value ? item.value : "");
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(value, &lv_font_montserrat_12, 0);

    if (item.has_value_color)
        lv_obj_set_style_text_color(value, item.value_color, 0);
    else
        lv_obj_set_style_text_color(value, ZV_COLOR_TEXT_MAIN, 0);
}

void add_info_panel_header(ui_info_panel *panel, const char *text, lv_color_t color)
{
    if (!panel || !panel->main_layout || !text)
        return;

    apply_divider_to_prev(panel);

    lv_obj_t *header = lv_obj_create(panel->main_layout);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_ver(header, 6, 0);
    lv_obj_set_style_pad_hor(header, 4, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lb = lv_label_create(header);
    lv_label_set_text(lb, text);
    lv_obj_set_style_text_color(lb, color, 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_12, 0);

    panel->item_count += 1;
}

lv_obj_t *add_info_panel_custom_row(ui_info_panel *panel, const char *label)
{
    lv_obj_t *row = create_info_panel_row_base(panel, label);
    if (!row)
        return NULL;

    lv_obj_t *right = lv_obj_create(row);
    lv_obj_remove_style_all(right);

    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_set_width(right, LV_PCT(55));
    lv_obj_set_height(right, LV_SIZE_CONTENT);

    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_column(right, 4, 0);
    lv_obj_set_style_pad_row(right, 4, 0);

    lv_obj_set_flex_align(
        right,
        LV_FLEX_ALIGN_END,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    return right;
}

void clear_info_panel(ui_info_panel *panel)
{
    if (!panel || !panel->main_layout)
        return;

    lv_obj_clean(panel->main_layout);
    panel->item_count = 0;
}
