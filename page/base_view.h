#ifndef BASE_VIEW_H
#define BASE_VIEW_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TITLE_LENGTH 30

typedef struct base_view base_view;

struct base_view {
    lv_obj_t *page;
    lv_obj_t *root;

    char title[MAX_TITLE_LENGTH];

    void (*change_title)(base_view *self, const char *new_title);
    void (*set_flex_layout)(base_view *self, lv_flex_flow_t flow, lv_coord_t pad_all, lv_coord_t pad_row);
    void (*set_scrollable)(base_view *self, bool scrollable);
};

base_view *zv_view_create(base_view *self, lv_obj_t *menu, const char *title);
void zv_view_change_title(base_view *self, const char *new_title);
void zv_view_set_flex_layout(base_view *self, lv_flex_flow_t flow, lv_coord_t pad_all, lv_coord_t pad_row);
void zv_view_set_scrollable(base_view *self, bool scrollable);


#ifdef __cplusplus
}
#endif

#endif /* BASE_VIEW_H */
