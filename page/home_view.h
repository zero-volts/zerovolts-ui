#ifndef HOME_VIEW_H
#define HOME_VIEW_H

#include "zv_view.h"
#include "navigation_page.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef zv_view_create_cb home_view_create_cb;

typedef struct {
    const char *label;
    const char *icon;
    bool rotate_icon_90;
    home_view_create_cb create_view;
} home_item;

lv_obj_t *get_page(zv_view_t *self);
zv_view_t *home_view_create(const home_item *items, size_t item_count, const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* HOME_VIEW_H */
