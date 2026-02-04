#include "page/ir/learn_button.h"
#include "components/ui_theme.h"
#include "components/nav.h"
#include "config.h"
#include "ir/ir_service.h"
#include "utils/string_utils.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t *keyboard;
    lv_obj_t *remote_dropdown;
    lv_obj_t *button_input;
    lv_obj_t *status;
    lv_obj_t *active_textarea;
    bool use_on_screen_keyboard;
} learn_ui_t;

static learn_ui_t g_learn;

bool ir_learn_button_keyboard_is_visible(void)
{
    if (!g_learn.use_on_screen_keyboard || !g_learn.keyboard)
        return false;

    return !lv_obj_has_flag(g_learn.keyboard, LV_OBJ_FLAG_HIDDEN);
}

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
        lv_dropdown_set_options(obj, "");
    } else {
        lv_textarea_set_one_line(obj, true);
        lv_textarea_set_placeholder_text(obj, placeholder);
        lv_obj_set_style_text_color(obj, ZV_COLOR_TEXT_MUTED, LV_PART_TEXTAREA_PLACEHOLDER);
    }

    return obj;
}

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

static void get_dropdown_text(lv_obj_t *dd, char *out, size_t out_sz)
{
    if (!dd || !out || out_sz == 0)
        return;

    out[0] = '\0';
    lv_dropdown_get_selected_str(dd, out, out_sz);
}

static void load_remote_dropdown(void)
{
    ir_remote_list_t remotes = {0};
    char opts[2048];

    if (!g_learn.remote_dropdown)
        return;

    opts[0] = '\0';
    printf("[IR][learn_ui] loading remotes...\n");

    if (ir_service_list_remotes(&remotes) != IR_OK || remotes.count == 0) {
        printf("[IR][learn_ui] no remotes available\n");
        lv_dropdown_set_options(g_learn.remote_dropdown, "");
        learn_set_status("No remotes available. Create one first.");
        ir_service_free_remotes(&remotes);
        return;
    }

    for (size_t i = 0; i < remotes.count; i++) {
        strncat(opts, remotes.items[i].name, sizeof(opts) - strlen(opts) - 1);
        if (i + 1 < remotes.count)
            strncat(opts, "\n", sizeof(opts) - strlen(opts) - 1);
    }

    lv_dropdown_set_options(g_learn.remote_dropdown, opts);
    lv_dropdown_set_selected(g_learn.remote_dropdown, 0);
    learn_set_status("Ready to capture.");
    printf("[IR][learn_ui] remotes loaded: %zu\n", remotes.count);

    ir_service_free_remotes(&remotes);
}

static void learn_refresh_remotes_cb(lv_event_t *e)
{
    (void)e;
    load_remote_dropdown();
}

static void learn_keyboard_hide(learn_ui_t *ui)
{
    lv_group_t *group;

    if (!ui || !ui->use_on_screen_keyboard || !ui->keyboard)
        return;

    group = lv_obj_get_group(ui->keyboard);
    if (group)
        lv_group_set_editing(group, false);

    lv_obj_add_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(ui->keyboard, NULL);
    if (group)
        lv_group_remove_obj(ui->keyboard);

    if (ui->active_textarea && group)
        lv_group_focus_obj(ui->active_textarea);

    ui->active_textarea = NULL;
}

static void learn_keyboard_show(learn_ui_t *ui, lv_obj_t *ta)
{
    lv_group_t *group;

    if (!ui || !ta)
        return;

    if (!ui->use_on_screen_keyboard || !ui->keyboard)
        return;

    ui->active_textarea = ta;
    lv_keyboard_set_textarea(ui->keyboard, ta);
    lv_obj_clear_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui->keyboard);

    group = lv_obj_get_group(ta);
    if (group) {
        if (!lv_obj_get_group(ui->keyboard))
            lv_group_add_obj(group, ui->keyboard);
        lv_group_focus_obj(ui->keyboard);
        lv_group_set_editing(group, true);
    }
}

static void learn_keyboard_event_cb(lv_event_t *e)
{
    learn_ui_t *ui = (learn_ui_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (!ui)
        return;

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
        learn_keyboard_hide(ui);
}

static void learn_button_input_event_cb(lv_event_t *e)
{
    learn_ui_t *ui = (learn_ui_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);

    if (!ui || !ta)
        return;

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ENTER)
            learn_keyboard_show(ui, ta);
        return;
    }

    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED)
        learn_keyboard_show(ui, ta);
}

