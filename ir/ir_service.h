#ifndef IR_SERVICE_H
#define IR_SERVICE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IR_NAME_MAX 128

typedef enum {
    IR_OK = 0,
    IR_ERR_CONFIG = -1,
    IR_ERR_IO = -2,
    IR_ERR_TIMEOUT = -3,
    IR_ERR_INVALID = -4,
    IR_ERR_UNSUPPORTED = -5
} ir_status_t;

typedef struct {
    char name[IR_NAME_MAX];
    int button_count;
} ir_remote_info_t;

typedef struct {
    ir_remote_info_t *items;
    size_t count;
} ir_remote_list_t;

typedef struct {
    char name[IR_NAME_MAX];
} ir_button_info_t;

typedef struct {
    ir_button_info_t *items;
    size_t count;
} ir_button_list_t;


typedef struct {
    const char *backend;        // How to implement the communication with the system by default "irctl"
    const char *tx_dev;         // The transmissor /dev path like "/dev/lirc0"
    const char *rx_dev;         // the reciever /dev path
    const char *remotes_root;   // Where to store the remotes lerned
    int learn_timeout_ms;       // The timeout to wait when is listening a new signal.
} ir_service_cfg_t;

ir_status_t ir_service_init(const ir_service_cfg_t *cfg);

ir_status_t ir_service_list_remotes(ir_remote_list_t *out);
void ir_service_free_remotes(ir_remote_list_t *list);

ir_status_t ir_service_create_remote(const char *remote_name);

ir_status_t ir_service_list_buttons(const char *remote_name, ir_button_list_t *out);
void ir_service_free_buttons(ir_button_list_t *list);

ir_status_t ir_service_learn_button(const char *remote_name, const char *button_name);
ir_status_t ir_service_send_button(const char *remote_name, const char *button_name);

const char *ir_service_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_SERVICE_H */
