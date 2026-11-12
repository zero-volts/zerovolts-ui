#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "subghz_service.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "types.h"
#include "utils/logger.h"
#include "utils/parser.h"
#include "utils/error_handler.h"
#include "service/uart_service.h"
#include "service/uart_commands.h"

#define SUBGHZ_SEND_CHUNK_VALUES            16
#define SUBGHZ_SEND_MAX_TIMINGS             256
#define SUBGHZ_US_PER_MS                   1000
#define SUBGHZ_TIMEOUT_PER_REPEAT_MS       100
#define SUBGHZ_TIMEOUT_FIXED_MARGIN_MS     2000 // Estimated margin for radio setup, command processing and the UART reply.
#define SUBGHZ_UPLOAD_RESPONSE_TIMEOUT_MS  2000 // Maximum wait for BEGIN_OK or the ACK of one data block.

#define UART_SUBGHZ_TAG_ID "SUBGHZ_TAG_CONTROLLER"
#define DATA_TIMING_BUFFER SUBGHZ_DATA_CHUNK_MAX_VALUES * 10

typedef enum {
    REPLAY_IDLE = 0,
    REPLAY_WAIT_BEGIN_OK,
    REPLAY_WAIT_ACK,
    REPLAY_WAIT_DONE
} replay_state_t;

// The controller can release the file as soon as BEGIN is sent.
static struct {
    uint32_t id;
    uint16_t seq;
    uint16_t count;
    uint16_t chunks;
    uint64_t completion_timeout_ms;
    int32_t timings[SUBGHZ_SEND_MAX_TIMINGS];
} replay;

typedef struct {
    char files_directory[512];
    subghz_service_handler service_handler;

    subghz_capture_status_t capture_status;
    uint64_t capture_id; // Learned from the first data block; zero before data arrives.
    replay_state_t replay_state;
} service_context;

typedef struct {
    file_callback callback;
    void *user_data;
} signal_list_ctx;

static service_context ctx;

static bool capture_is_active(void)
{
    return ctx.capture_status == SUBGHZ_CAPTURE_WAIT_START ||
           ctx.capture_status == SUBGHZ_CAPTURE_STARTED ||
           ctx.capture_status == SUBGHZ_CAPTURE_RECEIVING;
}

static void notify_event(subghz_service_event_t *event)
{
    if (ctx.service_handler)
        ctx.service_handler(event);
}

static void finish_capture(subghz_capture_status_t state, subghz_status_t status,
                           const char *message, subghz_capture_session_t *session)
{
    uart_cancel_response_timeout(UART_SUBGHZ_TAG_ID);
    ctx.capture_status = state;

    if (state != SUBGHZ_CAPTURE_DONE && ctx.capture_id != 0)
        subghz_remove_session_by(ctx.capture_id);

    ctx.capture_id = 0;

    subghz_service_event_t event = {
        .type = state == SUBGHZ_CAPTURE_DONE ? SUBGHZ_EVENT_CAPTURE_READY :
                state == SUBGHZ_CAPTURE_TIMEOUT ? SUBGHZ_EVENT_TIMEOUT : SUBGHZ_EVENT_CAPTURE_ERROR,
        .status = status,
        .data = session,
    };

    snprintf(event.message, sizeof(event.message), "%s", message);
    set_last_error(status == SUBGHZ_OK ? NULL : event.message);

    notify_event(&event);
}

static void finish_replay(subghz_status_t status, const char *message)
{
    uart_cancel_response_timeout(UART_SUBGHZ_TAG_ID);
    ctx.replay_state = REPLAY_IDLE;

    subghz_service_event_t event = {
        .type = status == SUBGHZ_OK ? SUBGHZ_EVENT_SEND_DONE : SUBGHZ_EVENT_SEND_ERROR,
        .status = status,
    };

    snprintf(event.message, sizeof(event.message), "%s", message);
    set_last_error(status == SUBGHZ_OK ? NULL : event.message);
    if (ctx.service_handler)
        ctx.service_handler(&event);
}

static void on_event_timeout(const char *tag_id)
{
    (void)tag_id;
    if (ctx.replay_state != REPLAY_IDLE)
    {
        finish_replay(SUBGHZ_ERR_TIMEOUT, "Timeout waiting for Sub-GHz \nreplay confirmation.");
        return;
    }

    if (capture_is_active())
        finish_capture(SUBGHZ_CAPTURE_TIMEOUT, SUBGHZ_ERR_TIMEOUT, "Timeout waiting for Sub-GHz \ncapture confirmation.", NULL);
}

