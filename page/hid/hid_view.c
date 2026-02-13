#include "page/hid/hid_view.h"

#include <stdlib.h>
#include <string.h>

#include "components/ui_theme.h"

typedef struct {
    hid_view *view;
    char *path;
} hid_item_ctx;

static void set_status(const char *msg, lv_color_t color, hid_view *self)
{
    if (!self || !self->status)
        return;

    lv_label_set_text(self->status, msg);
    lv_obj_set_style_text_color(self->status, color, 0);
}

static void hid_set_selected_label(hid_view *self)
{
    if (!self || !self->selected_lbl)
        return;

    const char *selected_script = hid_controller_selected_script(&self->controller);
    if (selected_script[0]) {
        const char *base = strrchr(selected_script, '/');
        base = base ? base + 1 : selected_script;
        lv_label_set_text_fmt(self->selected_lbl, "Selected: %s", base);
        return;
    }

    lv_label_set_text(self->selected_lbl, "Selected: (none)");
}

static void clear_list(lv_obj_t *list)
{
    if (!list)
        return;

    lv_obj_clean(list);
}

static void hid_script_item_clicked(lv_event_t *e)
{
    hid_item_ctx *ctx = (hid_item_ctx *)lv_event_get_user_data(e);
    if (!ctx || !ctx->view || !ctx->path)
        return;

    hid_status_t rc = hid_controller_set_selected_script(&ctx->view->controller, ctx->path);
    if (rc != HID_OK) {
        set_status(hid_controller_last_error(), lv_color_hex(0xFF5A5A), ctx->view);
        return;
    }

    hid_set_selected_label(ctx->view);
    set_status("Script selected.", ZV_COLOR_TEXT_MAIN, ctx->view);
}

static void hid_btn_delete_cb(lv_event_t *e)
{
    hid_item_ctx *ctx = (hid_item_ctx *)lv_event_get_user_data(e);
    if (!ctx)
        return;

    free(ctx->path);
    free(ctx);
}

static void add_script_button(hid_view *view, const char *name, const char *path)
{
    if (!view || !view->list || !name || !path)
        return;

    lv_obj_t *btn = lv_list_add_button(view->list, NULL, name);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_text_color(btn, ZV_COLOR_TEXT_MAIN, 0);

    char *heap_path = strdup(path);
    if (!heap_path)
        return;

    hid_item_ctx *ctx = (hid_item_ctx *)malloc(sizeof(*ctx));
    if (!ctx) {
        free(heap_path);
        return;
    }

    ctx->view = view;
    ctx->path = heap_path;

    lv_obj_add_event_cb(btn, hid_script_item_clicked, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(btn, hid_btn_delete_cb, LV_EVENT_DELETE, ctx);
}

static void refresh_list_impl(hid_view *self)
{
    if (!self)
        return;

    clear_list(self->list);

    hid_script_list list;
    hid_status_t rc = hid_controller_list_scripts(&self->controller, &list);
    if (rc != HID_OK) {
        set_status(hid_controller_last_error(), lv_color_hex(0xFF5A5A), self);
        return;
    }

    for (size_t i = 0; i < list.count; i++)
        add_script_button(self, list.scripts[i].name, list.scripts[i].path);

    hid_controller_free_script_list(&list);
}

static void hid_refresh_btn_cb(lv_event_t *e)
{
    hid_view *self = (hid_view *)lv_event_get_user_data(e);
    refresh_list_impl(self);
}

static void hid_toggle_cb(lv_event_t *e)
{
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    hid_view *self = (hid_view *)lv_event_get_user_data(e);
    if (!sw || !self)
        return;

    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    hid_status_t rc = hid_controller_toggle(&self->controller, on);
    if (rc != HID_OK)
    {
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        set_status(hid_controller_last_error(), lv_color_hex(0xFF5A5A), self);
        return;
    }

    hid_set_selected_label(self);
}

static hid_view *hid_page_create(hid_view *self, lv_obj_t *menu, const zv_config *cfg)
{
    base_view *view = zv_view_create(&self->base, menu, "HID / BadUSB");
    if (!view)
        return NULL;

    if (hid_controller_init(&self->controller, cfg) != HID_OK)
        return NULL;

    lv_obj_set_style_pad_all(self->base.root, 8, 0);
    self->base.set_flex_layout(&self->base, LV_FLEX_FLOW_COLUMN, 5, 0);

    lv_obj_t *enable_container = lv_obj_create(self->base.root);
    lv_obj_set_size(enable_container, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(enable_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(enable_container, 0, 0);
    lv_obj_set_style_pad_all(enable_container, 0, 0);
    lv_obj_set_layout(enable_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(enable_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(enable_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    self->status = lv_label_create(enable_container);
    lv_label_set_text(self->status, "Enable HID");
    lv_obj_set_style_text_color(self->status, ZV_COLOR_TEXT_MAIN, 0);

    self->toggle = lv_switch_create(enable_container);
    lv_obj_add_event_cb(self->toggle, hid_toggle_cb, LV_EVENT_VALUE_CHANGED, self);

    lv_obj_t *center_container = lv_obj_create(self->base.root);
    lv_obj_set_size(center_container, LV_PCT(100), 45);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(center_container, 0, 0);
    lv_obj_set_style_pad_all(center_container, 0, 0);
    lv_obj_set_layout(center_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(center_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(center_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    self->selected_lbl = lv_label_create(center_container);
    lv_obj_set_style_text_color(self->selected_lbl, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *refresh_btn = lv_btn_create(center_container);
    lv_obj_set_size(refresh_btn, 45, 35);
    lv_obj_set_style_bg_color(refresh_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(refresh_btn, 2, 0);
    lv_obj_set_style_border_color(refresh_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(refresh_btn, 10, 0);
    lv_obj_add_event_cb(refresh_btn, hid_refresh_btn_cb, LV_EVENT_CLICKED, self);
    lv_obj_set_flex_flow(refresh_btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(refresh_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(refresh_btn, 6, 0);

    lv_obj_t *icon = lv_label_create(refresh_btn);
    lv_label_set_text(icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(icon, ZV_COLOR_TEXT_MAIN, 0);

    self->list = lv_list_create(self->base.root);
    lv_obj_set_width(self->list, LV_PCT(100));
    lv_obj_set_style_bg_color(self->list, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_radius(self->list, 5, 0);
    lv_obj_set_style_pad_all(self->list, 5, 0);
    lv_obj_set_style_pad_row(self->list, 10, 0);
    lv_obj_set_scroll_dir(self->list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(self->list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(self->list, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(self->list, LV_OBJ_FLAG_SCROLL_MOMENTUM);

    hid_set_selected_label(self);
    refresh_list_impl(self);
    lv_obj_set_state(self->toggle, LV_STATE_CHECKED, self->controller.hid_enabled);

    return self;
}

lv_obj_t *hid_view_create(lv_obj_t *menu, const zv_config *cfg)
{
    hid_view *view = (hid_view *)malloc(sizeof(*view));
    if (!view)
        return NULL;

    hid_view *self = hid_page_create(view, menu, cfg);
    if (!self)
    {
        free(view);
        return NULL;
    }

    return self->base.page;
}

void hid_refresh_scripts(hid_view *self)
{
    refresh_list_impl(self);
}

const char *hid_get_selected_script(hid_view *self)
{
    if (!self)
        return "";

    return hid_controller_selected_script(&self->controller);
}
