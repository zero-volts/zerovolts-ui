#include "page/ir/new_remote.h"

#include "components/nav.h"
#include "utils/logger.h"
#include "utils/string_utils.h"
#include "components/ui_theme.h"
#include "page/ir/ir_controller.h"
#include "components/component_helper.h"
#include "components/text_input/ui_text_input.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    ui_text_input *name_input_text;
    lv_obj_t *status_label;
} new_remote_ui_t;

static new_remote_ui_t g_new_remote;

// bool ir_new_remote_keyboard_is_visible(void)
// {
//     if (!g_new_remote.use_on_screen_keyboard || !g_new_remote.keyboard)
//         return false;

//     return !lv_obj_has_flag(g_new_remote.keyboard, LV_OBJ_FLAG_HIDDEN);
// }

static lv_obj_t *ir_create_chip_button(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 70, 34);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(btn, 10, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(label);

    return btn;
}

static void ir_create_remote_file(new_remote_ui_t *ui)
{
    ir_status_t rc;

    if (!ui || !ui->name_input_text || !ui->status_label)
        return;

    const char *name = ui_text_get_text(ui->name_input_text);
    if (!name || !name[0]) {
        lv_label_set_text(ui->status_label, "Enter a remote name first.");
        return;
    }

    char safe_name[256];
    if (!zv_sanitize_name(name, safe_name, sizeof(safe_name)) ||
        zv_has_whitespace(name)) {
        lv_label_set_text(ui->status_label, "Name cannot contain spaces.");
        return;
    }

    rc = ir_controller_create_remote(safe_name);
    if (rc == IR_OK) {
        lv_label_set_text(ui->status_label, "Remote created.");
        ui_text_clear(ui->name_input_text);
        return;
    }

    lv_label_set_text_fmt(ui->status_label, "Error: %s", ir_controller_last_error());
}

static void ir_create_btn_cb(lv_event_t *e)
{
    new_remote_ui_t *ui = (new_remote_ui_t *)lv_event_get_user_data(e);
    if (!ui) 
        return;

    ir_create_remote_file(ui);
}

lv_obj_t *ir_new_remote_page_create(lv_obj_t *menu)
{
    memset(&g_new_remote, 0, sizeof(g_new_remote));

    lv_obj_t *page = lv_menu_page_create(menu, "New Remote");
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

    create_section_label(root, "Remote Name:");

    // TODO: no olvidar liberar exte objeto cuando se destruya (en algun momento) la pantalla!
    g_new_remote.name_input_text = ui_text_create(root, LV_PCT(100), 60, "Enter remote name");

    create_section_label(root, "Category:");

    lv_obj_t *category_row = lv_obj_create(root);
    lv_obj_set_size(category_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(category_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(category_row, 0, 0);
    lv_obj_set_style_pad_all(category_row, 0, 0);
    lv_obj_set_layout(category_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(category_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(category_row, 10, 0);

    // TODO: ocupar control ya creado "ui_pills"
    ir_create_chip_button(category_row, "TV");
    ir_create_chip_button(category_row, "AC");
    ir_create_chip_button(category_row, "Audio");
    ir_create_chip_button(category_row, "Custom");

    create_section_label(root, "Icon:");

    lv_obj_t *icon_row = lv_obj_create(root);
    lv_obj_set_size(icon_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(icon_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_row, 0, 0);
    lv_obj_set_style_pad_all(icon_row, 0, 0);
    lv_obj_set_layout(icon_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(icon_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(icon_row, 10, 0);

    create_icon_button(icon_row, LV_SYMBOL_VIDEO, 68, 68, NULL, NULL);
    create_icon_button(icon_row, LV_SYMBOL_REFRESH, 68, 68, NULL, NULL);
    create_icon_button(icon_row, LV_SYMBOL_AUDIO, 68, 68, NULL, NULL);
    create_icon_button(icon_row, LV_SYMBOL_SETTINGS, 68, 68, NULL, NULL);

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    lv_obj_t *footer_row = create_transparent_flex_row(root, LV_PCT(100), LV_SIZE_CONTENT);

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

    lv_obj_t *create_btn = lv_btn_create(footer_row);
    lv_obj_set_size(create_btn, LV_PCT(45), 40);
    lv_obj_set_style_bg_color(create_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(create_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(create_btn, 2, 0);
    lv_obj_set_style_border_color(create_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(create_btn, 12, 0);
    lv_obj_add_event_cb(create_btn, ir_create_btn_cb, LV_EVENT_CLICKED, &g_new_remote);

    lv_obj_t *create_label = lv_label_create(create_btn);
    lv_label_set_text(create_label, "Create");
    lv_obj_set_style_text_color(create_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(create_label);

    g_new_remote.status_label = lv_label_create(root);
    lv_label_set_text(g_new_remote.status_label, "");
    lv_obj_set_style_text_color(g_new_remote.status_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_width(g_new_remote.status_label, LV_PCT(100));

    return page;
}
