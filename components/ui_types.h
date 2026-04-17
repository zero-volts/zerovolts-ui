#ifndef UI_TYPES_H
#define UI_TYPES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
} obj_size_t;

typedef struct {
    const char *path;
    obj_size_t size;
} obj_icon_t;

#ifdef __cplusplus
}
#endif

#endif /* UI_TYPES_H */
