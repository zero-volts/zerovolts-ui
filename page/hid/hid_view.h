#ifndef HID_VIEW_H
#define HID_VIEW_H

#include "lvgl.h"
#include "config.h"
#include "page/zv_view.h"
#include "page/hid/hid_controller.h"
#include "components/list/ui_list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    zv_view_t *base;

    ui_list *list;
    lv_obj_t *status;
    lv_obj_t *toggle;
    lv_obj_t *selected_lbl;
    hid_controller controller;
} hid_view;

zv_view_t *hid_view_create(const zv_config *cfg);
void hid_refresh_scripts(hid_view *self);
const char *hid_get_selected_script(hid_view *self);

#ifdef __cplusplus
}
#endif

#endif /* HID_VIEW_H */
