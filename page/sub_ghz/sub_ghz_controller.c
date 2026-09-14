#include "sub_ghz_controller.h"

#include <string.h>
#include <inttypes.h>

#include "types.h"
#include "utils/logger.h"
#include "utils/error_handler.h"

static data_capture_handler capture_handler = NULL;

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

                if (subghz_service_save((subghz_capture_session_t *)event->data) != SUBGHZ_OK)
                {
                    event_arg.message = "No se pudo guardar la captura Sub-GHz";
                    capture_handler(UI_ERROR, &event_arg);
                }
                else
                    capture_handler(UI_DONE, NULL);
                    
                break;
            case SUBGHZ_EVENT_CAPTURE_DATA:

                event_arg.data = (subghz_data_chunk_t *)event->data;
                capture_handler(UI_LOADING, &event_arg);
                break;
            case SUBGHZ_EVENT_CAPTURE_ERROR:
               event_arg = {
                    .data = NULL,
                    .message = event->message
                };

                capture_handler(UI_ERROR, &event_arg);
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