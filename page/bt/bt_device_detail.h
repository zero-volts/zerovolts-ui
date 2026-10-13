#ifndef BT_DEVICE_DETAIL_H
#define BT_DEVICE_DETAIL_H

#include "page/zv_view.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *bt_device_detail_page_create(const zv_config *cfg);
void bt_device_detail_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_DEVICE_DETAIL_H */
