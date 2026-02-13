#include "send_signal.h"
#include "components/ui_theme.h"
#include "page/ir/ir_controller.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t *remote_dropdown;
    lv_obj_t *button_dropdown;
    lv_obj_t *grid;
    lv_obj_t *status;
    char selected_button[IR_MAX_NAME];
} send_signal_ui_t;

static send_signal_ui_t g_send_ui;

static void load_remote_dropdown(void);

static lv_obj_t *ir_create_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    return label;
}

static lv_obj_t *ir_create_dropdown_box(lv_obj_t *parent)
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

    return btn;
}

static void get_dropdown_text(lv_obj_t *dd, char *out, size_t out_sz)
{
    if (!dd || !out || out_sz == 0)
        return;

    out[0] = '\0';
    lv_dropdown_get_selected_str(dd, out, out_sz);
}

static void send_signal_status(const char *msg)
{
    if (!g_send_ui.status)
        return;
    lv_label_set_text(g_send_ui.status, msg);
}

static void send_selected_button(void)
{
    char remote[IR_MAX_NAME];
    char button[IR_MAX_NAME];
    ir_status_t rc;

    get_dropdown_text(g_send_ui.remote_dropdown, remote, sizeof(remote));

    if (g_send_ui.selected_button[0]) {
        snprintf(button, sizeof(button), "%s", g_send_ui.selected_button);
    } else {
        get_dropdown_text(g_send_ui.button_dropdown, button, sizeof(button));
    }

    if (!remote[0] || !button[0]) {
        send_signal_status("Select remote and button.");
        return;
    }

    rc = ir_controller_send_button(remote, button);
    if (rc == IR_OK) {
        send_signal_status("Signal sent.");
        return;
    }

    send_signal_status(ir_controller_last_error());
}

static void send_grid_button_cb(lv_event_t *e)
{
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char *btn_name;

    if (!label)
        return;

    btn_name = lv_label_get_text(label);
    if (!btn_name)
        return;

    snprintf(g_send_ui.selected_button, sizeof(g_send_ui.selected_button), "%s", btn_name);
    send_selected_button();
}

static void clear_grid(void)
{
    if (g_send_ui.grid)
        lv_obj_clean(g_send_ui.grid);
}

static void set_dropdown_options_from_buttons(const ir_button_list *buttons)
{
    char opts[2048];

    if (!g_send_ui.button_dropdown)
        return;

    opts[0] = '\0';

    if (!buttons || buttons->count == 0) {
        lv_dropdown_set_options(g_send_ui.button_dropdown, "");
        return;
    }

    for (size_t i = 0; i < buttons->count; i++) {
        strncat(opts, buttons->buttons[i].name, sizeof(opts) - strlen(opts) - 1);
        if (i + 1 < buttons->count)
            strncat(opts, "\n", sizeof(opts) - strlen(opts) - 1);
    }

    lv_dropdown_set_options(g_send_ui.button_dropdown, opts);
    lv_dropdown_set_selected(g_send_ui.button_dropdown, 0);
}

static void rebuild_button_controls(const char *remote_name)
{
    ir_button_list buttons = {0};
    ir_status_t rc;

    g_send_ui.selected_button[0] = '\0';
    clear_grid();

    rc = ir_controller_list_buttons(remote_name, &buttons);
    if (rc != IR_OK) {
        set_dropdown_options_from_buttons(NULL);
        send_signal_status("No buttons for this remote.");
        return;
    }

    set_dropdown_options_from_buttons(&buttons);

    for (size_t i = 0; i < buttons.count && i < 9; i++) {
        lv_obj_t *btn = ir_create_key_button(g_send_ui.grid, buttons.buttons[i].name);
        lv_obj_add_event_cb(btn, send_grid_button_cb, LV_EVENT_CLICKED, NULL);
    }

    if (buttons.count == 0)
        send_signal_status("No buttons learned yet.");
    else
        send_signal_status("");

    ir_controller_free_button_list(&buttons);
}

static void remote_changed_cb(lv_event_t *e)
{
    char remote[IR_MAX_NAME];

    (void)e;
    get_dropdown_text(g_send_ui.remote_dropdown, remote, sizeof(remote));
    if (!remote[0])
        return;

    rebuild_button_controls(remote);
}

static void send_btn_cb(lv_event_t *e)
{
    (void)e;
    send_selected_button();
}

static void send_refresh_remotes_cb(lv_event_t *e)
{
    (void)e;
    load_remote_dropdown();
}

static void load_remote_dropdown(void)
{
    ir_remote_list remotes = {0};
    ir_status_t rc;
    char opts[2048];

    if (!g_send_ui.remote_dropdown)
        return;

    opts[0] = '\0';

    rc = ir_controller_list_remotes(&remotes);
    if (rc != IR_OK || remotes.count == 0) {
        lv_dropdown_set_options(g_send_ui.remote_dropdown, "");
        send_signal_status("No remotes available.");
        ir_controller_free_remote_list(&remotes);
        return;
    }

    for (size_t i = 0; i < remotes.count; i++) {
        strncat(opts, remotes.remotes[i].name, sizeof(opts) - strlen(opts) - 1);
        if (i + 1 < remotes.count)
            strncat(opts, "\n", sizeof(opts) - strlen(opts) - 1);
    }

    lv_dropdown_set_options(g_send_ui.remote_dropdown, opts);
    lv_dropdown_set_selected(g_send_ui.remote_dropdown, 0);

    ir_controller_free_remote_list(&remotes);

    remote_changed_cb(NULL);
}

#ifdef __cplusplus
extern "C"
#endif
lv_obj_t *ir_send_signal_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Send Signal");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    memset(&g_send_ui, 0, sizeof(g_send_ui));

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
    g_send_ui.remote_dropdown = ir_create_dropdown_box(root);
    lv_obj_add_event_cb(g_send_ui.remote_dropdown, remote_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *refresh_btn = lv_btn_create(root);
    lv_obj_set_size(refresh_btn, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(refresh_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(refresh_btn, 2, 0);
    lv_obj_set_style_border_color(refresh_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(refresh_btn, 10, 0);
    lv_obj_add_event_cb(refresh_btn, send_refresh_remotes_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, LV_SYMBOL_REFRESH "  Refresh remotes");
    lv_obj_set_style_text_color(refresh_lbl, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(refresh_lbl);

    ir_create_section_label(root, "Button:");
    g_send_ui.button_dropdown = ir_create_dropdown_box(root);

    g_send_ui.grid = lv_obj_create(root);
    lv_obj_set_size(g_send_ui.grid, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(g_send_ui.grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_send_ui.grid, 0, 0);
    lv_obj_set_style_pad_all(g_send_ui.grid, 0, 0);
    lv_obj_set_layout(g_send_ui.grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_send_ui.grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(g_send_ui.grid, 10, 0);
    lv_obj_set_style_pad_column(g_send_ui.grid, 10, 0);

    g_send_ui.status = lv_label_create(root);
    lv_label_set_text(g_send_ui.status, "");
    lv_obj_set_style_text_color(g_send_ui.status, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_width(g_send_ui.status, LV_PCT(100));

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
    lv_obj_add_event_cb(send_btn, send_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *send_label = lv_label_create(send_btn);
    lv_label_set_text(send_label, "Send");
    lv_obj_set_style_text_color(send_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(send_label);

    load_remote_dropdown();

    return page;
}