static void learn_finish_cb(lv_event_t *e)
{
    char remote[IR_NAME_MAX];
    char button[IR_NAME_MAX];
    ir_status_t rc;
    lv_obj_t *finish_btn = (lv_obj_t *)lv_event_get_target(e);

    get_dropdown_text(g_learn.remote_dropdown, remote, sizeof(remote));
    snprintf(button, sizeof(button), "%s", lv_textarea_get_text(g_learn.button_input));
    zv_trim_inplace(button);
    printf("[IR][learn_ui] finish pressed remote='%s' button='%s'\n", remote, button);

    if (!remote[0]) {
        learn_set_status("Select a remote first.");
        return;
    }

    if (!button[0]) {
        learn_set_status("Enter button name (e.g. KEY_POWER).");
        return;
    }

    learn_keyboard_hide(&g_learn);
    if (finish_btn)
        lv_group_focus_obj(finish_btn);

    learn_set_status("Recording signal... Press remote now.");
    lv_refr_now(NULL);

    rc = ir_service_learn_button(remote, button);
    printf("[IR][learn_ui] learn result rc=%d err='%s'\n", (int)rc, ir_service_last_error());
    
    if (rc == IR_OK) {
        learn_set_status("Signal captured and stored.");
        return;
    }

    learn_set_status(ir_service_last_error());
}

static void learn_cancel_cb(lv_event_t *e)
{
    (void)e;
    learn_keyboard_hide(&g_learn);
    if (g_learn.button_input)
        lv_textarea_set_text(g_learn.button_input, "");
    learn_set_status("Capture canceled.");
}

lv_obj_t *ir_learn_button_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Learn Button");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    memset(&g_learn, 0, sizeof(g_learn));
    g_learn.use_on_screen_keyboard = true;

    const zv_config *cfg = config_get();
    if (cfg)
        g_learn.use_on_screen_keyboard = cfg->ir.use_on_screen_keyboard;

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
    g_learn.remote_dropdown = ir_create_input_box(root, "", true);

    lv_obj_t *refresh_btn = lv_btn_create(root);
    lv_obj_set_size(refresh_btn, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(refresh_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(refresh_btn, 2, 0);
    lv_obj_set_style_border_color(refresh_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(refresh_btn, 10, 0);
    lv_obj_add_event_cb(refresh_btn, learn_refresh_remotes_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, LV_SYMBOL_REFRESH "  Refresh remotes");
    lv_obj_set_style_text_color(refresh_lbl, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(refresh_lbl);

    ir_create_section_label(root, "Button Name:");
    g_learn.button_input = ir_create_input_box(root, "KEY_VOLUMEUP", false);
    lv_obj_add_event_cb(g_learn.button_input, learn_button_input_event_cb, LV_EVENT_FOCUSED, &g_learn);
    lv_obj_add_event_cb(g_learn.button_input, learn_button_input_event_cb, LV_EVENT_DEFOCUSED, &g_learn);
    lv_obj_add_event_cb(g_learn.button_input, learn_button_input_event_cb, LV_EVENT_CLICKED, &g_learn);
    lv_obj_add_event_cb(g_learn.button_input, learn_button_input_event_cb, LV_EVENT_PRESSED, &g_learn);
    lv_obj_add_event_cb(g_learn.button_input, learn_button_input_event_cb, LV_EVENT_KEY, &g_learn);

    lv_obj_t *icon = lv_label_create(root);
    lv_label_set_text(icon, LV_SYMBOL_GPS);
    lv_obj_set_style_text_color(icon, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *hint = lv_label_create(root);
    lv_label_set_text(hint, "Point remote at receiver and press one button.");
    lv_obj_set_style_text_color(hint, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, LV_PCT(100));

    ir_create_status_box(root);

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
    lv_obj_add_event_cb(cancel_btn, learn_cancel_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_set_style_text_color(cancel_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(cancel_label);

    lv_obj_t *finish_btn = lv_btn_create(footer_row);
    lv_obj_set_size(finish_btn, LV_PCT(45), 40);
    lv_obj_set_style_bg_color(finish_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(finish_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(finish_btn, 2, 0);
    lv_obj_set_style_border_color(finish_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(finish_btn, 12, 0);
    lv_obj_add_event_cb(finish_btn, learn_finish_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *finish_label = lv_label_create(finish_btn);
    lv_label_set_text(finish_label, "Finish");
    lv_obj_set_style_text_color(finish_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(finish_label);

    if (g_learn.use_on_screen_keyboard) {
        g_learn.keyboard = lv_keyboard_create(lv_layer_top());
        lv_obj_set_size(g_learn.keyboard, lv_pct(100), 120);
        lv_obj_align(g_learn.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(g_learn.keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(g_learn.keyboard, learn_keyboard_event_cb, LV_EVENT_READY, &g_learn);
        lv_obj_add_event_cb(g_learn.keyboard, learn_keyboard_event_cb, LV_EVENT_CANCEL, &g_learn);
    }

    load_remote_dropdown();

    return page;
}
