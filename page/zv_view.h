#ifndef ZV_VIEW_H
#define ZV_VIEW_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zv_view_t {
    lv_obj_t *main_container;   // Main container in the screen is a lv_obj
    lv_obj_t *header;           // Optional: Topbar to show the title and back button
    lv_obj_t *content;
} zv_view_t;

zv_view_t *zv_view_new(const char *title);
void zv_view_destroy(zv_view_t *view);

#ifdef __cplusplus
}
#endif

#endif /* ZV_VIEW_H */
