#include "bt_controller.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "app_context.h"
#include "utils/logger.h"
#include "utils/string_utils.h"
#include "utils/error_handler.h"
#include "utils/parser.h"
#include "service/uart_commands.h"

#define UART_BT_TAG_ID "BT_TAG_CONTROLLER"
#define BT_SERVICE_TIMEOUT_PROCCESS_MS 30000


typedef struct {
    scanner_handler internal_cb;
    bt_conn_handler conn_cb;

    bt_status_t bt_status;
} bt_ctx;

typedef struct {
    device_t devices[BT_ALLOWED_MAX_DEVICES];
    int amount;
} list_items_ctx;

static list_items_ctx local_ctx;

static bt_service_t services[BT_MAX_SERVICES];
static int services_count = 0;

static bt_ctx context;

void set_scanner_cb(scanner_handler new_callback)
{
    context.internal_cb = new_callback;
}

void set_conn_cb(bt_conn_handler new_callback)
{
    context.conn_cb = new_callback;
}

bt_status_t bt_get_conn_status(void)
{
    return context.bt_status;
}

bt_service_t *bt_get_services(void)
{
    return services;
}

int bt_get_services_length(void)
{
    return services_count;
}

void bt_clear_discovery(void)
{
    services_count = 0;
    memset(services, 0, sizeof(services));
    context.bt_status = BT_IDLE;
}

static bt_service_t *find_service(int svc_index)
{
    for (int i = 0; i < services_count; i++) {
        if (services[i].svc_index == svc_index)
            return &services[i];
    }

    return NULL;
}

static bt_service_t *add_service(int svc_index, const char *uuid)
{
    bt_service_t *existing = find_service(svc_index);
    if (existing)
        return existing;

    if (services_count >= BT_MAX_SERVICES) {
        log_warning("services full (max %d)\n", BT_MAX_SERVICES);
        return NULL;
    }

    bt_service_t *svc = &services[services_count++];
    svc->svc_index = svc_index;
    snprintf(svc->uuid, sizeof(svc->uuid), "%s", uuid ? uuid : "");
    svc->chars_count = 0;
    return svc;
}

static void add_characteristic(int svc_index, int char_index, const char *uuid,
                               unsigned int props, unsigned int handle)
{
    bt_service_t *svc = find_service(svc_index);
    if (!svc) {
        svc = add_service(svc_index, "");
        if (!svc)
            return;
    }

    if (svc->chars_count >= BT_MAX_CHARS_PER_SERVICE) {
        log_warning("chars full for svc %d\n", svc_index);
        return;
    }

    bt_characteristic_t *ch = &svc->chars[svc->chars_count++];
    ch->char_index = char_index;
    snprintf(ch->uuid, sizeof(ch->uuid), "%s", uuid ? uuid : "");
    ch->props = props;
    ch->handle = handle;
}

static device_t parse_device(const char *buffer)
{
    device_t device = {0};
    char value[16];

    get_field_value(buffer, "name", device.name, sizeof(device.name));
    get_field_value(buffer, "mac", device.mac, sizeof(device.mac));

    if (get_field_value(buffer, "rssi", value, sizeof(value)))
        device.rssi = atoi(value);

    get_field_value(buffer, "manufacturer", device.manufacturer, sizeof(device.manufacturer));
    get_field_value(buffer, "service", device.service, sizeof(device.service));
    get_field_value(buffer, "appearance", device.appearance, sizeof(device.appearance));

    if (get_field_value(buffer, "connectable", value, sizeof(value)))
        device.connectable = atoi(value);

    if (get_field_value(buffer, "addr_type", value, sizeof(value)))
        device.addr_type = atoi(value);

    return device;
}

static void set_status(bt_status_t new_status, const char *info)
{
    context.bt_status = new_status;
    if (context.conn_cb)
        context.conn_cb(new_status, info);
}

