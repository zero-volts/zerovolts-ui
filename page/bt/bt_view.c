#include "bt_view.h"
#include "bt_controller.h"
#include "utils/logger.h"
#include "bt_scanner.h"
#include "components/ui_theme.h"
#include "components/component_helper.h"
#include "components/list/ui_list.h"
#include "page/navigation_page.h"
#include <stdio.h>

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;
    (void)user_data;
    zv_nav_open((const nav_ctx_t *)item->user_data);
}

zv_view_t *bt_page_create(const zv_config *cfg)
{
    if (!cfg)
        return NULL;

    uart_config_t uart_cfg = {};
    snprintf(uart_cfg.device, sizeof(uart_cfg.device), "%s", cfg->uart.device);
    uart_cfg.baudrate = cfg->uart.baudrate;
    if (bt_controller_init(&uart_cfg) != UART_OK)
    {
        log_error("Bluetooth init failed\n");
        return NULL;
    }

    zv_view_t *view = zv_view_new("Bluetooth");
    if (!view)
        return NULL;

    nav_ctx_t nav_scanner = { bt_scanner_page_create, cfg };
    ui_list *list = create_list(view->content, 100, 100);
    if (!list)
    {
        zv_view_destroy(view);
        return NULL;
    }

    set_list_border(list, false);
    set_list_bg_color(list, COLOR_BG_MAIN);
    set_event_data(list, handler, NULL);

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
        .user_data = &nav_scanner,
        .user_data_size = sizeof(nav_ctx_t),
    };

    add_item(list, &item);

    return view;
}