static subghz_status_t send_replay_chunk(void)
{
    uint16_t start = replay.seq * SUBGHZ_SEND_CHUNK_VALUES;
    uint16_t count = replay.count - start;
    if (count > SUBGHZ_SEND_CHUNK_VALUES)
        count = SUBGHZ_SEND_CHUNK_VALUES;

    char line[254];
    int written = snprintf(line, sizeof(line),
        "%s|id=%" PRIu32 "|seq=%" PRIu16 "|count=%" PRIu16 "|timings=",
        SUBGHZ_COMMAND_REQ_SEND_DATA, replay.id, replay.seq, count);

    if (written < 0 || (size_t)written >= sizeof(line))
    {
        set_last_error("Replay chunk exceeds the UART line buffer.");
        return SUBGHZ_ERR_INVALID;
    }

    size_t offset = (size_t)written;
    for (uint16_t i = 0; i < count; i++)
    {
        written = snprintf(line + offset, sizeof(line) - offset, "%s%+" PRId32,
                           i ? "," : "", replay.timings[start + i]);
        if (written < 0 || (size_t)written >= sizeof(line) - offset)
        {
            set_last_error("Replay chunk exceeds the UART line buffer.");
            return SUBGHZ_ERR_INVALID;
        }

        offset += (size_t)written;
    }

    uart_status_t result = uart_send_line(line);
    if (result != UART_OK)
        return (subghz_status_t)result;

    ctx.replay_state = REPLAY_WAIT_ACK;

    return (subghz_status_t)uart_start_response_timeout(UART_SUBGHZ_TAG_ID, SUBGHZ_UPLOAD_RESPONSE_TIMEOUT_MS);
}

static subghz_status_t send_replay_end(void)
{
    uart_status_t result = uart_send_formatted_line("%s|id=%" PRIu32, SUBGHZ_COMMAND_REQ_SEND_END, replay.id);
    if (result != UART_OK)
        return (subghz_status_t)result;

    ctx.replay_state = REPLAY_WAIT_DONE;

    return (subghz_status_t)uart_start_response_timeout(UART_SUBGHZ_TAG_ID, replay.completion_timeout_ms);
}

static bool handle_replay_response(const char *buffer)
{
    if (ctx.replay_state == REPLAY_IDLE)
        return false;

    char id[21];
    char *end;
    if (!get_field_value(buffer, "id", id, sizeof(id)))
        return false;

    uint64_t response_id = strtoull(id, &end, 10);
    if (!id[0] || *end || response_id != replay.id)
        return false;

    if (strncmp(buffer, SUBGHZ_COMMAND_RES_SEND_FAIL "|", strlen(SUBGHZ_COMMAND_RES_SEND_FAIL) + 1) == 0)
    {
        char reason[128] = "Sub-GHz replay failed.";
        get_field_value(buffer, "reason", reason, sizeof(reason));
        finish_replay(SUBGHZ_ERR_IO, reason);

        return true;
    }

    const char *expected;
    switch (ctx.replay_state)
    {
        case REPLAY_WAIT_BEGIN_OK: 
            expected = SUBGHZ_COMMAND_RES_SEND_BEGIN_OK; 
            break;
        case REPLAY_WAIT_ACK: 
            expected = SUBGHZ_COMMAND_RES_SEND_ACK;
            break;
        case REPLAY_WAIT_DONE: 
            expected = SUBGHZ_COMMAND_RES_SEND_DONE;
            break;
        default: 
            return false;
    }

    size_t length = strlen(expected);
    if (strncmp(buffer, expected, length) != 0 || buffer[length] != '|')
        return false;

    if (ctx.replay_state == REPLAY_WAIT_ACK)
    {
        char seq[21];
        if (!get_field_value(buffer, "seq", seq, sizeof(seq)))
            return false;

        uint64_t response_seq = strtoull(seq, &end, 10);
        if (!seq[0] || *end || response_seq != replay.seq)
            return false;
    }

    uart_cancel_response_timeout(UART_SUBGHZ_TAG_ID);
    if (ctx.replay_state == REPLAY_WAIT_DONE)
    {
        finish_replay(SUBGHZ_OK, "Signal sent.");
        return true;
    }

    if (ctx.replay_state == REPLAY_WAIT_ACK)
        replay.seq++;

    subghz_status_t result;
    if (replay.seq < replay.chunks)
        result = send_replay_chunk();
    else
        result = send_replay_end();

    if (result != SUBGHZ_OK)
        finish_replay(result, last_error());

    return true;
}