static void parse_discover_service(char *buffer)
{
    char val[64];
    int svc_index = 0;
    char uuid[BT_UUID_STR_LEN] = {0};

    if (get_field_value(buffer, "svc", val, sizeof(val)))
        svc_index = atoi(val);

    get_field_value(buffer, "uuid", uuid, sizeof(uuid));

    add_service(svc_index, uuid);
    if (context.conn_cb)
        context.conn_cb(BT_DISCOVERING, NULL);
}

static void parse_discover_char(char *buffer)
{
    char val[64];
    int svc_index = 0;
    int char_index = 0;
    char uuid[BT_UUID_STR_LEN] = {0};
    unsigned int props = 0;
    unsigned int handle = 0;

    if (get_field_value(buffer, "svc", val, sizeof(val)))
        svc_index = atoi(val);

    if (get_field_value(buffer, "char", val, sizeof(val)))
        char_index = atoi(val);

    get_field_value(buffer, "uuid", uuid, sizeof(uuid));
    if (get_field_value(buffer, "props", val, sizeof(val)))
        props = (unsigned int)strtoul(val, NULL, 0);

    if (get_field_value(buffer, "handle", val, sizeof(val)))
        handle = (unsigned int)strtoul(val, NULL, 0);

    add_characteristic(svc_index, char_index, uuid, props, handle);

    if (context.conn_cb)
        context.conn_cb(BT_DISCOVERING, NULL);
}

static void event_handler(const char *tag_id, char *buffer)
{
    if (strcmp(tag_id, UART_BT_TAG_ID) != 0) {
        return;
    }

    // -- SCAN --
    if (context.internal_cb != NULL)
    {
        if (strstr(buffer, BT_COMMAND_RES_SCAN_START) != NULL) {
            context.internal_cb(NULL, UI_LOADING);
            return;
        }

        if (strstr(buffer, BT_COMMAND_RES_SCAN_DONE) != NULL)
        {   
            context.internal_cb(NULL, UI_DONE);

            uart_cancel_response_timeout(UART_BT_TAG_ID);
            context.bt_status = BT_IDLE;
            return;
        }

        if (strstr(buffer, BT_COMMAND_RES_SCAN_DEVICE) != NULL)
        {
            device_t device = parse_device(buffer);
            context.internal_cb(&device, UI_LOADING);
            bt_context_add_device(&device);

            return;
        }
    }

    // -- CONNECT --
    if (zv_starts_with(buffer, BT_COMMAND_RES_CONNECT_START))
    {
        set_status(BT_CONNECTING, NULL);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_CONNECT_OK))
    {
        set_status(BT_CONNECTED, NULL);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_CONNECT_FAIL))
    {
        char reason[32] = {0};
        get_field_value(buffer, "reason", reason, sizeof(reason));
        set_status(BT_FAILED, reason);

        uart_cancel_response_timeout(UART_BT_TAG_ID);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_CONNECT_LOST))
    {
        char reason[32] = {0};
        get_field_value(buffer, "reason", reason, sizeof(reason));
        set_status(BT_LOST, reason);

        uart_cancel_response_timeout(UART_BT_TAG_ID);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_CONNECT_ERROR))
    {
        set_status(BT_FAILED, "command error");
        uart_cancel_response_timeout(UART_BT_TAG_ID);
        
        return;
    }

    // -- DISCONNECT --
    if (zv_starts_with(buffer, BT_COMMAND_RES_DISCONNECT_OK))
    {
        set_status(BT_DISCONNECTED, NULL);
        uart_cancel_response_timeout(UART_BT_TAG_ID);

        return;
    }

    // -- DISCOVER --
    if (zv_starts_with(buffer, BT_COMMAND_RES_DISCOVER_START))
    {
        services_count = 0;
        memset(services, 0, sizeof(services));
        set_status(BT_DISCOVERING, NULL);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_DISCOVER_SERVICE)) {
        parse_discover_service(buffer);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_DISCOVER_CHAR)) {
        parse_discover_char(buffer);
        return;
    }
    if (zv_starts_with(buffer, BT_COMMAND_RES_DISCOVER_DONE))
    {
        set_status(BT_READY, NULL);
        uart_cancel_response_timeout(UART_BT_TAG_ID);
        return;
    }

    if (zv_starts_with(buffer, BT_COMMAND_RES_DISCOVER_FAIL))
    {
        char reason[32] = {0};
        get_field_value(buffer, "reason", reason, sizeof(reason));
        set_status(BT_FAILED, reason);

        uart_cancel_response_timeout(UART_BT_TAG_ID);
        return;
    }
}

