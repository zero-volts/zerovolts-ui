#include "subghz_service.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "utils/logger.h"
#include "utils/parser.h"
#include "utils/error_handler.h"
#include "service/uart_service.h"
#include "service/uart_commands.h"
#include "service/subghz/subghz_file.h"

#define UART_SUBGHZ_TAG_ID "SUBGHZ_TAG_CONTROLLER"
#define DATA_TIMING_BUFFER SUBGHZ_DATA_CHUNK_MAX_VALUES * 10

typedef struct {
    char files_directory[512];
    subghz_service_handler service_handler;
} service_context;

static service_context ctx;

static bool parse_capture_data(const char *buffer, subghz_data_chunk_t *chunk)
{
    char tmp_value[21];
    char timing_buffer[DATA_TIMING_BUFFER];

    if(!get_field_value(buffer, "capture_id", tmp_value, sizeof(tmp_value)) )
        return false;

    chunk->capture_id = strtoull(tmp_value, NULL, 10);

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

    if (ctx.service_handler == NULL)
    {
        log_warning("subghz_service::event_handler no associated handler");
        return;
    }

    subghz_service_event_t event = {
        .type = SUBGHZ_EVENT_NONE,
        .message = {0},
        .data = NULL
    };

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_START) != NULL)
    {
        event.type = SUBGHZ_EVENT_CAPTURE_STARTED;
        ctx.service_handler(&event);
        return;
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DONE) != NULL)
    {
        char tmp_value[21];
        if(!get_field_value(buffer, "capture_id", tmp_value, sizeof(tmp_value)) )
            return;

        uint64_t session_id = strtoull(tmp_value, NULL, 10);
        subghz_capture_session_t *session = subghz_get_session_by(session_id);
        char *end = NULL;
        if (!session || !get_field_value(buffer, "freq", tmp_value, sizeof(tmp_value)))
        {
            event.type = SUBGHZ_EVENT_CAPTURE_ERROR;
            snprintf(event.message, sizeof(event.message), "Captura incompleta o sin frecuencia");
            ctx.service_handler(&event);
            return;
        }
        unsigned long long frequency = strtoull(tmp_value, &end, 10);
        if (!tmp_value[0] || *end || frequency == 0 || frequency > UINT32_MAX)
        {
            event.type = SUBGHZ_EVENT_CAPTURE_ERROR;
            snprintf(event.message, sizeof(event.message), "Frecuencia de captura invalida");
            ctx.service_handler(&event);
            return;
        }
        session->frequency = (uint32_t)frequency;

        if (!subghz_set_session_completed(session_id))
        {
            event.type = SUBGHZ_EVENT_CAPTURE_ERROR;
            ctx.service_handler(&event);

            log_warning("subghz_service::event_handler Can't complete the session : %" PRIu64 "", session_id);
            return;
        }

        event.data = subghz_get_session_by(session_id);
        event.type = SUBGHZ_EVENT_CAPTURE_READY;
        ctx.service_handler(&event);
        return;
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DATA) != NULL)
    {
        subghz_data_chunk_t chunk = {0};
        if(!parse_capture_data(buffer, &chunk))
        {
            event.type = SUBGHZ_EVENT_CAPTURE_ERROR;
            snprintf(event.message, sizeof(event.message), "%s", "Error capturando datos");
            ctx.service_handler(&event);
            return;
        }

        subghz_add_session_chunk(chunk);

        event.data = &chunk;
        event.type = SUBGHZ_EVENT_CAPTURE_DATA;

        ctx.service_handler(&event);
        return;
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_ERROR) != NULL || strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_FAIL) != NULL) 
    {
        char err_message[255] = "Unknown capture error";
        get_field_value(buffer, "reason", err_message, sizeof(err_message));

        event.type = SUBGHZ_EVENT_CAPTURE_ERROR;
        snprintf(event.message, sizeof(event.message), "%s", err_message);
        ctx.service_handler(&event);
        return;
    }
}

subghz_status_t subghz_service_init(const zv_config *config)
{
    if (config == NULL) {
        return SUBGHZ_ERR_CONFIG;
    }

    const uart_config_t *uart_config = &config->uart;
    uart_status_t uart_rc = uart_service_init(uart_config->device, uart_config->baudrate);
    if (uart_rc != UART_OK)
    {
        log_warning("subghz_service_init::UART init failed: %s\n", last_error());
        return (subghz_status_t)uart_rc;
    }

    add_event_callback(event_handler, UART_SUBGHZ_TAG_ID);
    if (!subghz_file_create_root_path(config->subghz.signals_path))
        return SUBGHZ_ERR_IO;
    snprintf(ctx.files_directory, sizeof(ctx.files_directory), "%s", config->subghz.signals_path);

    return SUBGHZ_OK;
}

subghz_status_t subghz_service_start_capture(uint32_t frequency, uint32_t time_frame_ms)
{
    uart_status_t uart_rc = uart_send_formatted_line("%s|freq_hz=%" PRIu32 "|timeout_ms=%" PRIu32,
        SUBGHZ_COMMAND_REQ_CAPTURE, frequency, time_frame_ms);

    if (uart_rc != UART_OK)
    {
        log_warning("subghz_start_capture error: %s\n", last_error());
        return (subghz_status_t)uart_rc;
    }

    return SUBGHZ_OK;
}

void subghz_service_add_event_callback(subghz_service_handler new_cb)
{
    ctx.service_handler = new_cb;
}

subghz_status_t subghz_service_save(subghz_capture_session_t *session)
{
    if (session == NULL)
        return SUBGHZ_ERR_IO;

    time_t timestamp = time(NULL);
    char file_name_path[512];

    int written = snprintf(file_name_path, sizeof(file_name_path), "%s/%" PRIu64 "_%ld.sub",
                           ctx.files_directory, session->capture_id, (long)timestamp);
    if (written < 0 || (size_t)written >= sizeof(file_name_path))
        return SUBGHZ_ERR_IO;

    if (!subghz_file_create(file_name_path, session))
    {
        log_error("Unable to save Sub-GHz capture: %s", file_name_path);
        return SUBGHZ_ERR_IO;
    }
    return SUBGHZ_OK;
}