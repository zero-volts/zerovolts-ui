#include "base_view.h"
#include "./../utils/string_utils.h"

#include <stdio.h>
#include <string.h>

#define DEFAULT_TITLE "Unknown Title"

base_view *zv_view_create(base_view *self, lv_obj_t *menu, const char *title)
{
    if (!self || !menu)
        return NULL;

    memset(self, 0, sizeof(*self));

    self->change_title = zv_view_change_title;
    self->set_flex_layout = zv_view_set_flex_layout;
    self->set_scrollable = zv_view_set_scrollable;

    if(zv_is_empty(title) == 0) {
        snprintf(self->title, sizeof(self->title), "%s", title);
    } 
    
    self->page = lv_menu_page_create(menu, self->title);
    if (!self->page)
        return NULL;

    lv_obj_set_scrollbar_mode(self->page, LV_SCROLLBAR_MODE_OFF);

    self->root = lv_obj_create(self->page);
    if (!self->root)
        return NULL;

    lv_obj_set_size(self->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(self->root, 0, 0);
    lv_obj_set_style_border_width(self->root, 0, 0);
    lv_obj_set_style_radius(self->root, 0, 0);
    lv_obj_set_style_bg_opa(self->root, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(self->root, LV_OBJ_FLAG_SCROLLABLE);

    return self;
}

void zv_view_change_title(base_view *self, const char *new_title)
{
    const char *title;

    if (!self)
        return;

    title = zv_is_empty(new_title) ? DEFAULT_TITLE : new_title;
    snprintf(self->title, sizeof(self->title), "%s", title);

    if (self->page)
        lv_menu_set_page_title(self->page, self->title);
}

void zv_view_set_flex_layout(base_view *self, lv_flex_flow_t flow, lv_coord_t pad_all, lv_coord_t pad_row)
{
    if (!self || !self->root)
        return;

    lv_obj_set_layout(self->root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(self->root, flow);
    lv_obj_set_style_pad_all(self->root, pad_all, 0);
    lv_obj_set_style_pad_row(self->root, pad_row, 0);
}

void zv_view_set_scrollable(base_view *self, bool scrollable)
{
    if (!self || !self->root)
        return;

    if (scrollable)
        lv_obj_add_flag(self->root, LV_OBJ_FLAG_SCROLLABLE);
    else
        lv_obj_clear_flag(self->root, LV_OBJ_FLAG_SCROLLABLE);
}