static void list_signal_file(const file_desc *file, void *user_data)
{
    signal_list_ctx *list = (signal_list_ctx *)user_data;
    if (file->is_file && file_has_extension(file->file_name, ".sub"))
        list->callback(file, list->user_data);
}

subghz_status_t subghz_service_list_signals_cb(const char *signals_dir, file_callback callback, void *user_data)
{
    if (!signals_dir || !signals_dir[0] || !callback)
    {
        set_last_error("Signals directory and callback are required.");
        return SUBGHZ_ERR_INVALID;
    }

    if (!file_is_directory(signals_dir))
    {
        set_last_error("Signals directory not found.");
        return SUBGHZ_ERR_IO;
    }

    signal_list_ctx list = { .callback = callback, .user_data = user_data };
    get_file_list(signals_dir, list_signal_file, &list);
    set_last_error(NULL);

    return SUBGHZ_OK;
}

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

    if (handle_replay_response(buffer))
        return;

    if (!capture_is_active())
        return;

    subghz_service_event_t event = {
        .type = SUBGHZ_EVENT_NONE,
        .status = SUBGHZ_OK,
    };

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_START) != NULL)
    {
        if (ctx.capture_status != SUBGHZ_CAPTURE_WAIT_START)
            return;

        ctx.capture_status = SUBGHZ_CAPTURE_STARTED;
        event.type = SUBGHZ_EVENT_CAPTURE_STARTED;
        notify_event(&event);

        return;
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DONE) != NULL)
    {
        char tmp_value[21];
        if (!get_field_value(buffer, "capture_id", tmp_value, sizeof(tmp_value)))
        {
            finish_capture(SUBGHZ_CAPTURE_FAILED, SUBGHZ_ERR_INVALID, "Capture result is missing capture_id.", NULL);
            return;
        }

        uint64_t session_id = strtoull(tmp_value, NULL, 10);
        subghz_capture_session_t *session = subghz_get_session_by(session_id);
        char *end = NULL;
        if (!session || !get_field_value(buffer, "freq", tmp_value, sizeof(tmp_value)))
        {
            finish_capture(SUBGHZ_CAPTURE_FAILED, SUBGHZ_ERR_INVALID, "Captura incompleta o sin frecuencia", NULL);
            return;
        }

        unsigned long long frequency = strtoull(tmp_value, &end, 10);
        if (!tmp_value[0] || *end || frequency == 0 || frequency > UINT32_MAX)
        {
            finish_capture(SUBGHZ_CAPTURE_FAILED, SUBGHZ_ERR_INVALID, "Frecuencia de captura invalida", NULL);
            return;
        }
        session->frequency = (uint32_t)frequency;

        if (!subghz_set_session_completed(session_id))
        {
            finish_capture(SUBGHZ_CAPTURE_FAILED, SUBGHZ_ERR_INVALID, "Captura incompleta: faltan bloques de datos.", NULL);
            return;
        }

        finish_capture(SUBGHZ_CAPTURE_DONE, SUBGHZ_OK, "Signal captured.", session);
        return;
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_DATA) != NULL)
    {
        subghz_data_chunk_t chunk = {0};
        if (!parse_capture_data(buffer, &chunk))
        {
            finish_capture(SUBGHZ_CAPTURE_FAILED, SUBGHZ_ERR_INVALID, "Error capturando datos", NULL);
            return;
        }

        if (ctx.capture_id != 0 && ctx.capture_id != chunk.capture_id)
        {
            finish_capture(SUBGHZ_CAPTURE_FAILED, SUBGHZ_ERR_INVALID, "Capture ID changed during data transfer.", NULL);
            return;
        }

        ctx.capture_id = chunk.capture_id;
        subghz_add_session_chunk(chunk);
        ctx.capture_status = SUBGHZ_CAPTURE_RECEIVING;
        event.data = &chunk;
        event.type = SUBGHZ_EVENT_CAPTURE_DATA;
        notify_event(&event);

        return;
    }

    if (strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_ERROR) != NULL || strstr(buffer, SUBGHZ_COMMAND_RES_CAPTURE_FAIL) != NULL)
    {
        char err_message[255] = "Unknown capture error";
        get_field_value(buffer, "reason", err_message, sizeof(err_message));
        bool timed_out = strcmp(err_message, "timeout") == 0;
        finish_capture(timed_out ? SUBGHZ_CAPTURE_TIMEOUT : SUBGHZ_CAPTURE_FAILED,
                       timed_out ? SUBGHZ_ERR_TIMEOUT : SUBGHZ_ERR_IO, err_message, NULL);
    }
}

