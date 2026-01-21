#include "page/ir/learn_button.h"
#include "components/ui_theme.h"
#include "components/nav.h"

static lv_obj_t *ir_create_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    return label;
}

static lv_obj_t *ir_create_input_box(lv_obj_t *parent, const char *placeholder, bool dropdown)
{
    lv_obj_t *obj = dropdown ? lv_dropdown_create(parent) : lv_textarea_create(parent);
    lv_obj_set_width(obj, LV_PCT(100));
    lv_obj_set_height(obj, 40);
    lv_obj_set_style_bg_color(obj, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_text_color(obj, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_pad_left(obj, 10, 0);

    if (dropdown) {
        lv_dropdown_set_text(obj, placeholder);
        lv_obj_set_style_text_color(obj, ZV_COLOR_TEXT_MAIN, 0);
    } else {
        lv_textarea_set_placeholder_text(obj, placeholder);
        lv_obj_set_style_text_color(obj, ZV_COLOR_TEXT_MUTED, LV_PART_TEXTAREA_PLACEHOLDER);
    }

    zv_nav_add(obj);
    return obj;
}

static lv_obj_t *ir_create_status_box(lv_obj_t *parent, const char *text)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, LV_PCT(100));
    lv_obj_set_height(panel, 110);
    lv_obj_set_style_bg_color(panel, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_border_color(panel, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 10, 0);

    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));

    return panel;
}

lv_obj_t *ir_learn_button_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Learn Button");
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
    ir_create_input_box(root, "TV Samsung", true);

    ir_create_section_label(root, "Button Name:");
    ir_create_input_box(root, "KEY_VOLUMEUP", false);

    lv_obj_t *icon = lv_label_create(root);
    lv_label_set_text(icon, LV_SYMBOL_GPS);
    lv_obj_set_style_text_color(icon, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *hint = lv_label_create(root);
    lv_label_set_text(hint, "Press the button now...");
    lv_obj_set_style_text_color(hint, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, LV_PCT(100));

    ir_create_status_box(root,
        "Recording signal...\n"
        "Signal detected!\n"
        "Signal detected!\n"
        "Signal detected!");

    lv_obj_t *footer_row = lv_obj_create(root);
    lv_obj_set_size(footer_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(footer_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer_row, 0, 0);
    lv_obj_set_style_pad_all(footer_row, 0, 0);
    lv_obj_set_layout(footer_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *cancel_btn = lv_btn_create(footer_row);
    lv_obj_set_size(cancel_btn, LV_PCT(45), 40);
    lv_obj_set_style_bg_color(cancel_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cancel_btn, 2, 0);
    lv_obj_set_style_border_color(cancel_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(cancel_btn, 12, 0);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_set_style_text_color(cancel_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(cancel_label);
    zv_nav_add(cancel_btn);

    lv_obj_t *finish_btn = lv_btn_create(footer_row);
    lv_obj_set_size(finish_btn, LV_PCT(45), 40);
    lv_obj_set_style_bg_color(finish_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(finish_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(finish_btn, 2, 0);
    lv_obj_set_style_border_color(finish_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(finish_btn, 12, 0);

    lv_obj_t *finish_label = lv_label_create(finish_btn);
    lv_label_set_text(finish_label, "Finish");
    lv_obj_set_style_text_color(finish_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(finish_label);
    zv_nav_add(finish_btn);

    return page;
}
