#ifndef COMPONENT_HELPER_H
#define COMPONENT_HELPER_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *find_first_element_child(lv_obj_t *parent, const lv_obj_class_t * class_p);
lv_obj_t *create_square_main_button(lv_obj_t *parent, const char *text, const char *icon, lv_event_cb_t cb, void *user_data);
void rotate_icon_by_tag(lv_obj_t * btn, int32_t angle);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENT_HELPER_H */
