#include "bt_view.h"
#include "bt_scanner.h"
#include "components/ui_theme.h"
#include "components/component_helper.h"
#include "components/list/ui_list.h"
#include "components/nav.h"
#include <stdio.h>

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    nav_ctx_t *ctx = (nav_ctx_t *)user_data;
    if (!ctx || !ctx->menu || !ctx->page)
        return;

    lv_menu_set_page(ctx->menu, ctx->page);
    zv_nav_update_group(ctx->menu, ctx->page);
}

lv_obj_t *bt_page_create(lv_obj_t *menu, const zv_config *cfg)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Bluetooth");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);

    (void)cfg;

    lv_obj_t *scanner_page = bt_scanner_page_create(menu);

    static nav_ctx_t nav_scanner;

    nav_scanner.menu = menu;
    nav_scanner.page = scanner_page;

    ui_list *list = create_list(page, 100, 80);
    set_list_border(list, false);
    set_list_bg_color(list, ZV_COLOR_BG_MAIN);
    set_event_data(list, handler, &nav_scanner);

    list_item_t item = {
        .text = "Scanner",
        .subtitle= "Discover devices",
        .left_badge = {
            .type = BADGE_IMG_TYPE,
            .icon = {
                .path = "icons/scaner.png",
                .size = {
                    .width = 50,
                    .height = 50,
                    .scale = 4
                }
            }
        },
    };

    add_item(list, &item);

    return page;
}
