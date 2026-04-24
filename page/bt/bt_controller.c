#include "bt_controller.h"
#include "utils/error_handler.h"
#include "utils/logger.h"

#include <string.h>
#include <stdlib.h>
#define UART_DEVICE "/dev/ttyAMA5"
#define UART_BAUDRATE 115200
#define UART_BT_TAG_ID "BT_TAG_CONTROLLER"

#define MAX_DEVICES 20
#define DEFAULT_NAME "Unknown"
static bt_event_cb internal_cb = NULL;

struct bt_context_t {
    device_t devices[MAX_DEVICES];
    int current_device_amount;
    device_t selected;

    device_t *visible_devices[MAX_DEVICES];
    int visible_device_amount;
};
static bt_context_t context;

void set_scanner_cb(bt_event_cb new_callback)
{
    internal_cb = new_callback;
}

static void event_handler(const char *tag_id, char *buffer)
{
    if (strcmp(tag_id, UART_BT_TAG_ID) != 0) {
        return;
    }

    if (strstr(buffer, "SCAN:START") != NULL)
    {
        internal_cb("SCAN:START", NULL);
        return;
    }

    if (strstr(buffer, "SCAN:DONE") != NULL)
    {
        internal_cb("SCAN:DONE", NULL);
        return;
    }

    // log_info("buffer handler: %s\n", buffer);
    if (strstr(buffer, "SCAN:DEVICE") == NULL) {
        return;
    }

    char *token;
    char *saveptr;

    token = strtok_r(buffer, "|", &saveptr);

    device_t device = {0};
    while (token != NULL)
    {
        //log_info("token: %s\n", token);
        if (strcmp(token, "SCAN:DEVICE") == 0) {
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

    if (internal_cb != NULL)
        internal_cb("SCAN:UPDATE", &device);
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


bt_context_t *bt_controller_ctx(void)
{
    return &context;
}

const device_t *selected_device(bt_context_t *ctx)
{
    if (ctx == NULL)
        return NULL;

    if (ctx->selected.mac[0] == '\0')
        return NULL;

    return &ctx->selected;
}

void bt_controller_select(const device_t *dev)
{
    if (dev == NULL)
    {
        memset(&context.selected, 0, sizeof(context.selected));
        return;
    }

    context.selected = *dev;
}

void bt_controller_clean_ctx(void)
{
    memset(&context.selected, 0, sizeof(context.selected));
}

static device_t *find_device(const char *mac)
{
    for (int index = 0; index < context.current_device_amount; index++)
    {
        if ( strcmp(context.devices[index].mac, mac) == 0)
            return &context.devices[index];
    }

    return NULL;
}

void bt_reset_visible_devices(void)
{
    context.visible_device_amount = 0;

    for (int i = 0; i < context.current_device_amount; i++) {
        context.visible_devices[context.visible_device_amount++] = &context.devices[i];
    }
}

void bt_apply_connectable_filter(void)
{
    int write_index = 0;

    for (int i = 0; i < context.visible_device_amount; i++) {
        device_t *device = context.visible_devices[i];

        if (device->connectable) {
            context.visible_devices[write_index++] = device;
        }
    }

    context.visible_device_amount = write_index;
}

int bt_compare_device_ptrs_by_rssi_desc(const void *a, const void *b)
{
    const device_t *dev_a = *(const device_t **)a;
    const device_t *dev_b = *(const device_t **)b;

    if (dev_a->rssi < dev_b->rssi) return 1;
    if (dev_a->rssi > dev_b->rssi) return -1;
    return 0;
}
void bt_sort_visible_devices_by_nearest(void)
{
    if (context.visible_device_amount == 0)
        bt_reset_visible_devices();
        
    qsort(
        context.visible_devices,
        context.visible_device_amount,
        sizeof(device_t *),
        bt_compare_device_ptrs_by_rssi_desc
    );
}

int devices_length()
{
    return context.visible_device_amount;
}

device_t **bt_get_devices()
{
    return context.visible_devices;
}

void bt_controller_ctx_add_device(device_t *dev)
{
    if (!dev->mac[0])
        return;

    device_t *device = find_device(dev->mac);
    if (device)
    {
        device->rssi = dev->rssi;
        if (dev->name[0] && strcmp(device->name, DEFAULT_NAME) == 0)
            snprintf(device->name, sizeof(device->name), "%s", dev->name);

        if (dev->manufacturer[0] && strcmp(device->manufacturer, DEFAULT_NAME) == 0)
            snprintf(device->manufacturer, sizeof(device->manufacturer), "%s", dev->manufacturer);

        return;
    }

    if (context.current_device_amount >= MAX_DEVICES)
    {
        log_info("Can't save more devices, is in the limit of %d", MAX_DEVICES);
        return;
    }

    device = &context.devices[context.current_device_amount++];

    device->rssi = dev->rssi;
    device->connectable = dev->connectable;

    snprintf(device->name, sizeof(device->name), "%s", dev->name[0] ? dev->name : DEFAULT_NAME);
    snprintf(device->mac, sizeof(device->mac), "%s", dev->mac);
    snprintf(device->manufacturer, sizeof(device->manufacturer), "%s",  dev->manufacturer[0] ? dev->manufacturer : DEFAULT_NAME);
    snprintf(device->service, sizeof(device->service), "%s",  dev->service[0] ? dev->service : DEFAULT_NAME);
    snprintf(device->appearance, sizeof(device->appearance), "%s",  dev->appearance[0] ? dev->appearance : DEFAULT_NAME);
}