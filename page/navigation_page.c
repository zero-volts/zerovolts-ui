#include "navigation_page.h"

#include "zv_view.h"
#include "utils/logger.h"

static zv_nav_stack_t screen_stack[ZV_SCREEN_MAX_DEEP];
static int stack_top = -1;

bool zv_nav_push(zv_view_t *new_view)
{
    if (!new_view || !new_view->main_container ||
        (stack_top + 1) >= ZV_SCREEN_MAX_DEEP)
        return false;

    // hiding previous screen
    if (stack_top >= 0 && screen_stack[stack_top].view->main_container != NULL)
        lv_obj_add_flag(screen_stack[stack_top].view->main_container, LV_OBJ_FLAG_HIDDEN);

    zv_nav_stack_t stack_view = {
        .id = stack_top + 1,
        .view = new_view
    };

    screen_stack[stack_top + 1] = stack_view;
    stack_top++;
    lv_obj_remove_flag(new_view->main_container, LV_OBJ_FLAG_HIDDEN);
    zv_nav_update_group(screen_stack[stack_top].view->main_container);
    return true;
}

bool zv_nav_pop(void)
{
    if (stack_top <= 0)
        return false;

    zv_view_destroy(screen_stack[stack_top].view);
    screen_stack[stack_top].view = NULL;

    stack_top--;
    if (screen_stack[stack_top].view != NULL)
        lv_obj_remove_flag(screen_stack[stack_top].view->main_container, LV_OBJ_FLAG_HIDDEN);

    zv_nav_update_group(screen_stack[stack_top].view->main_container);
    return true;
}

bool zv_nav_is_empty(void)
{
    return stack_top < 0;
}

static lv_group_t *g_nav_group = NULL;

static bool obj_is_focusable(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj))
        return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN))
        return false;
    if (lv_obj_has_state(obj, LV_STATE_DISABLED))
        return false;
    // Plain layout containers are not navigation controls.
    if (lv_obj_check_type(obj, &lv_obj_class))
        return false;
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
        return true;
    if (lv_obj_is_editable(obj))
        return true;

    return false;
}

static void group_add_focusables(lv_group_t *group, lv_obj_t *root)
{
    if (!group || !root || lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN))
        return;

    if (obj_is_focusable(root))
        lv_group_add_obj(group, root);

    uint32_t n = lv_obj_get_child_count(root);
    for (uint32_t i = 0; i < n; i++)
    {
        lv_obj_t *child = lv_obj_get_child(root, i);
        group_add_focusables(group, child);
    }
}

void zv_nav_set_group(lv_group_t *group)
{
    g_nav_group = group;
}

void zv_nav_update_group(lv_obj_t *page)
{
    if (!g_nav_group || !page)
        return;

    lv_group_set_editing(g_nav_group, false);
    lv_group_remove_all_objs(g_nav_group);

    group_add_focusables(g_nav_group, page);

    lv_obj_t *first = lv_group_get_obj_by_index(g_nav_group, 0);
    if (first)
        lv_group_focus_obj(first);
}

bool zv_nav_open(const nav_ctx_t *ctx)
{
    if (!ctx || !ctx->create_view)
        return false;

    zv_view_t *view = ctx->create_view(ctx->cfg);
    if (!view)
    {
        log_error("Could not create destination view\n");
        return false;
    }

    if (!zv_nav_push(view)) 
    {
        zv_view_destroy(view);
        log_error("Could not push destination view\n");
        return false;
    }

    return true;
}

void zv_nav_back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        zv_nav_pop();
}
