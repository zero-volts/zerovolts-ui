#ifndef BT_CONTROLLER_H
#define BT_CONTROLLER_H

#include "service/uart_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[32];
    char mac[18];
    int rssi;
    char manufacturer[32];
    char appearance[32];
    char service[32];
    int connectable;
} device_t;

typedef void (*bt_event_cb)(const char *event, device_t *device);

uart_status_t bt_controller_init(); // TODO: revisar si necesitamos mas datos
uart_status_t start_scan();
void set_scanner_cb(bt_event_cb new_callback);

#ifdef __cplusplus
}
#endif

#endif /* BT_CONTROLLER_H */
