#include "ir.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "components/nav.h"
#include "page/ir/new_remote.h"
#include "page/ir/learn_button.h"
#include "page/ir/send_signal.h"
#include "page/ir/remotes.h"

static void ir_menu_handler(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;
    (void)user_data;

    nav_ctx_t *ctx = (nav_ctx_t *)item->user_data;
    if (!ctx || !ctx->menu || !ctx->page)
        return;

    lv_menu_set_page(ctx->menu, ctx->page);
    zv_nav_update_group(ctx->menu, ctx->page);
}

lv_obj_t *ir_page_create(lv_obj_t *menu, const zv_config *cfg)
{
    (void)cfg;

    lv_obj_t *page = lv_menu_page_create(menu, "Infrared");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(page, ZV_COLOR_BG_MAIN, 0); 
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);

    lv_obj_t *remotes_page = ir_remotes_page_create(menu);
    lv_obj_t *new_remote_page = ir_new_remote_page_create(menu);
    lv_obj_t *learn_button_page = ir_learn_button_page_create(menu);
    lv_obj_t *send_signal_page = ir_send_signal_page_create(menu);

    static nav_ctx_t nav_remotes;
    static nav_ctx_t nav_new_remote;
    static nav_ctx_t nav_learn_button;
    static nav_ctx_t nav_send_signal;

    nav_remotes.menu = menu;
    nav_remotes.page = remotes_page;

    nav_new_remote.menu = menu;
    nav_new_remote.page = new_remote_page;

    nav_learn_button.menu = menu;
    nav_learn_button.page = learn_button_page;

    nav_send_signal.menu = menu;
    nav_send_signal.page = send_signal_page;

    ui_list *list = create_list(page, 100, 100);
    set_list_border(list, false);
    set_list_bg_color(list, ZV_COLOR_BG_MAIN);
    set_event_data(list, ir_menu_handler, NULL);

    list_item_t items[] = {
        {
            .text = "Remotes",
            .subtitle = "Saved remotes",
            .left_bage = { .label = LV_SYMBOL_HOME, .type = BAGE_TEXT_TYPE },
            .user_data = &nav_remotes,
        },
        {
            .text = "New Remote",
            .subtitle = "Create a new remote",
            .left_bage = { .label = LV_SYMBOL_PLUS, .type = BAGE_TEXT_TYPE },
            .user_data = &nav_new_remote,
        },
        {
            .text = "Learn Button",
            .subtitle = "Capture a new signal",
            .left_bage = { .label = LV_SYMBOL_SAVE, .type = BAGE_TEXT_TYPE },
            .user_data = &nav_learn_button,
        },
        {
            .text = "Send Signal",
            .subtitle = "Transmit a saved signal",
            .left_bage = { .label = LV_SYMBOL_GPS, .type = BAGE_TEXT_TYPE  },
            .user_data = &nav_send_signal,
        },
    };

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
        add_item(list, &items[i]);

    return page;
}
