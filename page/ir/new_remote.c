#include "page/ir/new_remote.h"

#include "config.h"
#include "utils/logger.h"
#include "page/navigation_page.h"
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

static void cancel_button_handler(event_data_btn *event)
{
    (void)event;
    zv_nav_pop();
}

static void view_delete_cb(lv_event_t *e)
{
    (void)e;
    memset(&g_new_remote, 0, sizeof(g_new_remote));
}

zv_view_t *ir_new_remote_page_create(const zv_config *cfg)
{
    memset(&g_new_remote, 0, sizeof(g_new_remote));

    zv_view_t *view = zv_view_new("New Remote");
    if (!view)
        return NULL;
    lv_obj_add_event_cb(view->main_container, view_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *root = view->content;
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);

    create_section_label(root, "Remote Name:");

    ui_text_input_options input_options = {
        .use_on_screen_keyboard = cfg ? cfg->ir.use_on_screen_keyboard : true,
        .one_line = true
    };

    g_new_remote.name_input_text = ui_text_create_with_options(root, LV_PCT(100), 60, "Enter remote name", &input_options);

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    lv_obj_t *footer_row = create_transparent_flex_row(root, LV_PCT(100), LV_SIZE_CONTENT);

    ui_button *cancel_btn = ui_button_create(footer_row, LV_PCT(45), 40, "Cancel");
    ui_button_set_on_click(cancel_btn, cancel_button_handler, NULL);
    ui_button *create_btn = ui_button_create(footer_row, LV_PCT(45), 40, "Create");
    ui_button_set_on_click(create_btn, create_button_handler, &g_new_remote);

    g_new_remote.status_label = lv_label_create(root);
    lv_label_set_text(g_new_remote.status_label, "");
    lv_obj_set_style_text_color(g_new_remote.status_label, COLOR_TEXT, 0);
    lv_obj_set_width(g_new_remote.status_label, LV_PCT(100));

    return view;
}
