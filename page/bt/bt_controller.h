#ifndef BT_CONTROLLER_H
#define BT_CONTROLLER_H

#include "service/uart_service.h"
#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bt_context_t bt_context_t;
typedef void (*scanner_handler)(device_t *device, ui_status_t status);
typedef void (*bt_conn_handler)(bt_conn_status_t status, const char *info);

uart_status_t bt_controller_init(const uart_config_t *config);
uart_status_t start_scan();
void set_scanner_cb(scanner_handler new_callback);

void bt_reset_visible_devices(void);
void bt_apply_connectable_filter(void);
int bt_compare_device_ptrs_by_rssi_desc(const void *a, const void *b);
void bt_sort_visible_devices_by_nearest(void);
device_t *bt_get_visible_devices(void);
int bt_get_visible_devices_length(void);

uart_status_t bt_connect(const device_t *device);
uart_status_t bt_disconnect(void);
void set_conn_cb(bt_conn_handler new_callback);
bt_conn_status_t bt_get_conn_status(void);

bt_service_t *bt_get_services(void);
int bt_get_services_length(void);
void bt_clear_discovery(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_CONTROLLER_H */
