#include "send_signal.h"

#include "components/ui_theme.h"
#include "page/ir/ir_controller.h"
#include "components/ui_pills.h"
#include "components/ui_info_panel.h"
#include "components/button/ui_button.h"
#include "components/component_helper.h"
#include "components/dropdown/ui_dropdown.h"
#include "page/base_view.h"
#include "utils/string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    base_view base;
    ui_dropdown *dropdown_remotes;
    int remote_selected_position;

    lv_obj_t *grid;
    ui_pills *signal_state_pill;
} send_signal_ui_t;

typedef struct {
    char name[IR_MAX_NAME];
} send_grid_button_ctx_t;

static send_signal_ui_t g_send_ui;

static lv_obj_t *ir_create_key_button(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 90, 60);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(btn, 12, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_center(label);

    return btn;
}

static void send_signal_status(const char *msg, lv_color_t color)
{
    if (!g_send_ui.signal_state_pill)
        return;

    pills_update(g_send_ui.signal_state_pill, 0, msg, color);
}

static void send_button_by_name(const char *button_name)
{
    dropdown_item_t *dropdown_item = dropdown_get_item(g_send_ui.dropdown_remotes, g_send_ui.remote_selected_position);
    if (!dropdown_item)
        return;

    if (zv_is_empty(button_name) ) {
        send_signal_status("Select remote and button.", ZV_COLOR_WARNING);
        return;
    }

    ir_status_t rc = ir_controller_send_button(dropdown_item->text, button_name);
    if (rc == IR_OK) {
        send_signal_status("Signal sent.", ZV_COLOR_ACCENT);
        return;
    }

    send_signal_status(ir_controller_last_error(), ZV_COLOR_WARNING);
}

static void send_grid_button_cb(lv_event_t *e)
{
    send_grid_button_ctx_t *ctx = (send_grid_button_ctx_t *)lv_event_get_user_data(e);
    const char *btn_name = ctx ? ctx->name : NULL;

    if (!btn_name || !btn_name[0]) {
        printf("send_grid_button_cb...sin nombre\n");
        return;
    }

    send_button_by_name(btn_name);
}

static void send_grid_button_delete_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

static void clear_grid(void)
{
    if (g_send_ui.grid)
        lv_obj_clean(g_send_ui.grid);
}

static void rebuild_button_controls(const char *remote_name)
{
    clear_grid();
    ir_button_list buttons = {0};

    ir_status_t rc = ir_controller_list_buttons(remote_name, &buttons);
    if (rc != IR_OK)
    {    
        send_signal_status("No buttons for this remote.", ZV_COLOR_WARNING);
        return;
    }

    for (size_t i = 0; i < buttons.count; i++)
    {
        send_grid_button_ctx_t *ctx = (send_grid_button_ctx_t *)calloc(1, sizeof(*ctx));
        if (!ctx)
            continue;

        snprintf(ctx->name, sizeof(ctx->name), "%s", buttons.buttons[i].name);

        lv_obj_t *btn = ir_create_key_button(g_send_ui.grid, buttons.buttons[i].name);
        lv_obj_add_event_cb(btn, send_grid_button_cb, LV_EVENT_CLICKED, ctx);
        lv_obj_add_event_cb(btn, send_grid_button_delete_cb, LV_EVENT_DELETE, ctx);
    }

    if (buttons.count == 0)
        send_signal_status("No buttons learned yet.", ZV_COLOR_WARNING);
    else
        send_signal_status("IDLE", ZV_COLOR_TEXT_MUTED);

    ir_controller_free_button_list(&buttons);
}

static void remote_dropdown_on_change(dropdown_item_t *item, void *user_data)
{
    g_send_ui.remote_selected_position = item->position;
    rebuild_button_controls(item->text);
}

static void load_remote_dropdown(void)
{
    ir_remote_list remotes = {0};
    if (!g_send_ui.dropdown_remotes)
        return;

    ir_status_t rc = ir_controller_list_remotes(&remotes);
    if (rc != IR_OK || remotes.count == 0)
    {
        dropdown_clean_items(g_send_ui.dropdown_remotes);
        send_signal_status("No remotes available.", ZV_COLOR_WARNING);
        ir_controller_free_remote_list(&remotes);
        return;
    }

    dropdown_clean_items(g_send_ui.dropdown_remotes);
    for (size_t i = 0; i < remotes.count; i++)
    {
        dropdown_item_t item = {
            .position = (int)i,
            .text = remotes.remotes[i].name
        };

        dropdown_add_item(g_send_ui.dropdown_remotes, &item);
    }

    dropdown_set_selected_item(g_send_ui.dropdown_remotes, 0);

    ir_controller_free_remote_list(&remotes);

    dropdown_item_t *default_item = dropdown_get_item(g_send_ui.dropdown_remotes, 0);
    rebuild_button_controls(default_item->text);
}

static void send_refresh_remotes_cb(event_data_btn *e)
{
    (void)e;
    load_remote_dropdown();
}

lv_obj_t *ir_send_signal_page_create(lv_obj_t *menu)
{
    base_view *view;

    memset(&g_send_ui, 0, sizeof(g_send_ui));

    view = zv_view_create(&g_send_ui.base, menu, "Send Signal");
    if (!view)
        return NULL;

    g_send_ui.base.set_flex_layout(&g_send_ui.base, LV_FLEX_FLOW_COLUMN, 12, 10);

    lv_obj_t *root = g_send_ui.base.root;
    create_section_label(root, "Remote");

    lv_obj_t *remote_list_container = create_transparent_flex_row(root, LV_PCT(100), 45);

    g_send_ui.dropdown_remotes = dropdown_create(remote_list_container, LV_PCT(70), LV_SIZE_CONTENT);
    dropdown_set_on_change_cb(g_send_ui.dropdown_remotes, remote_dropdown_on_change, NULL);
    
    ui_button *refresh_btn = ui_icon_button_create(remote_list_container, 45, 35, LV_SYMBOL_REFRESH);
    ui_button_set_on_click(refresh_btn, send_refresh_remotes_cb, NULL);

    lv_obj_t *btn = create_section_label(root, "BUTTONS");
    lv_obj_set_style_text_color(btn, ZV_COLOR_TERMINAL, 0);

    g_send_ui.grid = lv_obj_create(root);
    lv_obj_set_size(g_send_ui.grid, LV_PCT(100), LV_PCT(50));
    lv_obj_set_style_bg_opa(g_send_ui.grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_send_ui.grid, 0, 0);
    lv_obj_set_style_pad_all(g_send_ui.grid, 0, 0);
    lv_obj_set_layout(g_send_ui.grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_send_ui.grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(g_send_ui.grid, 10, 0);
    lv_obj_set_style_pad_column(g_send_ui.grid, 10, 0);

    ui_info_panel *panel = create_info_panel(root, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_t *right_item = add_info_panel_custom_row(panel, "TX: ");
    if (right_item)
    {
        g_send_ui.signal_state_pill = create_pills_sized(right_item, LV_SIZE_CONTENT, 35);
        pills_add(g_send_ui.signal_state_pill, "IDLE");
    }

    load_remote_dropdown();

    return g_send_ui.base.page;
}
