#include "page/ir/learn_button.h"
#include "components/component_helper.h"
#include "components/ui_theme.h"
#include "components/nav.h"
#include "config.h"
#include "utils/logger.h"
#include "page/ir/ir_controller.h"
#include "utils/string_utils.h"
#include "components/button/ui_button.h"
#include "components/dropdown/ui_dropdown.h"
#include "components/text_input/ui_text_input.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    ui_dropdown *dropdown_remotes;
    int remote_selected_position;
    ui_text_input *button_txt;
    lv_obj_t *status;
} learn_ui_t;

static learn_ui_t g_learn;

static lv_obj_t *ir_create_status_box(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, LV_PCT(100));
    lv_obj_set_height(panel, 60);
    lv_obj_set_style_bg_color(panel, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_border_color(panel, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_pad_all(panel, 10, 0);

    g_learn.status = lv_label_create(panel);
    lv_label_set_text(g_learn.status, "Ready to capture.");
    lv_obj_set_style_text_color(g_learn.status, ZV_COLOR_TEXT_MAIN, 0);
    lv_label_set_long_mode(g_learn.status, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_learn.status, LV_PCT(100));

    return panel;
}

static void learn_set_status(const char *txt)
{
    if (!g_learn.status)
        return;

    lv_label_set_text(g_learn.status, txt);
}

static void load_remote_dropdown(void)
{
    ir_remote_list remotes = {0};
    if (!g_learn.dropdown_remotes)
        return;
    
    log_debug("[IR][learn_ui] loading remotes...");

    if (ir_controller_list_remotes(&remotes) != IR_OK || remotes.count == 0)
    {
        log_warning("[IR][learn_ui] no remotes available");

        dropdown_clean_items(g_learn.dropdown_remotes);
        learn_set_status("No remotes available. Create one first.");
        ir_controller_free_remote_list(&remotes);

        return;
    }

    dropdown_clean_items(g_learn.dropdown_remotes);
    for (size_t i = 0; i < remotes.count; i++)
    {
        dropdown_item_t item = {
            .position = (int)i,
            .text = remotes.remotes[i].name
        };

        dropdown_add_item(g_learn.dropdown_remotes, &item);
    }

    dropdown_set_selected_item(g_learn.dropdown_remotes, 0);
    learn_set_status("Ready to capture.");
    log_debug("[IR][learn_ui] remotes loaded: %zu\n", remotes.count);

    ir_controller_free_remote_list(&remotes);
}

static void learn_refresh_remotes_cb(event_data_btn *e)
{
    (void)e;
    load_remote_dropdown();
}

static void finish_button_handler(event_data_btn *event)
{
    dropdown_item_t *selected_item = dropdown_get_item(g_learn.dropdown_remotes, g_learn.remote_selected_position);
    if (!selected_item)
        return;

    const char *button_name = ui_text_get_text(g_learn.button_txt);
    if (zv_is_empty(selected_item->text))
    {
        learn_set_status("Select a remote first.");
        return;
    }

    // zv_trim_inplace(button_name);
    if (zv_is_empty(button_name))
    {
        learn_set_status("Enter button name (e.g. KEY_POWER).");
        return;
    }

    log_debug("[IR][learn_ui] finish pressed remote='%s' button='%s'\n", selected_item->text, button_name);

    keyboard_hide(g_learn.button_txt);

    learn_set_status("Recording signal... Press remote now.");
    lv_refr_now(NULL);

    ir_status_t rc = ir_controller_learn_button(selected_item->text, button_name);
    log_debug("[IR][learn_ui] learn result rc=%d err='%s'\n", (int)rc, ir_controller_last_error());
    
    if (rc == IR_OK)
    {
        learn_set_status("Signal captured and stored.");
        return;
    }

    learn_set_status(ir_controller_last_error());
}

static void learn_cancel_cb(event_data_btn *event)
{
    (void)event;
    if (!g_learn.button_txt)
        return;

    keyboard_hide(g_learn.button_txt);
    ui_text_clear(g_learn.button_txt);

    learn_set_status("Capture canceled.");
}

static void remote_on_change_handler(dropdown_item_t *item, void *user_data)
{
    g_learn.remote_selected_position = item->position;   
}

lv_obj_t *ir_learn_button_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Learn Button");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    memset(&g_learn, 0, sizeof(g_learn));

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);

    create_section_label(root, "Remote:");
    lv_obj_t *remote_list_container = create_transparent_flex_row(root, LV_PCT(100), 45);

    g_learn.dropdown_remotes = dropdown_create(remote_list_container, LV_PCT(82), LV_SIZE_CONTENT);
    dropdown_set_on_change_cb(g_learn.dropdown_remotes, remote_on_change_handler, NULL);

    ui_button *refresh_btn = ui_icon_button_create(remote_list_container, 45, 35, LV_SYMBOL_REFRESH);
    ui_button_set_on_click(refresh_btn, learn_refresh_remotes_cb, NULL);
    
    
    create_section_label(root, "Button Name:");
    g_learn.button_txt = ui_text_create(root, LV_PCT(82), 40, "KEY_VOLUMEUP");

    lv_obj_t *hint = lv_label_create(root);
    lv_label_set_text(hint, "Point remote at receiver and press one button.");
    lv_obj_set_style_text_color(hint, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, LV_PCT(100));

    ir_create_status_box(root);

    lv_obj_t *footer_row = create_transparent_flex_row(root, LV_PCT(100), 50);

    ui_button *cancel_btn = ui_button_create(footer_row,  LV_PCT(45), 40, "Cancel");
    ui_button_set_on_click(cancel_btn, learn_cancel_cb, NULL);

    ui_button *finish_btn = ui_button_create(footer_row, LV_PCT(45), 40, "Finish");
    ui_button_set_on_click(finish_btn, finish_button_handler, NULL);

    load_remote_dropdown();

    return page;
}
