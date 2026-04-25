#ifndef BT_VIEW_H
#define BT_VIEW_H

#include "lvgl.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *bt_page_create(lv_obj_t *menu, const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* BT_VIEW_H */
