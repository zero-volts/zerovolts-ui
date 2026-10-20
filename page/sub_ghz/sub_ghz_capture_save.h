#ifndef SUBGHZ_CAPTURE_SAVE_H
#define SUBGHZ_CAPTURE_SAVE_H

#include "page/zv_view.h"
#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*subghz_save_confirm_cb)(const char *file_name);
zv_view_t *subghz_capture_save_create(subghz_save_confirm_cb on_confirm);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_CAPTURE_SAVE_H */