#ifndef TOP_BAR_H
#define TOP_BAR_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *container;
    lv_obj_t *title;
    lv_obj_t *clock;
} top_bar_t;

top_bar_t *top_bar_create(lv_obj_t *parent);
void top_bar_set_title(top_bar_t *bar, const char *title);
void top_bar_set_time(top_bar_t *bar, const char *time_str);

#ifdef __cplusplus
}
#endif

#endif /* TOP_BAR_H */
