#ifndef HID_VIEW_H
#define HID_VIEW_H

#include "lvgl.h"
#include "config.h"
#include "page/base_view.h"
#include "page/hid/hid_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    base_view base;

    lv_obj_t *list;
    lv_obj_t *status;
    lv_obj_t *toggle;
    lv_obj_t *selected_lbl;
    hid_controller controller;
} hid_view;

lv_obj_t *hid_view_create(lv_obj_t *menu, const zv_config *cfg);
void hid_refresh_scripts(hid_view *self);
const char *hid_get_selected_script(hid_view *self);

#ifdef __cplusplus
}
#endif

#endif /* HID_VIEW_H */
