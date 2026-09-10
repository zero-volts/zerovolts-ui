#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bt_context_t bt_context_t;
typedef struct  {
    uint64_t capture_id;
    uint32_t frequency;
    uint16_t next_seq;
    uint16_t expected_chunks;
    uint16_t received_chunks;
    uint16_t count;
    int32_t timings[SUBGHZ_CAPTURE_MAX_TIMINGS];
    bool completed;
} subghz_capture_session_t;

typedef struct subghz_sessions subghz_sessions;
typedef struct {
    bt_context_t *bt;
    subghz_sessions *sessions;
} app_context_t;

app_context_t *app_context_get();

void bt_context_add_device(device_t *device);
void bt_context_clear_devices(void);
device_t *bt_context_get_devices(void);
void bt_context_set_selected(const device_t *device);
const device_t *bt_context_get_selected(void);
int bt_context_devices_length(void);

void subghz_add_session_chunk(const subghz_data_chunk_t chunk);
bool subghz_set_session_completed(uint64_t session_id);
subghz_capture_session_t *subghz_get_session_by(uint64_t session_id);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONTEXT_H */