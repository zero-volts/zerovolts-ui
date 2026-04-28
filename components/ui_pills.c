#include "ui_pills.h"
#include "components/ui_theme.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PILLS 16

struct ui_pills {
    lv_obj_t *container;
    lv_obj_t *buttons[MAX_PILLS];
    char *labels[MAX_PILLS];
    int pill_height;
    int count;
    int active;
    ui_pills_event_cb_t cb;
    void *user_data;
};

typedef struct {
    ui_pills *pills;
    int index;
} pill_ctx_t;

static void apply_pill_style(lv_obj_t *btn, bool active)
{
    lv_color_t bg = active ? ZV_COLOR_ACCENT_DIM : ZV_COLOR_BG_PANEL;
    lv_color_t border = active ? ZV_COLOR_ACCENT : ZV_COLOR_BORDER;
    lv_color_t text = active ? ZV_COLOR_ACCENT : ZV_COLOR_TEXT_MUTED;

    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_border_color(btn, border, 0);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER_FOCUS, LV_STATE_FOCUSED);

    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl)
        lv_obj_set_style_text_color(lbl, text, 0);
}

static void on_pill_click(lv_event_t *e)
{
    pill_ctx_t *ctx = (pill_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->pills)
        return;

    pills_set_active(ctx->pills, ctx->index);

    if (ctx->pills->cb)
        ctx->pills->cb(ctx->pills, ctx->index,
                       ctx->pills->labels[ctx->index],
                       ctx->pills->user_data);
}

static void on_pill_delete(lv_event_t *e)
{
    pill_ctx_t *ctx = (pill_ctx_t *)lv_event_get_user_data(e);
    free(ctx);
}

ui_pills *create_pills_sized(lv_obj_t *parent, int container_width, int pill_height)
{
    ui_pills *p = (ui_pills *)calloc(1, sizeof(ui_pills));
    if (!p)
        return NULL;

    p->active = -1;
    p->pill_height = pill_height;

    p->container = lv_obj_create(parent);
    lv_obj_remove_style_all(p->container);
    lv_obj_set_size(p->container, container_width, LV_SIZE_CONTENT);
    lv_obj_set_layout(p->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(p->container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(p->container, 4, 0);
    lv_obj_set_style_pad_row(p->container, 4, 0);
    lv_obj_clear_flag(p->container, LV_OBJ_FLAG_SCROLLABLE);

    return p;
}

ui_pills *create_pills(lv_obj_t *parent)
{
    return create_pills_sized(parent, LV_PCT(100), 40);
}

void pills_add(ui_pills *pills, const char *label)
{
    if (!pills || pills->count >= MAX_PILLS || !label)
        return;

    int idx = pills->count;

    lv_obj_t *btn = lv_btn_create(pills->container);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, pills->pill_height);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_pad_left(btn, 12, 0);
    lv_obj_set_style_pad_right(btn, 12, 0);
    lv_obj_set_style_pad_top(btn, 0, 0);
    lv_obj_set_style_pad_bottom(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);

    pill_ctx_t *ctx = (pill_ctx_t *)malloc(sizeof(pill_ctx_t));
    if (!ctx) {
        lv_obj_del(btn);
        return;
    }

    ctx->pills = pills;
    ctx->index = idx;

    if (pills->cb){
        lv_obj_add_event_cb(btn, on_pill_click, LV_EVENT_CLICKED, ctx);
        lv_obj_add_event_cb(btn, on_pill_delete, LV_EVENT_DELETE, ctx);
    }

    pills->buttons[idx] = btn;
    pills->labels[idx] = strdup(label);
    pills->count++;

    apply_pill_style(btn, false);
}

void pills_set_active(ui_pills *pills, int index)
{
    if (!pills)
        return;

    for (int i = 0; i < pills->count; i++)
        apply_pill_style(pills->buttons[i], i == index);

    pills->active = index;
}

int pills_get_active(const ui_pills *pills)
{
    return pills ? pills->active : -1;
}

void pills_set_event_cb(ui_pills *pills, ui_pills_event_cb_t cb, void *user_data)
{
    if (!pills)
        return;

    pills->cb = cb;
    pills->user_data = user_data;
}

void pills_clear(ui_pills *pills)
{
    if (!pills || !pills->container)
        return;

    lv_obj_clean(pills->container);

    for (int i = 0; i < pills->count; i++)
    {
        free(pills->labels[i]);

        pills->labels[i] = NULL;
        pills->buttons[i] = NULL;
    }

    pills->count = 0;
    pills->active = -1;
}

void destroy_pills(ui_pills *pills)
{
    if (!pills)
        return;

    pills_clear(pills);
    if (pills->container)
        lv_obj_del(pills->container);

    free(pills);
}
