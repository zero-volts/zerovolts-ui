#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t btn_id;
    void *user_data;
    lv_event_t *lv_event;
} event_data_btn;

typedef enum {
    Normal = 1,
    Warning,
    Success,
    Error
} action_style_type_t;

typedef struct ui_button ui_button;
typedef void (*ui_button_on_click)(event_data_btn *event);

ui_button *ui_button_create(lv_obj_t *parent, int width, int height, const char *text);
ui_button *ui_icon_button_create(lv_obj_t *parent, int width, int height, const char *icon);
void ui_button_set_on_click(ui_button *button, ui_button_on_click callback, void *user_data);
void ui_button_set_text(ui_button *button, const char *text);
void ui_button_set_loading(ui_button *button, bool loading);
void ui_button_set_action_style(ui_button *button, action_style_type_t style);
lv_obj_t *ui_button_get_obj(ui_button *button);
void ui_button_hide(ui_button *button, bool hide);
void ui_button_enable(ui_button *button, bool enable);
void ui_button_destroy(ui_button *button);


#ifdef __cplusplus
}
#endif

#endif /* UI_BUTTON_H */
