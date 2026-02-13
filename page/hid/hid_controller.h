#ifndef HID_CONTROLLER_H
#define HID_CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "service/hid_service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HID_MAX_PATH 512

typedef struct {
    char name[256];
    char path[1024];
} hid_script_item;

typedef struct {
    hid_script_item *scripts;
    size_t count;
} hid_script_list;

typedef struct {
    char scripts_dir[HID_MAX_PATH];
    char selected_path[HID_MAX_PATH];
    char selected_script[HID_MAX_PATH];
    bool hid_enabled;
} hid_controller;

hid_status_t hid_controller_init(hid_controller *controller, const zv_config *cfg);
hid_status_t hid_controller_set_selected_script(hid_controller *controller, const char *script_path);
hid_status_t hid_controller_toggle(hid_controller *controller, bool enable);
hid_status_t hid_controller_list_scripts(hid_controller *controller, hid_script_list *out_list);
void hid_controller_free_script_list(hid_script_list *list);
const char *hid_controller_selected_script(const hid_controller *controller);
const char *hid_controller_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* HID_CONTROLLER_H */
