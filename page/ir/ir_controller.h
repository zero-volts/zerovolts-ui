#ifndef IR_CONTROLLER_H
#define IR_CONTROLLER_H

#include "service/ir_service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MAX_NAME 128

typedef struct {
    char *remotes_root;
    ir_context ir_ctx;
} ir_remote_ctx;

// Remote control definition
typedef struct {
    char name[IR_MAX_NAME];
    int button_count;
} ir_remote_info;

typedef struct {
    ir_remote_info *remotes;
    size_t count;
} ir_remote_list;

typedef struct {
    char name[IR_MAX_NAME];
} ir_button;

typedef struct {
    ir_button *buttons;
    size_t count;
} ir_button_list;

ir_status_t ir_controller_init(const ir_remote_ctx *remote_ctx);
ir_status_t ir_controller_create_remote(const char *remote_name);
ir_status_t ir_controller_list_remotes(ir_remote_list *out);
ir_status_t ir_controller_list_buttons(const char *remote_name, ir_button_list *out);
ir_status_t ir_controller_learn_button(const char *remote_name, const char *button_name);
ir_status_t ir_controller_send_button(const char *remote_name, const char *button_name);
const char *ir_controller_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_CONTROLLER_H */
