#ifndef IR_H
#define IR_H

#include "lvgl.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif


lv_obj_t *ir_page_create(lv_obj_t *menu, const zv_config *cfg);


#ifdef __cplusplus
}
#endif

#endif /* IR_H */