#include "home_view.h"

#include "components/component_helper.h"

home_view *home_view_create(home_view *self, lv_obj_t *menu, const zv_config *cfg, home_item *items, size_t item_count)
{
    if (!self || !menu || !items || item_count == 0)
        return NULL;

    if (!zv_view_create(&self->base, menu, NULL))
        return NULL;

    self->base.set_flex_layout(&self->base, LV_FLEX_FLOW_ROW_WRAP, 16, 14);

    for (size_t i = 0; i < item_count; i++) 
    {
        home_item *item = &items[i];
        if (!item->create_page)
            continue;

        item->nav.menu = menu;
        item->nav.page = item->create_page(menu, cfg);
        if (!item->nav.page)
            continue;

        lv_obj_t *btn = create_square_main_button(self->base.root, item->label, item->icon, zv_goto_page_cb, &item->nav);
        if (btn && item->rotate_icon_90)
            rotate_icon_by_tag(btn, 90);
    }

    return self;
}

lv_obj_t *get_page(home_view *self)
{
    return self->base.page;
}
