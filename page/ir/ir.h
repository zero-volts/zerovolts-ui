#ifndef IR_H
#define IR_H

#include "page/zv_view.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif


zv_view_t *ir_page_create(const zv_config *cfg);


#ifdef __cplusplus
}
#endif

#endif /* IR_H */