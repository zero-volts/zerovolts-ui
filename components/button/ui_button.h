#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t btn_id;
    void *user_data;
} event_data_btn;

typedef struct ui_button ui_button;
typedef void (*ui_btton_on_click)(event_data_btn *event);

ui_button *ui_button_create(lv_obj_t *parent, int width, int height, const char *text);
ui_button *ui_icon_button_create(lv_obj_t *parent, int width, int height, const char *icon);
void ui_button_set_on_click(ui_button *button, ui_btton_on_click callback, void *user_data);
void ui_button_destroy(ui_button *button);

#ifdef __cplusplus
}
#endif

#endif /* UI_BUTTON_H */
