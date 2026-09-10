#ifndef SUBGHZ_CONTROLLER_H
#define SUBGHZ_CONTROLLER_H

#include "service/subghz/subghz_service.h"
#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    subghz_data_chunk_t *data;
    const char *message;
} subghz_event_arg;

typedef void (*data_capture_handler)(ui_status_t status, subghz_event_arg *event);

subghz_status_t subghz_controller_init(const zv_config *config);
subghz_status_t subghz_controller_start_capture(uint32_t frequency, uint32_t time_frame_ms);
void subghz_controller_set_capture_cb(data_capture_handler callback);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_CONTROLLER_H */
