#include "components/nav.h"

void zv_goto_page_cb(lv_event_t *e)
{
    nav_ctx_t *ctx = (nav_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->menu || !ctx->page) 
        return;

    lv_menu_set_page(ctx->menu, ctx->page);
}
