#include "ir.h"
#include "components/ui_theme.h"
#include "components/component_helper.h"
#include "components/nav.h"
#include "page/ir/new_remote.h"
#include "page/ir/learn_button.h"
#include "page/ir/send_signal.h"
#include "page/ir/remotes.h"

static lv_obj_t *ir_simple_page_create(lv_obj_t *menu, const char *title, const char *text)
{
    lv_obj_t *page = lv_menu_page_create(menu, title);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);

    return page;
}

lv_obj_t *ir_page_create(lv_obj_t *menu, const zv_config *cfg)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Infrared");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF); 

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(root, 10, 0);

    (void)cfg;

    lv_obj_t *remotes_page = ir_remotes_page_create(menu);
    lv_obj_t *new_remote_page = ir_new_remote_page_create(menu);
    lv_obj_t *learn_button_page = ir_learn_button_page_create(menu);
    lv_obj_t *send_signal_page = ir_send_signal_page_create(menu);

    static nav_ctx_t nav_remotes;
    static nav_ctx_t nav_new_remote;
    static nav_ctx_t nav_learn_button;
    static nav_ctx_t nav_send_signal;

    nav_remotes.menu = menu;
    nav_remotes.page = remotes_page;

    nav_new_remote.menu = menu;
    nav_new_remote.page = new_remote_page;

    nav_learn_button.menu = menu;
    nav_learn_button.page = learn_button_page;

    nav_send_signal.menu = menu;
    nav_send_signal.page = send_signal_page;

    create_square_main_button(root, "Remotes", LV_SYMBOL_HOME, zv_goto_page_cb, &nav_remotes);
    create_square_main_button(root, "New Remote", LV_SYMBOL_PLUS, zv_goto_page_cb, &nav_new_remote);
    create_square_main_button(root, "Learn Button", LV_SYMBOL_SAVE, zv_goto_page_cb, &nav_learn_button);
    create_square_main_button(root, "Send Signal", LV_SYMBOL_GPS, zv_goto_page_cb, &nav_send_signal);


    return page;
}
