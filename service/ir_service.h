#ifndef IR_SERVICE_H
#define IR_SERVICE_H

#include <stddef.h>
#include "utils/file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MAX_NAME         256
#define MAX_IR_DEV_PATH     256
#define EXCAPED_DEV_PATH    512
#define ESCAPED_TMP_PATH    1024
#define COMMAND_SIZE        2048
#define BACKEND_TYPE_IRCTL  "irctl"
#define BACKEND_TYPE_LIRC   "lircdev"

typedef enum {
    IR_OK               = 0,
    IR_ERR_CONFIG       = -1,
    IR_ERR_IO           = -2,
    IR_ERR_TIMEOUT      = -3,
    IR_ERR_INVALID      = -4,
    IR_ERR_UNSUPPORTED  = -5
} ir_status_t;

typedef struct {
    char tx_dev[MAX_IR_DEV_PATH];   // The transmissor /dev path like /dev/lirc0""
    char rx_dev[MAX_IR_DEV_PATH];   // The reciever /dev path
    int timeout_ms;                 // The timeout to wait when is listening hte signal
    const char *backend;            // How to implement the comunnication with the system, by default "BACKEND_TYPE_IRCTL"
} ir_context;

typedef void (*ir_raw_cb)(const file_desc *desc, void *user);
typedef struct {
    ir_raw_cb cb;
    void *data;
}ir_callback_event;

ir_status_t ir_service_init(const ir_context *ctx);
ir_status_t ir_learn_raw (const char *out_raw_path);
ir_status_t ir_send_raw(const char *raw_path);

ir_status_t ir_list_raw_files_cb(const char *dir, ir_callback_event *event);
const char *ir_service_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_SERVICE_H */
