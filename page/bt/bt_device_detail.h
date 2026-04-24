#ifndef BT_DEVICE_DETAIL_H
#define BT_DEVICE_DETAIL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *bt_device_detail_page_create(lv_obj_t *menu);
void bt_device_detail_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_DEVICE_DETAIL_H */
