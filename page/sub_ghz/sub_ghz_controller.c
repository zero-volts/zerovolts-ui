#include "sub_ghz_controller.h"

#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "types.h"
#include "utils/logger.h"
#include "utils/error_handler.h"

static data_capture_handler capture_handler = NULL;

typedef struct {
    subghz_signal_list *list;
    size_t capacity;
    bool failed;
} signal_list_builder;

static void collect_signal(const file_desc *file, void *user_data)
{
    signal_list_builder *builder = (signal_list_builder *)user_data;
    if (builder->failed)
        return;

    if (builder->list->count == builder->capacity)
    {
        if (builder->capacity > SIZE_MAX / 2 / sizeof(subghz_signal_item))
        {
            builder->failed = true;
            return;
        }

        size_t capacity = builder->capacity ? builder->capacity * 2 : 8;
        subghz_signal_item *signals = (subghz_signal_item *)realloc(builder->list->signals, capacity * sizeof(*signals));
        if (!signals)
        {
            builder->failed = true;
            return;
        }

        builder->list->signals = signals;
        builder->capacity = capacity;
    }

    subghz_signal_item *item = &builder->list->signals[builder->list->count++];
    snprintf(item->name, sizeof(item->name), "%s", file->file_name);
    snprintf(item->path, sizeof(item->path), "%s", file->file_path);
}

const char *subghz_controller_last_error(void)
{
    return last_error();
}

void subghz_controller_free_signal_list(subghz_signal_list *list)
{
    if (!list)
        return;

    free(list->signals);
    memset(list, 0, sizeof(*list));
}

subghz_status_t subghz_controller_list_signals(const char *signals_path, subghz_signal_list *out_list)
{
    if (!out_list)
    {
        set_last_error("Signal list is required.");
        return SUBGHZ_ERR_INVALID;
    }

    memset(out_list, 0, sizeof(*out_list));
    if (!signals_path || !signals_path[0])
    {
        set_last_error("Signals directory is not configured.");
        return SUBGHZ_ERR_CONFIG;
    }

    signal_list_builder builder = { .list = out_list };
    subghz_status_t result = subghz_service_list_signals_cb(signals_path, collect_signal, &builder);
    if (result != SUBGHZ_OK || builder.failed)
    {
        subghz_controller_free_signal_list(out_list);
        if (result != SUBGHZ_OK)
            return result;

        set_last_error("Not enough memory to build signal list.");
        return SUBGHZ_ERR_IO;
    }

    set_last_error(NULL);
    return SUBGHZ_OK;
}

static void event_handler(subghz_service_event_t *event)
{
    if (capture_handler != NULL)
    {
        subghz_event_arg event_arg = {0};
        switch(event->type)
        {
            case SUBGHZ_EVENT_CAPTURE_STARTED:
                capture_handler(UI_LOADING, NULL);
                break;
            case SUBGHZ_EVENT_CAPTURE_READY:

                event_arg.session_id = ((subghz_capture_session_t *)event->data)->capture_id;
                capture_handler(UI_DONE, &event_arg);
                    
                break;
            case SUBGHZ_EVENT_CAPTURE_DATA:

                event_arg.data = (subghz_data_chunk_t *)event->data;
                capture_handler(UI_LOADING, &event_arg);
                break;
            case SUBGHZ_EVENT_CAPTURE_ERROR:
               subghz_event_arg event_arg2 = {
                    .data = NULL,
                    .message = event->message
                };

                capture_handler(UI_ERROR, &event_arg2);
                break;
            default:
                break;
        }
    }
}

void subghz_controller_set_capture_cb(data_capture_handler callback)
{
    capture_handler = callback;
}

subghz_status_t subghz_controller_init(const zv_config *config)
{
    if (config == NULL)
        return SUBGHZ_ERR_CONFIG;

    subghz_status_t subghz_rc = subghz_service_init(config);
    if (subghz_rc != SUBGHZ_OK)
    {
        log_warning("subghz_controller_init init failed: %s\n", last_error());
        return subghz_rc;
    }

    subghz_service_add_event_callback(event_handler);
    return SUBGHZ_OK;
}

subghz_status_t subghz_controller_start_capture(uint32_t frequency, uint32_t time_frame_ms)
{
    subghz_status_t subghz_rc = subghz_service_start_capture(frequency, time_frame_ms);
    if (subghz_rc != SUBGHZ_OK)
    {
        log_warning("subghz_start_capture error: %s\n", last_error());
        return subghz_rc;
    }

    return SUBGHZ_OK;
}

subghz_status_t subghz_controller_save_data(subghz_capture_session_t *session, const char *file_name)
{
    if (!session)
        return SUBGHZ_ERR_INVALID;

    uint64_t session_id = session->capture_id;
    subghz_status_t result = subghz_service_save(session, file_name);
    if (result != SUBGHZ_OK)
        return result;

    subghz_remove_session_by(session_id);
    return SUBGHZ_OK;
}

bool subghz_controller_discard_session( uint64_t session_id)
{
    return subghz_remove_session_by(session_id);
}