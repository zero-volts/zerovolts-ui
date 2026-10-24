#ifndef SUBGHZ_CONTROLLER_H
#define SUBGHZ_CONTROLLER_H

#include "types.h"
#include "config.h"
#include "app_context.h"
#include "service/subghz/subghz_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    subghz_data_chunk_t *data;
    uint64_t session_id;
    const char *message;
} subghz_event_arg;

/* Event data and message are borrowed and only valid during the callback. */
typedef void (*subghz_event_handler)(ui_status_t status, subghz_event_arg *event);

subghz_status_t subghz_controller_init(const zv_config *config);
subghz_status_t subghz_controller_start_capture(uint32_t frequency, uint32_t time_frame_ms);
void subghz_controller_set_event_cb(subghz_event_handler callback);
subghz_status_t subghz_controller_save_data(subghz_capture_session_t *session, const char *file_name);
subghz_status_t subghz_controller_replay(const char *file_name, int times);
bool subghz_controller_discard_session( uint64_t session_id);
subghz_status_t subghz_controller_list_signals(const char *signals_path, zv_file_item_list *out_list);
void subghz_controller_free_signal_list(zv_file_item_list *list);
const char *subghz_controller_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_CONTROLLER_H */
