#include "ui_dropdown.h"
#include <stdlib.h>
#include <string.h>

#include "utils/logger.h"
#include "components/ui_theme.h"

struct ui_dropdown {
    lv_obj_t *dropdown;
    int item_count;
    dropdown_item_t *items;
    ui_dropdown_item_change_cb on_change;
    void *user_data;
};

static void dropdown_release_items(ui_dropdown *dropdown)
{
    for (int i = 0; i < dropdown->item_count; i++)
        free((void *)dropdown->items[i].text);

    free(dropdown->items);
    dropdown->items = NULL;
    dropdown->item_count = 0;
}

static void dropdown_delete_handler(lv_event_t *e)
{
    ui_dropdown *dropdown = (ui_dropdown *)lv_event_get_user_data(e);
    if (!dropdown)
        return;

    dropdown_release_items(dropdown);
    free(dropdown);
}

static void dropdown_changed_cb(lv_event_t *e)
{
    ui_dropdown *self = (ui_dropdown *)lv_event_get_user_data(e);
    if (!self || !self->dropdown)
        return;

    uint32_t index = lv_dropdown_get_selected(self->dropdown);

    if (self->on_change != NULL)
        self->on_change(&self->items[index], self->user_data);
}

ui_dropdown *dropdown_create(lv_obj_t *parent, int width, int height)
{
    ui_dropdown *dropdown_box = (ui_dropdown *)calloc(1, sizeof(ui_dropdown));
    if (!dropdown_box)
        return NULL;

    lv_obj_t *obj = lv_dropdown_create(parent);
    if (!obj)
    {
        free(dropdown_box);
        return NULL;
    }

    lv_dropdown_clear_options(obj);

    lv_obj_set_width(obj, width);
    lv_obj_set_height(obj, height);
    lv_obj_set_style_bg_color(obj, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_text_color(obj, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_pad_left(obj, 10, 0);

    lv_obj_add_event_cb(obj, dropdown_changed_cb, LV_EVENT_VALUE_CHANGED, dropdown_box);

    dropdown_box->dropdown = obj;
    dropdown_box->item_count = 0;
    dropdown_box->user_data = NULL;
    dropdown_box->items = NULL;
    dropdown_box->on_change = NULL;

    return dropdown_box;
}

void dropdown_set_on_change_cb(ui_dropdown *dropdown, ui_dropdown_item_change_cb callback, void *user_data)
{
    if (!dropdown)
        return;

    dropdown->on_change = callback;
    dropdown->user_data = user_data;
}

dropdown_item_t *dropdown_get_item(ui_dropdown *dropdown, int position)
{
    if (!dropdown || position < 0 || position >= dropdown->item_count)
        return NULL;

    return &dropdown->items[position];
}

void dropdown_add_item(ui_dropdown *dropdown, const dropdown_item_t *item)
{
    if (!dropdown || !item)
        return;

    if (!item->text)
    {
        log_error("[UI dropdown][dropdown_add_item] The item text is null!");
        return;
    }

    size_t text_size = strlen(item->text) + 1;
    char *text_copy = (char *)malloc(text_size);
    if (!text_copy)
        return;

    memcpy(text_copy, item->text, text_size);

    dropdown_item_t item_copy = *item;
    dropdown_item_t *new_items = (dropdown_item_t *)realloc(dropdown->items, (dropdown->item_count + 1) * sizeof(*new_items));
    if (!new_items)
    {
        free(text_copy);
        return;
    }

    dropdown->items = new_items;
    new_items[dropdown->item_count] = item_copy;
    new_items[dropdown->item_count].text = text_copy;

    lv_dropdown_add_option(dropdown->dropdown, text_copy, item_copy.position);

    dropdown->item_count++;
}

void dropdown_set_items(ui_dropdown *dropdown, const dropdown_item_t *items, size_t amount)
{
    if (!dropdown || !items)
        return;

    for (size_t index = 0; index < amount; index++)
    {
        dropdown_add_item(dropdown, &items[index]);
    }

    lv_dropdown_set_selected(dropdown->dropdown, 0);
}

void dropdown_set_selected_item(ui_dropdown *dropdown, int selected_index)
{
    lv_dropdown_set_selected(dropdown->dropdown, selected_index);
}

void dropdown_clean_items(ui_dropdown *dropdown)
{
    if (!dropdown)
        return;

    lv_dropdown_clear_options(dropdown->dropdown);
    
    dropdown_release_items(dropdown);
}

void dropdown_destroy(ui_dropdown *dropdown)
{
    if (dropdown)
        lv_obj_delete(dropdown->dropdown);
}