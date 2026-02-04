#include "page/ir/new_remote.h"
#include "components/ui_theme.h"
#include "components/nav.h"
#include "config.h"
#include "ir/ir_service.h"
#include "utils/string_utils.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t *keyboard;
    lv_obj_t *name_input;
    lv_obj_t *status_label;
    lv_obj_t *active_textarea;
    bool use_on_screen_keyboard;
} new_remote_ui_t;

static new_remote_ui_t g_new_remote;

bool ir_new_remote_keyboard_is_visible(void)
{
    if (!g_new_remote.use_on_screen_keyboard || !g_new_remote.keyboard)
        return false;

    return !lv_obj_has_flag(g_new_remote.keyboard, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *ir_create_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    return label;
}

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

static lv_obj_t *ir_create_icon_button(lv_obj_t *parent, const char *icon)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 68, 68);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(btn, 10, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(label);

    return btn;
}

static void ir_keyboard_hide(new_remote_ui_t *ui)
{
    lv_group_t *group;

    if (!ui || !ui->use_on_screen_keyboard || !ui->keyboard)
        return;

    printf("[IR][new_remote] keyboard hide\n");
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

static void ir_keyboard_show(new_remote_ui_t *ui, lv_obj_t *ta)
{
    lv_group_t *group;

    if (!ui || !ta)
        return;

    if (!ui->use_on_screen_keyboard || !ui->keyboard)
        return;

    printf("[IR][new_remote] keyboard show\n");
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

static void ir_keyboard_event_cb(lv_event_t *e)
{
    new_remote_ui_t *ui = (new_remote_ui_t *)lv_event_get_user_data(e);
    if (!ui) 
        return;

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        ir_keyboard_hide(ui);
    }
}

static void ir_name_input_event_cb(lv_event_t *e)
{
    new_remote_ui_t *ui = (new_remote_ui_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);

    if (!ui || !ta) 
        return;

    printf("[IR][new_remote] name_input event=%d focused=%d\n",
           (int)code, lv_obj_has_state(ta, LV_STATE_FOCUSED) ? 1 : 0);

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        printf("[IR][new_remote] key=%u\n", (unsigned)key);
        if (key == LV_KEY_ENTER) {
            ir_keyboard_show(ui, ta);
        }
        return;
    }

    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
        ir_keyboard_show(ui, ta);
    }
}

static void ir_create_remote_file(new_remote_ui_t *ui)
{
    ir_status_t rc;

    if (!ui || !ui->name_input || !ui->status_label)
        return;

    const char *name = lv_textarea_get_text(ui->name_input);
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

    rc = ir_service_create_remote(safe_name);
    if (rc == IR_OK) {
        lv_label_set_text(ui->status_label, "Remote created.");
        lv_textarea_set_text(ui->name_input, "");
        return;
    }

    lv_label_set_text_fmt(ui->status_label, "Error: %s", ir_service_last_error());
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
    g_new_remote.use_on_screen_keyboard = true;

    const zv_config *cfg = config_get();
    if (cfg) {
        g_new_remote.use_on_screen_keyboard = cfg->ir.use_on_screen_keyboard;
    }
    printf("[IR][new_remote] use_on_screen_keyboard=%d\n",
           g_new_remote.use_on_screen_keyboard ? 1 : 0);

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

    ir_create_section_label(root, "Remote Name:");

    g_new_remote.name_input = lv_textarea_create(root);
    lv_obj_set_width(g_new_remote.name_input, LV_PCT(100));
    lv_obj_set_height(g_new_remote.name_input, 40);
    lv_textarea_set_placeholder_text(g_new_remote.name_input, "Enter a name...");
    lv_obj_set_style_bg_color(g_new_remote.name_input, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(g_new_remote.name_input, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_new_remote.name_input, 2, 0);
    lv_obj_set_style_border_color(g_new_remote.name_input, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(g_new_remote.name_input, 10, 0);
    lv_obj_set_style_text_color(g_new_remote.name_input, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_color(g_new_remote.name_input, ZV_COLOR_TEXT_MUTED, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_left(g_new_remote.name_input, 10, 0);
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_input_event_cb, LV_EVENT_FOCUSED, &g_new_remote);
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_input_event_cb, LV_EVENT_DEFOCUSED, &g_new_remote);
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_input_event_cb, LV_EVENT_CLICKED, &g_new_remote);
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_input_event_cb, LV_EVENT_PRESSED, &g_new_remote);
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_input_event_cb, LV_EVENT_KEY, &g_new_remote);

    ir_create_section_label(root, "Category:");

    lv_obj_t *category_row = lv_obj_create(root);
    lv_obj_set_size(category_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(category_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(category_row, 0, 0);
    lv_obj_set_style_pad_all(category_row, 0, 0);
    lv_obj_set_layout(category_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(category_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(category_row, 10, 0);

    ir_create_chip_button(category_row, "TV");
    ir_create_chip_button(category_row, "AC");
    ir_create_chip_button(category_row, "Audio");
    ir_create_chip_button(category_row, "Custom");

    ir_create_section_label(root, "Icon:");

    lv_obj_t *icon_row = lv_obj_create(root);
    lv_obj_set_size(icon_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(icon_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_row, 0, 0);
    lv_obj_set_style_pad_all(icon_row, 0, 0);
    lv_obj_set_layout(icon_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(icon_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(icon_row, 10, 0);

    ir_create_icon_button(icon_row, LV_SYMBOL_VIDEO);
    ir_create_icon_button(icon_row, LV_SYMBOL_REFRESH);
    ir_create_icon_button(icon_row, LV_SYMBOL_AUDIO);
    ir_create_icon_button(icon_row, LV_SYMBOL_SETTINGS);

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

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

    if (g_new_remote.use_on_screen_keyboard) {
        /* Create keyboard on top layer to avoid menu/page clipping issues. */
        g_new_remote.keyboard = lv_keyboard_create(lv_layer_top());
        lv_obj_set_size(g_new_remote.keyboard, lv_pct(100), 120);
        lv_obj_align(g_new_remote.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(g_new_remote.keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(g_new_remote.keyboard, ir_keyboard_event_cb, LV_EVENT_READY, &g_new_remote);
        lv_obj_add_event_cb(g_new_remote.keyboard, ir_keyboard_event_cb, LV_EVENT_CANCEL, &g_new_remote);
    }

    return page;
}
