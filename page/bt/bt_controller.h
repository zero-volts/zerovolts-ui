#ifndef BT_CONTROLLER_H
#define BT_CONTROLLER_H

#include "service/uart_service.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bt_context_t bt_context_t;
typedef void (*bt_event_cb)(const char *event, device_t *device);

uart_status_t bt_controller_init(); // TODO: check whether we need more data
uart_status_t start_scan();
void set_scanner_cb(bt_event_cb new_callback);

void bt_reset_visible_devices(void);
void bt_apply_connectable_filter(void);
int bt_compare_device_ptrs_by_rssi_desc(const void *a, const void *b);
void bt_sort_visible_devices_by_nearest(void);
device_t *bt_get_visible_devices(void);
int bt_get_visible_devices_length(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_CONTROLLER_H */
