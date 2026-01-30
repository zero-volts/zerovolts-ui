#include "components/nav.h"

static lv_group_t *g_nav_group = NULL;

static bool obj_is_focusable(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj))
        return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN))
        return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED))
        return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
        return true;
    if (lv_obj_is_editable(obj))
        return true;
    return false;
}

static void group_add_focusables(lv_group_t *group, lv_obj_t *root)
{
    if (!group || !root)
        return;

    if (obj_is_focusable(root))
        lv_group_add_obj(group, root);

    uint32_t n = lv_obj_get_child_count(root);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *child = lv_obj_get_child(root, i);
        group_add_focusables(group, child);
    }
}

void zv_nav_set_group(lv_group_t *group)
{
    g_nav_group = group;
}

void zv_nav_update_group(lv_obj_t *menu, lv_obj_t *page)
{
    if (!g_nav_group || !page)
        return;

    lv_group_remove_all_objs(g_nav_group);

    if (menu) {
        lv_obj_t *header = lv_menu_get_main_header(menu);
        if (header)
            group_add_focusables(g_nav_group, header);
    }

    group_add_focusables(g_nav_group, page);

    lv_obj_t *first = lv_group_get_obj_by_index(g_nav_group, 0);
    if (first)
        lv_group_focus_obj(first);
}

void zv_goto_page_cb(lv_event_t *e)
{
    nav_ctx_t *ctx = (nav_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->menu || !ctx->page) 
        return;

    lv_menu_set_page(ctx->menu, ctx->page);
    zv_nav_update_group(ctx->menu, ctx->page);
}
