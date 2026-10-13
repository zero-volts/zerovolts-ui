#include "ir.h"
#include "ir_controller.h"
#include "utils/logger.h"
#include <stdio.h>
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "page/navigation_page.h"
#include "page/ir/new_remote.h"
#include "page/ir/learn_button.h"
#include "page/ir/send_signal.h"
#include "page/ir/remotes.h"

static void ir_menu_handler(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;
    (void)user_data;

    zv_nav_open((const nav_ctx_t *)item->user_data);
}

static void ir_view_delete_cb(lv_event_t *e)
{
    (void)e;
    ir_controller_deinit();
}

zv_view_t *ir_page_create(const zv_config *cfg)
{
    if (!cfg)
        return NULL;

    ir_remote_ctx ir_cfg = {};
    ir_cfg.remotes_root = (char *)cfg->ir.remotes_path;
    ir_cfg.ir_ctx.backend = cfg->ir.backend;
    snprintf(ir_cfg.ir_ctx.tx_dev, sizeof(ir_cfg.ir_ctx.tx_dev), "%s", cfg->ir.tx_device);
    snprintf(ir_cfg.ir_ctx.rx_dev, sizeof(ir_cfg.ir_ctx.rx_dev), "%s", cfg->ir.rx_device);
    ir_cfg.ir_ctx.timeout_ms = cfg->ir.learn_timeout_ms;

    if (ir_controller_init(&ir_cfg) != IR_OK)
    {
        log_error("IR init failed: %s\n", ir_controller_last_error());
        return NULL;
    }

    zv_view_t *view = zv_view_new("Infrared");
    if (!view)
    {
        ir_controller_deinit();
        return NULL;
    }

    lv_obj_add_event_cb(view->main_container, ir_view_delete_cb, LV_EVENT_DELETE, NULL);
    nav_ctx_t nav_remotes = { ir_remotes_page_create, cfg };
    nav_ctx_t nav_new_remote = { ir_new_remote_page_create, cfg };
    nav_ctx_t nav_learn_button = { ir_learn_button_page_create, cfg };
    nav_ctx_t nav_send_signal = { ir_send_signal_page_create, cfg };

    ui_list *list = create_list(view->content, 100, 100);
    if (!list)
    {
        zv_view_destroy(view);
        return NULL;
    }

    set_list_border(list, false);
    set_list_bg_color(list, COLOR_BG_MAIN);
    set_event_data(list, ir_menu_handler, NULL);

    list_item_t items[] = {
        {
            .text = "Remotes",
            .subtitle = "Saved remotes",
            .left_badge = { 
                .type = BADGE_IMG_TYPE,
                .icon = {
                    .path = "icons/remote_control.png",
                    .size = {
                        .width = 60,
                        .height = 60,
                        .scale = 5
                    }
                }
            },
            .user_data = &nav_remotes,
            .user_data_size = sizeof(nav_ctx_t),
        },
        {
            .text = "New Remote",
            .subtitle = "Create a new remote",
            .left_badge = { .label = LV_SYMBOL_PLUS, .type = BADGE_TEXT_TYPE },
            .user_data = &nav_new_remote,
            .user_data_size = sizeof(nav_ctx_t),
        },
        {
            .text = "Learn Button",
            .subtitle = "Capture a new signal",
            .left_badge = { 
                .type = BADGE_IMG_TYPE,
                .icon = {
                    .path = "icons/ir_learn.png",
                    .size = {
                        .width = 60,
                        .height = 60,
                        .scale = 5
                    }
                }
            },
            .user_data = &nav_learn_button,
            .user_data_size = sizeof(nav_ctx_t),
        },
        {
            .text = "Send Signal",
            .subtitle = "Transmit a saved signal",
            .left_badge = { 
                .type = BADGE_IMG_TYPE,
                .icon = {
                    .path = "icons/ir_send_signal.png",
                    .size = {
                        .width = 60,
                        .height = 60,
                        .scale = 5
                    }
                }
            },
            .user_data = &nav_send_signal,
            .user_data_size = sizeof(nav_ctx_t),
        },
    };

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
    {
        add_item(list, &items[i]);
    }

    return view;
}
