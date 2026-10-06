#include "ui_panel.h"
#include "components/ui_theme.h"

lv_obj_t *ui_panel_create(lv_obj_t *parent, int width, int height)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, width, height);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);

    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_border_color(panel, ZV_COLOR_BORDER, LV_PART_MAIN);

    lv_obj_set_style_pad_hor(panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(panel, 25, LV_PART_MAIN);

    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);

    lv_obj_set_flex_align(panel, 
        LV_FLEX_ALIGN_SPACE_BETWEEN, 
        LV_FLEX_ALIGN_CENTER, 
        LV_FLEX_ALIGN_CENTER
    );

    return panel;
}

void ui_panel_set_flex_flow(lv_obj_t *panel, lv_flex_flow_t new_flex_flow)
{
    lv_obj_set_flex_flow(panel, new_flex_flow);
}