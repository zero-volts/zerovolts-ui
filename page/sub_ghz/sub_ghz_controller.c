#include "sub_ghz_controller.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "utils/parser.h"
#include "utils/logger.h"
#include "utils/error_handler.h"

#include "service/uart_commands.h"

#define UART_SUBGHZ_TAG_ID "SUBGHZ_TAG_CONTROLLER"

typedef struct {
    int selected_frecuency;
} subghz_ctx;

static subghz_ctx local_ctx;
static subghz_handler internal_cb = NULL;
static data_capture_handler capture_handler = NULL;

static void parse_data(const char *buffer, subghz_data_chunk_t *chunk)
{    
    char tmp_value[16];
    char timing_buffer[512];

    if(get_field_value(buffer, "seq", tmp_value, sizeof(tmp_value)) )
        chunk->seq = strtoul(tmp_value, NULL, 10);

    if(get_field_value(buffer, "chunks", tmp_value, sizeof(tmp_value)) )
        chunk->chunks = strtoul(tmp_value, NULL, 10);

    if (get_field_value(buffer, "count", tmp_value, sizeof(tmp_value)) )
        chunk->count = strtoul(tmp_value, NULL, 10);

    if(get_field_value(buffer, "timings", timing_buffer, sizeof(timing_buffer)) )
    {
        char *saveptr;
        char *token = strtok_r(timing_buffer, ",", &saveptr);

        uint16_t timing_idx = 0;
        while (token != NULL)
        {
            chunk->timings[timing_idx++] = (int32_t)strtol(token, NULL, 10);
            token = strtok_r(NULL, ",", &saveptr);
        }
    }
}

static void event_handler(const char *tag_id, char *buffer)
{
    if (strcmp(tag_id, UART_SUBGHZ_TAG_ID) != 0) {
        return;
    }

    if (capture_handler != NULL)
    {
        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DONE) != NULL) {    
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DATA) != NULL)
        {
            subghz_data_chunk_t chunk = {0};
            parse_data(buffer, &chunk);

            capture_handler(UI_LOADING, &chunk);
        }
    }
    
}

void subghz_set_cb(subghz_handler new_callback)
{
    internal_cb = new_callback;
}

void subghz_set_capture_cb(data_capture_handler callback)
{
    capture_handler = callback;
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

uart_status_t subghz_start_capture(int frequency, int time_frame_ms)
{
    int freq = frequency < local_ctx.selected_frecuency ? local_ctx.selected_frecuency : frequency;
    uart_status_t uart_rc = uart_send_formatted_line("%s|%d|%d", SUBGHZ_COMMAND_REQ_CAPTURE, freq, time_frame_ms);
    if (uart_rc != UART_OK)
    {
        log_warning("subghz_start_capture error: %s\n", last_error());
        return uart_rc;
    }

    return UART_OK;
}