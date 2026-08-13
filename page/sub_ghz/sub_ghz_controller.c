#include "sub_ghz_controller.h"

#include <string.h>

#include "utils/logger.h"
#include "utils/error_handler.h"
#include "service/uart_commands.h"

#define UART_SUBGHZ_TAG_ID "SUBGHZ_TAG_CONTROLLER"

typedef struct {
    int selected_frecuency;
} subghz_ctx;

static subghz_ctx local_ctx;

static subghz_handler internal_cb = NULL;

static void event_handler(const char *tag_id, char *buffer)
{
    log_warning("event handler: %s", tag_id);
    if (strcmp(tag_id, UART_SUBGHZ_TAG_ID) != 0) {
        return;
    }

    if (internal_cb != NULL)
    {
        if (strstr(buffer, SUBGHZ_COMMAND_RES_CHECK_MODULE) != NULL) {
            internal_cb(UI_LOADING);
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_SET_FRECUENCY) != NULL) {
            internal_cb(UI_DONE);
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_START) != NULL) {
            
            internal_cb( UI_LOADING);
            
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DONE) != NULL) {
            
            internal_cb(UI_LOADING);
            
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CHECK_MODULE) != NULL) {
            
            internal_cb(UI_LOADING);
            
            return;
        }

        
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DATA) != NULL)
    {
        log_warning("llego el dato, buffer: %s", buffer);
    }
}

void subghz_set_cb(subghz_handler new_callback)
{
    internal_cb = new_callback;
}

uart_status_t subghz_controller_init(const subghz_config_t *config, 
    const uart_config_t *uart_config)
{
    if (config == NULL) {
        return UART_ERR_CONFIG;
    }

    uart_status_t uart_rc = uart_service_init(uart_config->device, uart_config->baudrate);
    if (uart_rc != UART_OK)
    {
        log_warning("UART init failed: %s\n", last_error());
        return uart_rc;
    }

    add_event_callback(event_handler, UART_SUBGHZ_TAG_ID);

    local_ctx.selected_frecuency = config->default_freq;

    return UART_OK;
}

uart_status_t subghz_start_capture()
{
    uart_status_t uart_rc = uart_send_formatted_line("%s|%d|%d", SUBGHZ_COMMAND_REQ_CAPTURE, local_ctx.selected_frecuency, 20000);
    if (uart_rc != UART_OK)
    {
        log_warning("subghz_start_capture error: %s\n", last_error());
        return uart_rc;
    }

    return UART_OK;
}