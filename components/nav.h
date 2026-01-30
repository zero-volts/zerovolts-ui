#ifndef ZV_NAV_H
#define ZV_NAV_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *menu;
    lv_obj_t *page;
} nav_ctx_t;

void zv_goto_page_cb(lv_event_t *e);
void zv_nav_set_group(lv_group_t *group);
void zv_nav_update_group(lv_obj_t *menu, lv_obj_t *page);

#ifdef __cplusplus
}
#endif

#endif /* ZV_NAV_H */
