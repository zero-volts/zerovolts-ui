#ifndef IR_REMOTES_H
#define IR_REMOTES_H

#include "page/zv_view.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *ir_remotes_page_create(const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* IR_REMOTES_H */
