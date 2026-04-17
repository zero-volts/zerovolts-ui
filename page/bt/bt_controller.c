#include "bt_controller.h"
#include "utils/error_handler.h"
#include "utils/logger.h"

#include <string.h>
#include <stdlib.h>
#define UART_DEVICE "/dev/ttyAMA5"
#define UART_BAUDRATE 115200
#define UART_BT_TAG_ID "BT_TAG_CONTROLLER"

#define MAX_DEVICES 5
device_t devices[MAX_DEVICES];

static int device_count = 0;

static bt_event_cb internl_cb = NULL;


void set_scanner_cb(bt_event_cb new_callback)
{
    internl_cb = new_callback;
}

static void event_handler(const char *tag_id, char *buffer)
{
    if (strcmp(tag_id, UART_BT_TAG_ID) != 0) {
        return;
    }

    log_info("buffer handler: %s\n", buffer);
    if (strstr(buffer, "SCAN:DEVICE") == NULL) {
        return;
    }

    char *token;
    char *saveptr;

    token = strtok_r(buffer, "|", &saveptr);

    device_t device = {0};
    while (token != NULL) {
        printf("Token: %s\n", token);

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

    if (internl_cb != NULL)
        internl_cb(&device);
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