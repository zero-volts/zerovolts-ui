#include "page/ir/remotes.h"
#include "components/ui_theme.h"

static lv_obj_t *ir_create_remote_card(lv_obj_t *parent, const char *icon, const char *title, const char *subtitle)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 70);
    lv_obj_set_style_bg_color(card, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(card, 12, 0);

    lv_obj_t *ic = lv_label_create(card);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *text_col = lv_obj_create(card);
    lv_obj_set_size(text_col, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(text_col, 0, 0);
    lv_obj_set_style_pad_all(text_col, 0, 0);
    lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(text_col, 6, 0);

    lv_obj_t *title_lbl = lv_label_create(text_col);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *subtitle_lbl = lv_label_create(text_col);
    lv_label_set_text(subtitle_lbl, subtitle);
    lv_obj_set_style_text_color(subtitle_lbl, ZV_COLOR_TEXT_MUTED, 0);

    return card;
}

lv_obj_t *ir_remotes_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "IR Remotes");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 12, 0);

    ir_create_remote_card(root, LV_SYMBOL_VIDEO, "TV Samsung", "5 buttons learned");
    ir_create_remote_card(root, LV_SYMBOL_REFRESH, "AC Midea", "8 buttons learned");
    ir_create_remote_card(root, LV_SYMBOL_AUDIO, "Soundbar Sony", "3 buttons learned");

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    lv_obj_t *learn_btn = lv_btn_create(root);
    lv_obj_set_size(learn_btn, LV_PCT(100), 44);
    lv_obj_set_style_bg_color(learn_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(learn_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(learn_btn, 2, 0);
    lv_obj_set_style_border_color(learn_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(learn_btn, 12, 0);

    lv_obj_t *learn_label = lv_label_create(learn_btn);
    lv_label_set_text(learn_label, LV_SYMBOL_GPS "  Learn Button");
    lv_obj_set_style_text_color(learn_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(learn_label);

    return page;
}
