#ifndef IR_SEND_SIGNAL_H
#define IR_SEND_SIGNAL_H

#include "page/zv_view.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *ir_send_signal_page_create(const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* IR_SEND_SIGNAL_H */
