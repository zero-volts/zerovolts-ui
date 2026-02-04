#ifndef IR_LEARN_BUTTON_H
#define IR_LEARN_BUTTON_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ir_learn_button_page_create(lv_obj_t *menu);
bool ir_learn_button_keyboard_is_visible(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_LEARN_BUTTON_H */
