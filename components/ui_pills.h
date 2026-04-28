#ifndef UI_PILLS_H
#define UI_PILLS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ui_pills ui_pills;
typedef void (*ui_pills_event_cb_t)(ui_pills *pills, int index, const char *label, void *user_data);

ui_pills *create_pills(lv_obj_t *parent);
ui_pills *create_pills_sized(lv_obj_t *parent, int container_width, int pill_height);
void pills_add(ui_pills *pills, const char *label);
void pills_set_active(ui_pills *pills, int index);
int pills_get_active(const ui_pills *pills);
void pills_set_event_cb(ui_pills *pills, ui_pills_event_cb_t cb, void *user_data);
void pills_clear(ui_pills *pills);
void destroy_pills(ui_pills *pills);

#ifdef __cplusplus
}
#endif

#endif /* UI_PILLS_H */
