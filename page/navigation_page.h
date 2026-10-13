#ifndef NAVIGATION_PAGE_H
#define NAVIGATION_PAGE_H

#include <stdbool.h>
#include "lvgl.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZV_SCREEN_MAX_DEEP 15

typedef struct zv_view_t zv_view_t;
typedef zv_view_t *(*zv_view_create_cb)(const zv_config *cfg);

typedef struct {
    zv_view_create_cb create_view;
    const zv_config *cfg;
} nav_ctx_t;

typedef struct {
    int id;
    zv_view_t *view;
} zv_nav_stack_t;

bool zv_nav_open(const nav_ctx_t *ctx);
void zv_nav_set_group(lv_group_t *group);
void zv_nav_update_group(lv_obj_t *page);
void zv_nav_back_cb(lv_event_t *e);

bool zv_nav_push(zv_view_t *new_view);
bool zv_nav_pop(void);
bool zv_nav_is_empty(void);

#ifdef __cplusplus
}
#endif

#endif /* NAVIGATION_PAGE_H */
