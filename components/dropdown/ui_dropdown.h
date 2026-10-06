#ifndef UI_DROPDOWN_H
#define UI_DROPDOWN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int position;
    uint32_t value;
    const char *text;
} dropdown_item_t;

typedef void (*ui_dropdown_item_change_cb)(dropdown_item_t *item, void *user_data);
typedef struct ui_dropdown ui_dropdown;

ui_dropdown *dropdown_create(lv_obj_t *parent, int width, int height);
void dropdown_set_on_change_cb(ui_dropdown *dropdown, ui_dropdown_item_change_cb callback, void *user_data);
dropdown_item_t *dropdown_get_item(ui_dropdown *dropdown, int position);
void dropdown_add_item(ui_dropdown *dropdown, const dropdown_item_t *item);
void dropdown_set_items(ui_dropdown *dropdown, const dropdown_item_t *items, size_t amount);
void dropdown_set_selected_item(ui_dropdown *dropdown, int selected_index);
void dropdown_clean_items(ui_dropdown *dropdown);
void dropdown_destroy(ui_dropdown *dropdown);


#ifdef __cplusplus
}
#endif

#endif /* UI_DROPDOWN_H */
