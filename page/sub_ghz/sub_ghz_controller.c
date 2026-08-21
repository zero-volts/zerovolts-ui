#include "sub_ghz_controller.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#include "types.h"
#include "app_context.h"
#include "utils/parser.h"
#include "utils/logger.h"
#include "utils/error_handler.h"
#include "service/uart_commands.h"

#define UART_SUBGHZ_TAG_ID "SUBGHZ_TAG_CONTROLLER"
#define DATA_TIMING_BUFFER SUBGHZ_DATA_CHUNK_MAX_VALUES * 10

static data_capture_handler capture_handler = NULL;

static bool parse_capture_data(const char *buffer, subghz_data_chunk_t *chunk)
{    
    char tmp_value[16];
    char timing_buffer[DATA_TIMING_BUFFER];

    if(!get_field_value(buffer, "capture_id", tmp_value, sizeof(tmp_value)) )
        return false;

    chunk->capture_id = strtoul(tmp_value, NULL, 10);

    if(!get_field_value(buffer, "seq", tmp_value, sizeof(tmp_value)) )
        return false;    
    
    chunk->seq = strtoul(tmp_value, NULL, 10);

    if(!get_field_value(buffer, "chunks", tmp_value, sizeof(tmp_value)) )
        return false;

    chunk->chunks = strtoul(tmp_value, NULL, 10);

    if (!get_field_value(buffer, "count", tmp_value, sizeof(tmp_value)) )
        return false;

    chunk->count = strtoul(tmp_value, NULL, 10);

    if (chunk->count > SUBGHZ_DATA_CHUNK_MAX_VALUES)
    {
        log_error("wrong data chunks for parse_capture_data %u\n", chunk->count);
        return false;
    }

    if(!get_field_value(buffer, "timings", timing_buffer, sizeof(timing_buffer)) )
        return false;

    char *saveptr;
    char *token = strtok_r(timing_buffer, ",", &saveptr);

    uint16_t timing_idx = 0;
    while (token != NULL)
    {
        if (timing_idx >= chunk->count  || timing_idx >= SUBGHZ_DATA_CHUNK_MAX_VALUES)
            break;

        chunk->timings[timing_idx++] = (int32_t)strtol(token, NULL, 10);
        token = strtok_r(NULL, ",", &saveptr);
    }

    return true;
}

static void event_handler(const char *tag_id, char *buffer)
{
    if (strcmp(tag_id, UART_SUBGHZ_TAG_ID) != 0) {
        return;
    }

    if (capture_handler != NULL)
    {
        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_START) != NULL) 
        {
            capture_handler(UI_LOADING, NULL);    
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DONE) != NULL) 
        {
            char tmp_value[21];
            if(!get_field_value(buffer, "capture_id", tmp_value, sizeof(tmp_value)) )
                return;

            uint64_t session_id = strtoull(tmp_value, NULL, 10);
            if (!subghz_set_session_completed(session_id))
            {
                capture_handler(UI_ERROR, NULL);
                log_warning("event_handler Can't complete the session : %" PRIu64 "", session_id);
                return;
            }

            capture_handler(UI_DONE, NULL);  
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DATA) != NULL)
        {
            subghz_event_arg event_arg = {0};

            subghz_data_chunk_t chunk = {0};
            if(!parse_capture_data(buffer, &chunk)) 
            {
                subghz_event_arg event_arg = {
                    .data = NULL,
                    .message = "Problems getting keys to parser the data"
                };

                capture_handler(UI_ERROR, &event_arg);
                return;
            }

            subghz_add_session_chunk(chunk);
            event_arg.data = &chunk;
            capture_handler(UI_LOADING, &event_arg);
            return;
        }

        if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_ERROR) != NULL || strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_FAIL) != NULL) 
        {           
            char err_message[255] = "Unknown capture error";
            get_field_value(buffer, "reason", err_message, sizeof(err_message));

            subghz_event_arg event_arg = {
                .data = NULL,
                .message = err_message
            };

            capture_handler(UI_ERROR, &event_arg);
            return;
        }
    }
    
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
    return UART_OK;
}

uart_status_t subghz_start_capture(uint32_t frequency, uint32_t time_frame_ms)
{
    uart_status_t uart_rc = uart_send_formatted_line("%s|freq_hz=%" PRIu32 "|timeout_ms=%" PRIu32,
        SUBGHZ_COMMAND_REQ_CAPTURE, frequency, time_frame_ms);

    if (uart_rc != UART_OK)
    {
        log_warning("subghz_start_capture error: %s\n", last_error());
        return uart_rc;
    }

    return UART_OK;
}