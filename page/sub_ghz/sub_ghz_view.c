#include "sub_ghz_view.h"
#include "sub_ghz_capture.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "components/nav.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "service/subghz/subghz_service.h"

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    nav_ctx_t *ctx = (nav_ctx_t *)user_data;
    if (!ctx || !ctx->menu || !ctx->page)
        return;

    lv_menu_set_page(ctx->menu, ctx->page);
    zv_nav_update_group(ctx->menu, ctx->page);
}

static subghz_view *subghz_page_create(subghz_view *self, lv_obj_t *menu, const zv_config *cfg)
{
    base_view *view = zv_view_create(&self->base, menu, "Sub-GHZ");
    if (!view)
        return NULL;

    lv_obj_set_style_pad_all(self->base.root, 8, 0);
    self->base.set_flex_layout(&self->base, LV_FLEX_FLOW_COLUMN, 5, 0);

    memset(&self->config, 0, sizeof(self->config));
    sprintf(self->config.signals_path, cfg->subghz.signals_path);

    if (subghz_controller_init(cfg) != SUBGHZ_OK)
        return NULL;

    lv_obj_t *capture_page = subghz_capture_page_create(menu);
    static nav_ctx_t nav_scanner;

    nav_scanner.menu = menu;
    nav_scanner.page = capture_page;

    ui_list *list = create_list(self->base.root, 100, 80);
    set_list_border(list, false);
    set_list_bg_color(list, ZV_COLOR_BG_MAIN);
    set_event_data(list, handler, &nav_scanner);

    list_item_t item = {
        .text = "Signal Capture",
        .subtitle= "Record RF signal"
    };

    add_item(list, &item);

    return self;
}


lv_obj_t *subghz_view_create(lv_obj_t *menu, const zv_config *cfg)
{
    subghz_view *view = (subghz_view *)malloc(sizeof(*view));
    if (!view)
        return NULL;

    subghz_view *self = subghz_page_create(view, menu, cfg);
    if (!self)
    {
        free(view);
        return NULL;
    }

    return self->base.page;
}
