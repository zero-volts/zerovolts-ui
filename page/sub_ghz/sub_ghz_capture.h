#ifndef SUBGHZ_CAPTURE_H
#define SUBGHZ_CAPTURE_H

#include "page/zv_view.h"
#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *subghz_capture_page_create(const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_CAPTURE_H */
