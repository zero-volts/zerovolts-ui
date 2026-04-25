#include "ui_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "components/ui_theme.h"

struct ui_list{
    lv_obj_t *list;
    int item_count;
    ui_list_item_event_cb_t cb;
    void *user_data;
};

typedef struct {
    ui_list *list;
    list_item_t item;

    char *text_buf;
    char *subtitle_buf;
    char *raw_value_buf;
    void *user_data_buf;
} ui_list_item_ctx_t;

static void on_item_click(lv_event_t *e)
{
    ui_list_item_ctx_t *ctx = (ui_list_item_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->list)
        return;

    printf("selected item: %s \n", ctx->item.text);
    if (ctx->list->cb) {
        ctx->list->cb(ctx->list, &ctx->item, ctx->list->user_data);
    }
}

static void on_item_delete(lv_event_t *e)
{
    ui_list_item_ctx_t *ctx = (ui_list_item_ctx_t *)lv_event_get_user_data(e);
    if (ctx)
    {
        free(ctx->text_buf);
        free(ctx->subtitle_buf);
        free(ctx->raw_value_buf);
        free(ctx->user_data_buf);
        free(ctx);
    }
}

ui_list *create_list(lv_obj_t *parent, int width, int height)
{
    ui_list *list = (ui_list *)malloc(sizeof(ui_list));

    lv_obj_t *list_obj = lv_obj_create(parent);
    lv_obj_set_size(list_obj, LV_PCT(width), LV_PCT(height));

    lv_obj_set_style_bg_color(list_obj, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_border_color(list_obj, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(list_obj, 1, 0);
    lv_obj_set_style_radius(list_obj, 5, 0);
    lv_obj_set_style_pad_all(list_obj, 5, 0);
    lv_obj_set_style_pad_row(list_obj, 10, 0);

    lv_obj_set_layout(list_obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_set_scroll_dir(list_obj, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list_obj, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(list_obj, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(list_obj, LV_OBJ_FLAG_SCROLL_MOMENTUM);

    list->list = list_obj;
    list->user_data = NULL;
    list->item_count = 0;
    list->cb = NULL;

    return list;
}

lv_obj_t *add_item(ui_list *list, const list_item_t *item)
{
    if (!list || !item)
        return NULL;

    lv_obj_t *btn = lv_btn_create(list->list);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn, LV_PCT(100), 50);

    ui_list_item_ctx_t *ctx = (ui_list_item_ctx_t *)malloc(sizeof(ui_list_item_ctx_t));
    if (!ctx)
    {
        lv_obj_del(btn);
        return NULL;
    }

    ctx->list = list;
    ctx->text_buf = item->text ? strdup(item->text) : NULL;
    ctx->subtitle_buf = item->subtitle ? strdup(item->subtitle) : NULL;
    ctx->raw_value_buf = item->raw_value ? strdup(item->raw_value) : NULL;
    ctx->user_data_buf = NULL;

    if (item->user_data != NULL && item->user_data_size > 0)
    {
        ctx->user_data_buf = malloc(item->user_data_size);
        if (ctx->user_data_buf == NULL)
        {
            free(ctx->text_buf);
            free(ctx->subtitle_buf);
            free(ctx->raw_value_buf);
            free(ctx);
            lv_obj_del(btn);
            return NULL;
        }

        memcpy(ctx->user_data_buf, item->user_data, item->user_data_size);
        ctx->item.user_data = ctx->user_data_buf;
    }
    else
    {
        // size==0 -> shallow: the caller keeps the pointer alive
        ctx->item.user_data = item->user_data;
    }

    ctx->item.text = ctx->text_buf;
    ctx->item.subtitle = ctx->subtitle_buf;
    ctx->item.raw_value = ctx->raw_value_buf;
    ctx->item.user_data_size = item->user_data_size;

    lv_obj_add_event_cb(btn, on_item_click, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(btn, on_item_delete, LV_EVENT_DELETE, ctx);

    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_CARD, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER_FOCUS, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, ZV_COLOR_TERMINAL, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, ZV_COLOR_TERMINAL, LV_STATE_CHECKED);

    lv_obj_t *content = lv_obj_create(btn);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(content, 10, 0);
    lv_obj_set_style_pad_right(content, 10, 0);
    lv_obj_set_style_pad_top(content, 0, 0);
    lv_obj_set_style_pad_bottom(content, 0, 0);
    lv_obj_set_style_pad_column(content, 8, 0);

    if(item->left_badge.type == BAGE_IMG_TYPE)
    {
        obj_icon_t bagde_icon = item->left_badge.icon;
        if (bagde_icon.path != NULL && bagde_icon.path[0] != '\0')
        {
            lv_obj_t *icon = lv_img_create(content);
            lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_size(icon, bagde_icon.size.width, bagde_icon.size.height);

            char lv_path[1024];
            snprintf(lv_path, sizeof(lv_path), "A:%s", bagde_icon.path);

            lv_img_set_src(icon, lv_path);
            lv_image_set_scale(icon, LV_SCALE_NONE * 0.3);
        }
    }
    else if (item->left_badge.type == BAGE_TEXT_TYPE &&
             item->left_badge.label != NULL && item->left_badge.label[0] != '\0')
    {
        lv_obj_t *icon = lv_label_create(content);
        lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(icon, item->left_badge.label);
        lv_color_t color = item->left_badge.has_text_color
                               ? item->left_badge.text_color
                               : ZV_COLOR_ACCENT;
        lv_obj_set_style_text_color(icon, color, 0);
    }

    lv_obj_t *text_layout = lv_obj_create(content);
    lv_obj_remove_style_all(text_layout);
    lv_obj_set_layout(text_layout, LV_LAYOUT_FLEX);
    lv_obj_clear_flag(text_layout, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(text_layout, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(text_layout, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(text_layout, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(text_layout, 1);
    lv_obj_set_style_pad_all(text_layout, 0, 0);
    lv_obj_set_style_pad_row(text_layout, 0, 0);

    if (item->text != NULL && item->text[0] != '\0')
    {
        lv_obj_t *lb_title = lv_label_create(text_layout);
        lv_obj_clear_flag(lb_title, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_width(lb_title, LV_PCT(100));
        lv_label_set_text(lb_title, item->text);
        lv_obj_set_style_text_color(lb_title, ZV_COLOR_TEXT_MAIN, 0);
        lv_obj_set_style_text_align(lb_title, LV_TEXT_ALIGN_LEFT, 0);
    }

    if (item->subtitle != NULL && item->subtitle[0] != '\0')
    {
        lv_obj_t *lb_subtitle = lv_label_create(text_layout);
        lv_obj_clear_flag(lb_subtitle, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_width(lb_subtitle, LV_PCT(100));
        lv_label_set_text(lb_subtitle, item->subtitle);
        lv_obj_set_style_text_color(lb_subtitle, ZV_COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_align(lb_subtitle, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(lb_subtitle, &lv_font_montserrat_10, 0);
    }

     if (item->right_badge.type == BAGE_IMG_TYPE)
    {
        obj_icon_t right_icon = item->right_badge.icon;
        if (right_icon.path != NULL && right_icon.path[0] != '\0')
        {
            lv_obj_t *icon = lv_img_create(content);
            lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_size(icon, right_icon.size.width, right_icon.size.height);

            char lv_path[1024];
            snprintf(lv_path, sizeof(lv_path), "A:%s", right_icon.path);

            lv_img_set_src(icon, lv_path);
            lv_image_set_scale(icon, LV_SCALE_NONE * 0.3);
        }
    }
    else if(item->right_badge.type == BAGE_TEXT_TYPE)
    {
        lv_obj_t *text = lv_label_create(content);
        lv_obj_clear_flag(text, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(text, item->right_badge.label);
        lv_color_t color = item->right_badge.has_text_color
                               ? item->right_badge.text_color
                               : ZV_COLOR_TEXT_MAIN;
        lv_obj_set_style_text_color(text, color, 0);
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    }

    list->item_count += 1;
    return btn;
}

void set_event_data(ui_list *list, ui_list_item_event_cb_t cb, void *user_data)
{
    list->user_data = user_data;
    list->cb = cb;
}

void set_list_border(ui_list *list, bool enabled)
{
    if (!list || !list->list)
        return;

    lv_obj_set_style_border_width(list->list, enabled ? 1 : 0, 0);
}

void set_list_bg_color(ui_list *list, lv_color_t color)
{
    if (!list || !list->list)
        return;

    lv_obj_set_style_bg_color(list->list, color, 0);
}

int item_length(ui_list *list)
{
    return list->item_count;
}

void clean_list(ui_list *list)
{
    if (!list || !list->list)
        return;

    lv_obj_clean(list->list);

    list->item_count = 0;
}

void destroy_list(ui_list *list)
{
    clean_list(list);
    free(list);
}
