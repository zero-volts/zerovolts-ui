#include "page/ir/send_signal.h"
#include "components/ui_theme.h"
#include "components/nav.h"

static lv_obj_t *ir_create_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    return label;
}

static lv_obj_t *ir_create_dropdown_box(lv_obj_t *parent, const char *text)
{
    lv_obj_t *obj = lv_dropdown_create(parent);
    lv_obj_set_width(obj, LV_PCT(100));
    lv_obj_set_height(obj, 40);
    lv_obj_set_style_bg_color(obj, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_text_color(obj, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_pad_left(obj, 10, 0);
    lv_dropdown_set_text(obj, text);
    zv_nav_add(obj);

    return obj;
}

static lv_obj_t *ir_create_key_button(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 96, 70);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(btn, 12, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_center(label);
    zv_nav_add(btn);

    return btn;
}

lv_obj_t *ir_send_signal_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Send Signal");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);

    ir_create_section_label(root, "Remote:");
    ir_create_dropdown_box(root, "TV Samsung");

    ir_create_section_label(root, "Button:");
    ir_create_dropdown_box(root, "KEY_VOLUMEUP");

    lv_obj_t *grid = lv_obj_create(root);
    lv_obj_set_size(grid, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_set_style_pad_column(grid, 10, 0);

    ir_create_key_button(grid, "KEY\nPOWER");
    ir_create_key_button(grid, "KEY\nVOLUMEUP");
    ir_create_key_button(grid, "KEY\nVOLUMEDOWN");
    ir_create_key_button(grid, "KEY\nMUTE");
    ir_create_key_button(grid, "KEY\nMENU");
    ir_create_key_button(grid, "KEY\nSOURCE");

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    lv_obj_t *send_btn = lv_btn_create(root);
    lv_obj_set_size(send_btn, LV_PCT(100), 44);
    lv_obj_set_style_bg_color(send_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(send_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(send_btn, 2, 0);
    lv_obj_set_style_border_color(send_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(send_btn, 12, 0);

    lv_obj_t *send_label = lv_label_create(send_btn);
    lv_label_set_text(send_label, "Send");
    lv_obj_set_style_text_color(send_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(send_label);
    zv_nav_add(send_btn);

    return page;
}
