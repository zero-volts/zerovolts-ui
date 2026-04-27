#include "bt_controller.h"
#include "utils/error_handler.h"
#include "utils/logger.h"
#include "service/uart_commands.h"
#include "app_context.h"

#include <string.h>
#include <stdlib.h>
#define UART_DEVICE "/dev/ttyAMA5"
#define UART_BAUDRATE 115200
#define UART_BT_TAG_ID "BT_TAG_CONTROLLER"

typedef struct {
    device_t devices[BT_ALLOWD_MAX_DEVICES];
    int amount;
} list_items_ctx;

static list_items_ctx local_ctx;
static scanner_handler internal_cb = NULL;

void set_scanner_cb(scanner_handler new_callback)
{
    internal_cb = new_callback;
}

static device_t parse_device(char *buffer)
{
    char *saveptr;
    char *token = strtok_r(buffer, "|", &saveptr);

    device_t device = {0};
    while (token != NULL)
    {
        //log_info("token: %s\n", token);
        if (strcmp(token, BT_COMMAND_RES_SCANN_DEVICE) == 0) {
            token = strtok_r(NULL, "|", &saveptr);
            continue;
        }

        char *innersaveptr;
        char *key = strtok_r(token, "=", &innersaveptr);
        char *value = strtok_r(NULL, "=", &innersaveptr);
        if (key && value)
        {
            if (strcmp(key, "name") == 0) {
                snprintf(device.name, sizeof(device.name), "%s", value);
            } else if (strcmp(key, "mac") == 0) {
                snprintf(device.mac, sizeof(device.mac), "%s", value);
            } else if (strcmp(key, "rssi") == 0) {
                device.rssi = atoi(value);
            } else if (strcmp(key, "manufacturer") == 0) {
                snprintf(device.manufacturer, sizeof(device.manufacturer), "%s", value);
            } else if (strcmp(key, "service") == 0) {
                snprintf(device.service, sizeof(device.service), "%s", value);
            }else if (strcmp(key, "appearance") == 0) {
                snprintf(device.appearance, sizeof(device.appearance), "%s", value);
            }
            else if (strcmp(key, "connectable") == 0) {
                device.connectable = atoi(value);
            }
        }

        token = strtok_r(NULL, "|", &saveptr);
    }

    return device;
}

static void event_handler(const char *tag_id, char *buffer)
{
    if (strcmp(tag_id, UART_BT_TAG_ID) != 0) {
        return;
    }

    if (strstr(buffer, BT_COMMAND_RES_SCANN_START) != NULL)
    {
        internal_cb(NULL, UI_LOADING);
        return;
    }

    if (strstr(buffer, BT_COMMAND_RES_SCANN_DONE) != NULL)
    {
        internal_cb(NULL, UI_DONE);
        return;
    }

    if (strstr(buffer, BT_COMMAND_RES_SCANN_DEVICE) == NULL) {
        return;
    }

    device_t device = parse_device(buffer);
    if (internal_cb != NULL)
    {
        internal_cb(&device, UI_LOADING);
        bt_context_add_device(&device);
    }
}

uart_status_t bt_controller_init()
{
    uart_status_t uart_rc = uart_service_init(UART_DEVICE, UART_BAUDRATE);
    if (uart_rc != UART_OK)
    {
        log_warning("UART init failed: %s\n", last_error());
        return uart_rc;
    }

    add_event_callback(event_handler, UART_BT_TAG_ID);

    return UART_OK;
}

uart_status_t start_scan()
{
    uart_status_t uart_rc = uart_send_line("SCAN");
    if (uart_rc != UART_OK) {
        log_warning("start_scan error: %s\n", last_error());
        return uart_rc;
    }

    return UART_OK;
}

void bt_reset_visible_devices(void)
{
    local_ctx.amount = 0;

    device_t *devices = bt_context_get_devices();
    int current_devices = bt_context_devices_length();
    for (int i = 0; i < current_devices; i++) {
        local_ctx.devices[local_ctx.amount++] = devices[i];
    }
}

void bt_apply_connectable_filter(void)
{
    bt_reset_visible_devices();

    int write_index = 0;
    for (int i = 0; i < local_ctx.amount; i++)
    {
        device_t *device = &local_ctx.devices[i];
        if (device->connectable) {
            local_ctx.devices[write_index++] = *device;
        }
    }

    local_ctx.amount = write_index;
}

int bt_compare_device_ptrs_by_rssi_desc(const void *a, const void *b)
{
    const device_t *dev_a = (const device_t *)a;
    const device_t *dev_b = (const device_t *)b;

    if (dev_a->rssi < dev_b->rssi) return 1;
    if (dev_a->rssi > dev_b->rssi) return -1;
    
    return 0;
}

void bt_sort_visible_devices_by_nearest(void)
{
    bt_reset_visible_devices();

    qsort(
        local_ctx.devices,
        local_ctx.amount,
        sizeof(device_t),
        bt_compare_device_ptrs_by_rssi_desc
    );
}

device_t *bt_get_visible_devices(void)
{
    return local_ctx.devices;
}

int bt_get_visible_devices_length(void)
{
    return local_ctx.amount;
}
