#ifndef HOME_VIEW_H
#define HOME_VIEW_H

#include "base_view.h"
#include "components/nav.h"
#include "config.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef lv_obj_t *(*home_page_create_cb)(lv_obj_t *menu, const zv_config *cfg);

typedef struct {
    const char *label;
    const char *icon;
    home_page_create_cb create_page;
    bool rotate_icon_90;
    nav_ctx_t nav;
} home_item;

typedef struct {
    base_view base;
} home_view;

lv_obj_t *get_page(home_view *self);
home_view *home_view_create(home_view *self, lv_obj_t *menu, const zv_config *cfg, 
        home_item *items, size_t item_count);

#ifdef __cplusplus
}
#endif

#endif /* HOME_VIEW_H */
