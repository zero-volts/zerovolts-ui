#pragma once
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *scripts_dir;     // ej: "/opt/zerovolts/hid-scripts"
    const char *selected_path;   // ej: "/var/lib/zerovolts/hid-selected.txt"
} zv_hid_cfg_t;

lv_obj_t *hid_page_create(lv_obj_t *menu, const zv_hid_cfg_t *cfg);
void hid_refresh_scripts(void);
const char *hid_get_selected_script(void);

#ifdef __cplusplus
}
#endif