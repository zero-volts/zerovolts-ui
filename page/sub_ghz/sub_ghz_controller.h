#ifndef SUBGHZ_CONTROLLER_H
#define SUBGHZ_CONTROLLER_H

#include "service/uart_service.h"
#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*subghz_handler)( ui_status_t status);
typedef void (*data_capture_handler)(ui_status_t status, subghz_data_chunk_t *data);

uart_status_t subghz_controller_init(const subghz_config_t *config, const uart_config_t *uart_config);
uart_status_t subghz_start_capture(int frequency, int time_frame_ms);
void subghz_set_cb(subghz_handler new_callback);
void subghz_set_capture_cb(data_capture_handler callback);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_CONTROLLER_H */
