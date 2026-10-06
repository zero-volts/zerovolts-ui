#include "page/ir/new_remote.h"

#include "components/nav.h"
#include "utils/logger.h"
#include "utils/string_utils.h"
#include "components/ui_theme.h"
#include "page/ir/ir_controller.h"
#include "components/component_helper.h"
#include "components/text_input/ui_text_input.h"
#include "components/button/ui_button.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    ui_text_input *name_input_text;
    lv_obj_t *status_label;
} new_remote_ui_t;

static new_remote_ui_t g_new_remote;

static void ir_create_remote_file(new_remote_ui_t *ui)
{
    ir_status_t rc;

    if (!ui || !ui->name_input_text || !ui->status_label)
        return;

    const char *name = ui_text_get_text(ui->name_input_text);
    if (!name || !name[0])
    {
        lv_label_set_text(ui->status_label, "Enter a remote name first.");
        return;
    }

    char safe_name[256];
    if (!zv_sanitize_name(name, safe_name, sizeof(safe_name)) ||
        zv_has_whitespace(name))
    {
        lv_label_set_text(ui->status_label, "Name cannot contain spaces.");
        return;
    }

    rc = ir_controller_create_remote(safe_name);
    if (rc == IR_OK)
    {
        lv_label_set_text(ui->status_label, "Remote created.");
        ui_text_clear(ui->name_input_text);
        return;
    }

    lv_label_set_text_fmt(ui->status_label, "Error: %s", ir_controller_last_error());
}

static void create_button_handler(event_data_btn *event)
{
    new_remote_ui_t *ui = (new_remote_ui_t *)event->user_data;
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

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    lv_obj_t *footer_row = create_transparent_flex_row(root, LV_PCT(100), LV_SIZE_CONTENT);

    ui_button_create(footer_row, LV_PCT(45), 40, "Cancel");
    ui_button *create_btn = ui_button_create(footer_row, LV_PCT(45), 40, "Create");
    ui_button_set_on_click(create_btn, create_button_handler, &g_new_remote);

    g_new_remote.status_label = lv_label_create(root);
    lv_label_set_text(g_new_remote.status_label, "");
    lv_obj_set_style_text_color(g_new_remote.status_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_width(g_new_remote.status_label, LV_PCT(100));

    return page;
}