subghz_status_t subghz_service_init(const zv_config *config)
{
    if (config == NULL)
        return SUBGHZ_ERR_CONFIG;

    const uart_config_t *uart_config = &config->uart;
    uart_status_t uart_rc = uart_service_init(uart_config->device, uart_config->baudrate);
    if (uart_rc != UART_OK)
    {
        log_warning("subghz_service_init::UART init failed: %s\n", last_error());
        return (subghz_status_t)uart_rc;
    }

    add_event_callback(event_handler, UART_SUBGHZ_TAG_ID);
    uart_rc = uart_set_timeout_callback(UART_SUBGHZ_TAG_ID, on_event_timeout);
    if (uart_rc != UART_OK)
        return (subghz_status_t)uart_rc;

    if (!subghz_file_create_root_path(config->subghz.signals_path))
        return SUBGHZ_ERR_IO;

    snprintf(ctx.files_directory, sizeof(ctx.files_directory), "%s", config->subghz.signals_path);
    ctx.capture_status = SUBGHZ_CAPTURE_IDLE;
    ctx.capture_id = 0;
    ctx.replay_state = REPLAY_IDLE;

    return SUBGHZ_OK;
}

subghz_capture_status_t subghz_service_get_capture_status(void)
{
    return ctx.capture_status;
}

subghz_status_t subghz_service_start_capture(uint32_t frequency, uint32_t time_frame_ms)
{
    if (ctx.replay_state != REPLAY_IDLE || capture_is_active())
    {
        set_last_error("A Sub-GHz capture or replay is already in progress.");
        return SUBGHZ_ERR_BUSY;
    }

    if (frequency == 0 || time_frame_ms == 0)
    {
        set_last_error("Capture frequency and timeout must be positive.");
        ctx.capture_status = SUBGHZ_CAPTURE_FAILED;

        return SUBGHZ_ERR_INVALID;
    }

    ctx.capture_id = 0;
    ctx.capture_status = SUBGHZ_CAPTURE_WAIT_START;
    uart_status_t uart_rc = uart_send_formatted_line("%s|freq_hz=%" PRIu32 "|timeout_ms=%" PRIu32,
        SUBGHZ_COMMAND_REQ_CAPTURE, frequency, time_frame_ms);

    if (uart_rc != UART_OK)
    {
        ctx.capture_status = SUBGHZ_CAPTURE_FAILED;
        log_warning("subghz_start_capture error: %s\n", last_error());
        return (subghz_status_t)uart_rc;
    }

    // Allow time for command processing and transfer of the captured blocks.
    uart_rc = uart_start_response_timeout(UART_SUBGHZ_TAG_ID, (uint64_t)time_frame_ms + SUBGHZ_TIMEOUT_FIXED_MARGIN_MS);
    if (uart_rc != UART_OK)
    {
        ctx.capture_status = SUBGHZ_CAPTURE_FAILED;
        return (subghz_status_t)uart_rc;
    }

    set_last_error(NULL);
    return SUBGHZ_OK;
}

void subghz_service_add_event_callback(subghz_service_handler new_cb)
{
    ctx.service_handler = new_cb;
}

subghz_status_t subghz_service_save(subghz_capture_session_t *session, const char *file_name)
{
    if (session == NULL)
        return SUBGHZ_ERR_IO;

    time_t timestamp = time(NULL);
    char file_name_path[512];

    int written = 0;
    if (file_name == NULL)
    {
        written = snprintf(file_name_path, sizeof(file_name_path), "%s/%" PRIu64 "_%ld.sub",
            ctx.files_directory, session->capture_id, (long)timestamp);
    }
    else
    {
        written = snprintf(file_name_path, sizeof(file_name_path), "%s/%s.sub",
            ctx.files_directory, file_name);
    }

    if (written < 0 || (size_t)written >= sizeof(file_name_path))
        return SUBGHZ_ERR_IO;

    if (!subghz_file_create(file_name_path, session, FuriHalSubGhzPresetOok270Async))
    {
        log_error("Unable to save Sub-GHz capture: %s", file_name_path);
        return SUBGHZ_ERR_IO;
    }

    return SUBGHZ_OK;
}

