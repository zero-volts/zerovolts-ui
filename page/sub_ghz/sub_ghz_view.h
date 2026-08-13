#ifndef SUBGHZ_VIEW_H
#define SUBGHZ_VIEW_H

#include "lvgl.h"
#include "config.h"
#include "page/base_view.h"
#include "page/sub_ghz/sub_ghz_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    base_view base;
    subghz_config_t config;
} subghz_view;

lv_obj_t *subghz_view_create(lv_obj_t *menu, const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_VIEW_H */
