#include "sub_ghz_view.h"
#include "sub_ghz_capture.h"
#include "page/navigation_page.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;
    (void)user_data;
    zv_nav_open((const nav_ctx_t *)item->user_data);
}

zv_view_t *subghz_view_create(const zv_config *cfg)
{
    if (subghz_controller_init(cfg) != SUBGHZ_OK)
        return NULL;

    zv_view_t *view = zv_view_new("Sub-GHz");
    if (!view)
        return NULL;

    nav_ctx_t nav_capture = { subghz_capture_page_create, cfg };
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
        .text = "Signal Capture",
        .subtitle = "Record RF signal",
        .user_data = &nav_capture,
        .user_data_size = sizeof(nav_capture),
    };

    add_item(list, &item);
    return view;
}
