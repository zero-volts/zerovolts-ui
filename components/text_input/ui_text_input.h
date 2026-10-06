#ifndef UI_TEXT_INPUT_H
#define UI_TEXT_INPUT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ui_text_input ui_text_input;
typedef struct {
    bool use_on_screen_keyboard;
    bool one_line;
} ui_text_input_options;

ui_text_input *ui_text_create(lv_obj_t *parent, int width, int height, const char *hint);
ui_text_input *ui_text_create_with_options(lv_obj_t *parent, int width, int height,
                                         const char *hint, const ui_text_input_options *options);
                                         
const char *ui_text_get_text(ui_text_input *input_text);
void keyboard_hide(ui_text_input *ui);
void ui_text_clear(ui_text_input *input_text);
void ui_text_destroy(ui_text_input *input_text);

#ifdef __cplusplus
}
#endif

#endif /* UI_TEXT_INPUT_H */
