#ifndef BT_VIEW_H
#define BT_VIEW_H

#include "page/zv_view.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *bt_page_create(const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* BT_VIEW_H */
