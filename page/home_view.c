#include "home_view.h"

#include "components/component_helper.h"
#include "components/ui_theme.h"
#include "page/navigation_page.h"
#include "utils/logger.h"
#include <stdlib.h>

typedef struct {
    home_view_create_cb create_view;
    const zv_config *cfg;
} home_item_ctx;

static void home_item_delete_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

static void home_item_event_cb(lv_event_t *e)
{
    const home_item_ctx *ctx = (const home_item_ctx *)lv_event_get_user_data(e);
    if (!ctx || !ctx->create_view)
        return;

    nav_ctx_t nav = { ctx->create_view, ctx->cfg };
    zv_nav_open(&nav);
}

zv_view_t *home_view_create(const home_item *items, size_t item_count, const zv_config *cfg)
{
    if (!items || item_count == 0)
        return NULL;

    zv_view_t *base = zv_view_new(NULL);
    if (!base)
        return NULL;

    lv_obj_t *root = base->main_container;

    lv_obj_set_height(root, 0);
    lv_obj_set_flex_grow(root, 1);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content = base->content;
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(content, 16, 0);
    lv_obj_set_style_pad_row(content, 14, 0);

    for (size_t i = 0; i < item_count; i++)
    {
        const home_item *item = &items[i];
        home_item_ctx *ctx = (home_item_ctx *)malloc(sizeof(*ctx));
        if (!ctx)
        {
            zv_view_destroy(base);
            return NULL;
        }
        
        ctx->create_view = item->create_view;
        ctx->cfg = cfg;
        lv_obj_t *btn = create_square_main_button(content, item->label, item->icon, home_item_event_cb, ctx);
        if (!btn)
        {
            free(ctx);
            zv_view_destroy(base);
            return NULL;
        }

        lv_obj_add_event_cb(btn, home_item_delete_cb, LV_EVENT_DELETE, ctx);

        if (item->rotate_icon_90)
            rotate_icon_by_tag(btn, 90);
    }

    return base;
}

lv_obj_t *get_page(zv_view_t *self)
{
    return self ? self->main_container : NULL;
}
