#include "page/ir/remotes.h"
#include "components/ui_theme.h"
#include "ir/ir_service.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t *root;
    lv_obj_t *list_container;
    lv_obj_t *status_label;
} remotes_ui_t;

static remotes_ui_t g_remotes;

static lv_obj_t *ir_create_remote_card(lv_obj_t *parent, const char *icon, const char *title, const char *subtitle)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), 70);
    lv_obj_set_style_bg_color(card, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(card, 12, 0);

    lv_obj_t *ic = lv_label_create(card);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *text_col = lv_obj_create(card);
    lv_obj_set_size(text_col, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(text_col, 0, 0);
    lv_obj_set_style_pad_all(text_col, 0, 0);
    lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(text_col, 6, 0);

    lv_obj_t *title_lbl = lv_label_create(text_col);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *subtitle_lbl = lv_label_create(text_col);
    lv_label_set_text(subtitle_lbl, subtitle);
    lv_obj_set_style_text_color(subtitle_lbl, ZV_COLOR_TEXT_MUTED, 0);

    return card;
}

static void remotes_refresh(void)
{
    ir_remote_list_t remotes = {0};
    ir_status_t rc;

    if (!g_remotes.list_container || !g_remotes.status_label)
        return;

    lv_obj_clean(g_remotes.list_container);
    lv_label_set_text(g_remotes.status_label, "");

    rc = ir_service_list_remotes(&remotes);
    if (rc != IR_OK) {
        lv_label_set_text_fmt(g_remotes.status_label, "Error: %s", ir_service_last_error());
        return;
    }

    if (remotes.count == 0) {
        lv_label_set_text(g_remotes.status_label, "No remotes yet. Create one first.");
        ir_service_free_remotes(&remotes);
        return;
    }

    for (size_t i = 0; i < remotes.count; i++) {
        char subtitle[80];
        snprintf(subtitle, sizeof(subtitle), "%d buttons learned", remotes.items[i].button_count);
        ir_create_remote_card(g_remotes.list_container, LV_SYMBOL_VIDEO, remotes.items[i].name, subtitle);
    }

    ir_service_free_remotes(&remotes);
}

static void remotes_refresh_btn_cb(lv_event_t *e)
{
    (void)e;
    remotes_refresh();
}

lv_obj_t *ir_remotes_page_create(lv_obj_t *menu)
{
    lv_obj_t *page;
    lv_obj_t *root;
    lv_obj_t *footer_row;
    lv_obj_t *refresh_btn;
    lv_obj_t *refresh_label;

    memset(&g_remotes, 0, sizeof(g_remotes));

    page = lv_menu_page_create(menu, "IR Remotes");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    root = lv_obj_create(page);
    g_remotes.root = root;
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 10, 0);

    g_remotes.list_container = lv_obj_create(root);
    lv_obj_set_size(g_remotes.list_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(g_remotes.list_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_remotes.list_container, 0, 0);
    lv_obj_set_style_pad_all(g_remotes.list_container, 0, 0);
    lv_obj_set_layout(g_remotes.list_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_remotes.list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(g_remotes.list_container, 12, 0);

    g_remotes.status_label = lv_label_create(root);
    lv_label_set_text(g_remotes.status_label, "");
    lv_obj_set_style_text_color(g_remotes.status_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_width(g_remotes.status_label, LV_PCT(100));

    lv_obj_t *spacer = lv_obj_create(root);
    lv_obj_set_size(spacer, LV_PCT(100), 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    footer_row = lv_obj_create(root);
    lv_obj_set_size(footer_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(footer_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(footer_row, 0, 0);
    lv_obj_set_style_pad_all(footer_row, 0, 0);
    lv_obj_set_layout(footer_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer_row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    refresh_btn = lv_btn_create(footer_row);
    lv_obj_set_size(refresh_btn, LV_PCT(100), 44);
    lv_obj_set_style_bg_color(refresh_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(refresh_btn, 2, 0);
    lv_obj_set_style_border_color(refresh_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(refresh_btn, 12, 0);
    lv_obj_add_event_cb(refresh_btn, remotes_refresh_btn_cb, LV_EVENT_CLICKED, NULL);

    refresh_label = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH "  Refresh");
    lv_obj_set_style_text_color(refresh_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(refresh_label);

    remotes_refresh();

    return page;
}
