#ifndef UI_PANEL_H
#define UI_PANEL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ui_panel_create(lv_obj_t *parent, int width, int height);
void ui_panel_set_flex_flow(lv_obj_t *panel, lv_flex_flow_t new_flex_flow);
#ifdef __cplusplus
}
#endif

#endif /* UI_PANEL_H */
