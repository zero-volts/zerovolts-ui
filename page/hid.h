#ifndef HID_H
#define HID_H

#include "lvgl.h"
#include <stdbool.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif


lv_obj_t *hid_page_create(lv_obj_t *menu, const zv_config *cfg);
void hid_refresh_scripts(void);
const char *hid_get_selected_script(void);

#ifdef __cplusplus
}
#endif

#endif /* HID_H */