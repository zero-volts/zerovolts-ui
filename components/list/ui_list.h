#ifndef UI_LIST_H
#define UI_LIST_H

#include <stdbool.h>

#include "lvgl.h"
#include "components/ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BAGE_NONE = 0,
    BAGE_IMG_TYPE,
    BAGE_TEXT_TYPE
} bage_type;

typedef struct {
    const char *label;
    bage_type type;
    obj_icon_t icon;
} list_item_bage_t;

typedef struct {
    const char *text;
    const char *subtitle;
    list_item_bage_t left_bage;
    list_item_bage_t right_bage;

    const char *raw_value;
    void *user_data;
} list_item_t;

typedef struct ui_list ui_list;
typedef void (*ui_list_item_event_cb_t)(ui_list *list, const list_item_t *item, void *user_data);

ui_list *create_list(lv_obj_t *parent, int width, int height);
lv_obj_t *add_item(ui_list *list, const list_item_t *item);
void set_event_data(ui_list *list, ui_list_item_event_cb_t cb, void *user_data);
void set_list_border(ui_list *list, bool enabled);
void set_list_bg_color(ui_list *list, lv_color_t color);
int item_length(ui_list *list);
void clean_list(ui_list *list);
void destroy_list(ui_list *list);

#ifdef __cplusplus
}
#endif

#endif /* UI_LIST_H */
