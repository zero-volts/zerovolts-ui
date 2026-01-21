#include "page/ir/new_remote.h"
#include "components/ui_theme.h"
#include "components/nav.h"
#include "config.h"
#include "utils/file.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    lv_obj_t *keyboard;
    lv_obj_t *name_input;
    char remotes_path[512];
} new_remote_ui_t;

static new_remote_ui_t g_new_remote;

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
    if (!ui || !ui->keyboard) 
        return;

    lv_obj_add_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(ui->keyboard, NULL);
}

static void ir_keyboard_show(new_remote_ui_t *ui, lv_obj_t *ta)
{
    if (!ui || !ta) 
        return;

    if (!ui->keyboard) 
        return;

    lv_keyboard_set_textarea(ui->keyboard, ta);
    lv_obj_clear_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
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

static void ir_name_focus_cb(lv_event_t *e)
{
    new_remote_ui_t *ui = (new_remote_ui_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);

    if (!ui || !ta) 
        return;

    if (code == LV_EVENT_FOCUSED) {
        ir_keyboard_show(ui, ta);
    } else if (code == LV_EVENT_DEFOCUSED) {
        ir_keyboard_hide(ui);
    }
}

static int ir_ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }

    if (errno != ENOENT) 
        return -2;

    if (mkdir(path, 0755) != 0) 
        return -3;

    return 0;
}

static void ir_sanitize_name(const char *src, char *dst, size_t dst_len)
{
    size_t i = 0;
    if (!dst || dst_len == 0) 
        return;

    for (; src && src[i] && i + 1 < dst_len; i++) {
        char c = src[i];
        if (c == '/' || c == '\\') 
            c = '_';
        dst[i] = c;
    }
    dst[i] = '\0';
}

static void ir_create_remote_file(new_remote_ui_t *ui)
{
    if (!ui || !ui->name_input) 
        return;

    const char *name = lv_textarea_get_text(ui->name_input);
    if (!name || !name[0]) 
        return;

    if (!ui->remotes_path[0]) 
        return;

    if (ir_ensure_dir(ui->remotes_path) != 0) 
        return;

    char safe_name[256];
    ir_sanitize_name(name, safe_name, sizeof(safe_name));
    if (!safe_name[0]) 
        return;

    char file_path[768];
    int written = snprintf(file_path, sizeof(file_path), "%s/%s.lircd.conf",
                           ui->remotes_path, safe_name);
    if (written < 0 || (size_t)written >= sizeof(file_path))
        return;
    write_entire_file(file_path, "", 0);
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

    const zv_config *cfg = config_get();
    if (cfg) {
        snprintf(g_new_remote.remotes_path, sizeof(g_new_remote.remotes_path), "%s", cfg->ir.remotes_path);
    }

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
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_focus_cb, LV_EVENT_FOCUSED, &g_new_remote);
    lv_obj_add_event_cb(g_new_remote.name_input, ir_name_focus_cb, LV_EVENT_DEFOCUSED, &g_new_remote);

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

    g_new_remote.keyboard = lv_keyboard_create(page);
    lv_obj_set_size(g_new_remote.keyboard, LV_PCT(100), 120);
    lv_obj_align(g_new_remote.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(g_new_remote.keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_new_remote.keyboard, ir_keyboard_event_cb, LV_EVENT_READY, &g_new_remote);
    lv_obj_add_event_cb(g_new_remote.keyboard, ir_keyboard_event_cb, LV_EVENT_CANCEL, &g_new_remote);

    return page;
}