subghz_status_t subghz_service_read(const char *file_name_path, subghz_flipper_sub_file_t *out)
{
    if (!file_name_path || !out)
        return SUBGHZ_ERR_INVALID;

    if (!sughz_file_read(file_name_path, out))
    {
        set_last_error("Unable to read Sub-GHz signal file.");
        return SUBGHZ_ERR_IO;
    }

    return SUBGHZ_OK;
}

subghz_status_t subghz_service_send_signal(const subghz_flipper_sub_file_t *signal, int times)
{
    if (!signal || !signal->raw_data || times <= 0)
    {
        set_last_error("Signal and a valid repetition count are required.");
        return SUBGHZ_ERR_INVALID;
    }

    if (strcmp(signal->protocol, "RAW") != 0)
    {
        set_last_error("Replay supports RAW signals.");
        return SUBGHZ_ERR_UNSUPPORTED;
    }

    if (ctx.replay_state != REPLAY_IDLE || capture_is_active())
    {
        set_last_error("A Sub-GHz capture or replay is already in progress.");
        return SUBGHZ_ERR_BUSY;
    }

    replay.count = 0;
    replay.seq = 0;
    uint64_t duration_us = 0;
    const char *cursor = signal->raw_data;
    while (*cursor)
    {
        while (isspace((unsigned char)*cursor))
            cursor++;
        if (!*cursor)
            break;

        char *end;
        errno = 0;
        long value = strtol(cursor, &end, 10);
        if (errno == ERANGE)
        {
            set_last_error("RAW timing exceeds the range of long.");
            return SUBGHZ_ERR_INVALID;
        }

        if (end == cursor || (*end && !isspace((unsigned char)*end)))
        {
            set_last_error("RAW timings must be numbers separated by whitespace.");
            return SUBGHZ_ERR_INVALID;
        }

        if (value == 0 || value < INT32_MIN || value > INT32_MAX)
        {
            set_last_error("RAW timing must be a nonzero int32_t value.");
            return SUBGHZ_ERR_INVALID;
        }

        if (replay.count == SUBGHZ_SEND_MAX_TIMINGS)
        {
            set_last_error("RAW signal exceeds the 256 pulse buffer.");
            return SUBGHZ_ERR_INVALID;
        }

        replay.timings[replay.count++] = (int32_t)value;
        duration_us += value < 0 ? -(int64_t)value : (int64_t)value;
        cursor = end;
    }
 
    if (!replay.count)
    {
        set_last_error("RAW signal is empty.");
        return SUBGHZ_ERR_INVALID;
    }

    static uint32_t next_id;
    if (++next_id == 0)
        ++next_id;

    replay.id = next_id;
    replay.chunks = (replay.count + SUBGHZ_SEND_CHUNK_VALUES - 1) / SUBGHZ_SEND_CHUNK_VALUES;

    // Split before multiplying so even long int32_t pulses cannot overflow the timeout
    uint64_t duration_ms = duration_us / SUBGHZ_US_PER_MS;
    uint64_t remainder_us = duration_us % SUBGHZ_US_PER_MS;
    replay.completion_timeout_ms = duration_ms * times
        + (remainder_us * times + SUBGHZ_US_PER_MS - 1) / SUBGHZ_US_PER_MS
        + (uint64_t)times * SUBGHZ_TIMEOUT_PER_REPEAT_MS + SUBGHZ_TIMEOUT_FIXED_MARGIN_MS;

    uart_status_t result = uart_send_formatted_line(
        "%s|id=%" PRIu32 "|freq=%d|repeats=%d|count=%" PRIu16 "|chunks=%" PRIu16,
        SUBGHZ_COMMAND_REQ_SEND_BEGIN, replay.id, signal->frecuency, times, replay.count, replay.chunks);

    if (result != UART_OK)
        return (subghz_status_t)result;

    result = uart_start_response_timeout(UART_SUBGHZ_TAG_ID, SUBGHZ_UPLOAD_RESPONSE_TIMEOUT_MS);
    if (result != UART_OK)
        return (subghz_status_t)result;

    ctx.replay_state = REPLAY_WAIT_BEGIN_OK;
    set_last_error(NULL);

    return SUBGHZ_OK;
}
