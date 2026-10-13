#ifndef IR_NEW_REMOTE_H
#define IR_NEW_REMOTE_H

#include <stdbool.h>
#include "page/zv_view.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *ir_new_remote_page_create(const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* IR_NEW_REMOTE_H */
