#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <stdio.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bt_context_t bt_context_t;
typedef struct {
    bt_context_t *bt;
} app_context_t;

app_context_t *app_context_get();

void bt_context_add_device(device_t *device);
void bt_context_clear_devices(void);
device_t *bt_context_get_devices(void);
void bt_context_set_selected(device_t *device);
const device_t *bt_context_get_selected(void);
int bt_context_devices_length(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONTEXT_H */