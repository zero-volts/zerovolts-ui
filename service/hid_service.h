#ifndef HID_SERVICE_H
#define HID_SERVICE_H

#include <stddef.h>
#include "utils/file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HID_OK = 0,
    HID_ERR_CONFIG = -1,
    HID_ERR_IO = -2,
    HID_ERR_INVALID = -3
} hid_status_t;

hid_status_t hid_service_enable(void);
hid_status_t hid_service_disable(void);
hid_status_t hid_service_start_session(void);
hid_status_t hid_service_stop_session(void);
hid_status_t hid_service_list_scripts_cb(const char *scripts_dir, file_callback callback, void *user_data);
const char *hid_service_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* HID_SERVICE_H */
