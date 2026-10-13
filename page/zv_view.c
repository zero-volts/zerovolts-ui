#include "zv_view.h"

#include <stdlib.h>

#include "navigation_page.h"
#include "components/ui_theme.h"
#include "components/button/ui_button.h"

static void zv_view_back_event(event_data_btn *event)
{
    zv_nav_back_cb(event->lv_event);
}

static void zv_view_delete_event_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

static lv_obj_t *zv_view_create_header(lv_obj_t *parent, const char *title)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 45);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(header, COLOR_BG_MAIN, 0);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(header, 5, LV_PART_MAIN);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // A non-root view has a back button.
    if (!zv_nav_is_empty())
    {
        ui_button *back_btn = ui_icon_button_create(header, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_SYMBOL_LEFT);
        ui_button_set_on_click(back_btn, zv_view_back_event, NULL);        
    }

    lv_obj_t *title_label = lv_label_create(header);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT, 0);
    lv_label_set_text(title_label, title);

    return header;
}

zv_view_t *zv_view_new(const char *title)
{
    zv_view_t *new_view = (zv_view_t *)calloc(1, sizeof(zv_view_t));
    if (!new_view)
        return NULL;

    lv_obj_t *page = lv_obj_create(lv_screen_active());
    if (!page)
    {
        free(new_view);
        return NULL;
    }

    lv_obj_add_event_cb(page, zv_view_delete_event_cb, LV_EVENT_DELETE, new_view);
    lv_obj_set_width(page, lv_pct(100));
    lv_obj_set_height(page, 0);
    lv_obj_set_flex_grow(page, 1);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(page, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(page, COLOR_BG_MAIN, 0);
    lv_obj_set_style_radius(page, 0, LV_PART_MAIN);
    lv_obj_set_layout(page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);

    new_view->header = NULL;
    if (title != NULL)
        new_view->header = zv_view_create_header(page, title);

    new_view->content = lv_obj_create(page);
    lv_obj_set_layout(new_view->content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(new_view->content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(new_view->content, COLOR_BG_MAIN, 0);
    lv_obj_set_width(new_view->content, lv_pct(100));
    lv_obj_set_flex_grow(new_view->content, 1);
    lv_obj_set_style_pad_all(new_view->content, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(new_view->content, 0, LV_PART_MAIN);

    new_view->main_container = page;
    return new_view;
}

void zv_view_destroy(zv_view_t *view)
{
    if (!view)
        return;

    // Deleting the container also releases the view through LV_EVENT_DELETE.
    lv_obj_delete(view->main_container);
}
