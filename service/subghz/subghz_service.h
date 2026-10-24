#ifndef SUBGHZ_SERVICE_H
#define SUBGHZ_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "config.h"
#include "app_context.h"
#include "utils/file.h"
#include "service/subghz/subghz_file.h"
typedef enum {
    SUBGHZ_OK               = 0,
    SUBGHZ_ERR_CONFIG       = -1,
    SUBGHZ_ERR_IO           = -2,
    SUBGHZ_ERR_TIMEOUT      = -3,
    SUBGHZ_ERR_INVALID      = -4,
    SUBGHZ_ERR_UNSUPPORTED  = -5
} subghz_status_t;

typedef enum {
    SUBGHZ_EVENT_NONE = 0,
    SUBGHZ_EVENT_CAPTURE_STARTED,
    SUBGHZ_EVENT_CAPTURE_DATA,
    SUBGHZ_EVENT_CAPTURE_READY,
    SUBGHZ_EVENT_CAPTURE_SAVED,
    SUBGHZ_EVENT_CAPTURE_ERROR,
    SUBGHZ_EVENT_SEND_DONE,
    SUBGHZ_EVENT_SEND_ERROR
} subghz_event_type_t;

typedef struct {
    subghz_event_type_t type;
    char message[255];
    void *data;
    subghz_status_t status; //Result for SEND_DONE/SEND_ERROR
} subghz_service_event_t;

typedef void (*subghz_service_handler)(subghz_service_event_t *event);

subghz_status_t subghz_service_init(const zv_config *config);
subghz_status_t subghz_service_start_capture(uint32_t frequency, uint32_t time_frame_ms);
void subghz_service_add_event_callback(subghz_service_handler new_cb);
subghz_status_t subghz_service_save(subghz_capture_session_t *session, const char *file_name_path);
subghz_status_t subghz_service_read(const char *file_name_path, subghz_flipper_sub_file_t *out);
subghz_status_t subghz_service_send_signal(const subghz_flipper_sub_file_t *signal, int times);
subghz_status_t subghz_service_list_signals_cb(const char *signals_dir, file_callback callback, void *user_data);


#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_SERVICE_H */