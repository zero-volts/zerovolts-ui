#ifndef SUBGHZ_REPLAY_VIEW_H
#define SUBGHZ_REPLAY_VIEW_H

#include "page/zv_view.h"
#include "config.h"
#include "page/sub_ghz/sub_ghz_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

zv_view_t *subghz_replay_create(const zv_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_REPLAY_VIEW_H */
