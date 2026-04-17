#ifndef UI_LOADING_BTN_H
#define UI_LOADING_BTN_H


#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ui_loading_button ui_loading_button;

ui_loading_button *create_loading_btn(lv_obj_t *parent, int width, int height, const char *text);
void loading_set_event_cb(ui_loading_button *comp, lv_event_cb_t cb, void *user_data);
void loading_button_set_text(ui_loading_button *comp, const char *text);
void loading_button_set_loading(ui_loading_button *comp, bool loading);
void destroy_loading_btn(ui_loading_button *comp);

#ifdef __cplusplus
}
#endif

#endif /* UI_LOADING_BTN_H */
