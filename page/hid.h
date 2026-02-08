#ifndef HID_H
#define HID_H

#include "lvgl.h"
#include "config.h"
#include "base_view.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    base_view base;

    lv_obj_t *list;
    lv_obj_t *status;
    lv_obj_t *toggle;
    lv_obj_t *selected_lbl;

    struct script {
        char scripts_dir[512];
        char selected_path[512];
        char selected_script[512];
        bool hid_enabled;
    } script;
} hid_view;

lv_obj_t *hid_view_create(lv_obj_t *menu, const zv_config *cfg);
void hid_refresh_scripts(hid_view *self);
const char *hid_get_selected_script(hid_view *self);

#ifdef __cplusplus
}
#endif

#endif /* HID_H */