static void on_bt_action_timeout(const char *tag_id)
{
    log_warning("we have a timeout %d", context.bt_status);
    (void)tag_id;
    switch(context.bt_status)
    {
        case BT_SCANNING:
            set_status(BT_FAILED, "Can't scan devices the ESP32 never reply!");
            if (context.internal_cb != NULL)
                context.internal_cb(NULL, UI_TIMEOUT);

            break;
        case BT_CONNECTING:
            set_status(BT_FAILED, "Can't connect to the device");
            break;
        case BT_DISCOVERING:
            set_status(BT_FAILED, "Discovering services and characteristics return timeout");
            break;
        default:
            context.bt_status = BT_IDLE;

    }

    uart_cancel_response_timeout(UART_BT_TAG_ID);
}

uart_status_t bt_controller_init(const uart_config_t *config)
{
    if (config == NULL) {
        return UART_ERR_CONFIG;
    }

    uart_status_t uart_rc = uart_service_init(config->device, config->baudrate);
    if (uart_rc != UART_OK)
    {
        log_warning("UART init failed: %s\n", last_error());
        return uart_rc;
    }
    
    add_event_callback(event_handler, UART_BT_TAG_ID);
    uart_rc = uart_set_timeout_callback(UART_BT_TAG_ID, on_bt_action_timeout);
    if (uart_rc != UART_OK)
        return uart_rc;

    context.bt_status = BT_IDLE;

    return UART_OK;
}

uart_status_t start_scan()
{
    context.bt_status = BT_SCANNING;
    uart_status_t uart_rc = uart_send_line("SCAN");
    if (uart_rc != UART_OK)
    {
        context.bt_status = BT_FAILED;
        log_warning("start_scan error: %s\n", last_error());
        return uart_rc;
    }

    uart_rc = uart_start_response_timeout(UART_BT_TAG_ID, BT_SERVICE_TIMEOUT_PROCCESS_MS);
    if (uart_rc != UART_OK)
    {
        context.bt_status = BT_FAILED;
        return uart_rc;
    }

    return UART_OK;
}

uart_status_t bt_connect(const device_t *device)
{
    if (device == NULL || device->mac[0] == '\0') {
        log_warning("bt_connect: invalid device\n");
        return UART_ERR_INVALID;
    }

    bt_clear_discovery();
    set_status(BT_CONNECTING, NULL);

    context.bt_status = BT_CONNECTING;
    uart_status_t rc = uart_send_formatted_line("%s|%s|%d",
             BT_COMMAND_REQ_CONNECT, device->mac, device->addr_type);

    if (rc != UART_OK)
    {
        log_warning("bt_connect error: %s\n", last_error());
        set_status(BT_FAILED, "uart");
        return rc;
    }

    rc = uart_start_response_timeout(UART_BT_TAG_ID, BT_SERVICE_TIMEOUT_PROCCESS_MS);
    if (rc != UART_OK)
    {        
        set_status(BT_FAILED, "uart");
        return rc;
    }

    return rc;
}

uart_status_t bt_disconnect(void)
{
    uart_status_t rc = uart_send_line(BT_COMMAND_REQ_DISCONNECT);
    if (rc != UART_OK) {
        log_warning("bt_disconnect error: %s\n", last_error());
    }

    return rc;
}

// ---- bluetooth app context functions ---- ///
void bt_controller_select_device(const device_t *device)
{
    bt_context_set_selected(device);
}

const device_t *bt_controller_get_selected(void)
{
    return bt_context_get_selected();
}

void bt_controller_reset_devices(void)
{
    bt_context_clear_devices();
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
