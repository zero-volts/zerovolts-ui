#ifndef UI_TEXT_INPUT_H
#define UI_TEXT_INPUT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ui_text_input ui_text_input;

ui_text_input *ui_text_create(lv_obj_t *parent, int width, int height, const char *hint);
const char *ui_text_get_text(ui_text_input *input_text);
void ui_text_clear(ui_text_input *input_text);

#ifdef __cplusplus
}
#endif

#endif /* UI_TEXT_INPUT_H */
