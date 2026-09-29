#ifndef SUBGHZ_FILE_H
#define SUBGHZ_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "app_context.h"

typedef enum {
    FuriHalSubGhzPresetOok270Async = 1,
    FuriHalSubGhzPresetOok650Async,
    FuriHalSubGhzPreset2FSKDev238Async,
    FuriHalSubGhzPreset2FSKDev12KAsync,
    FuriHalSubGhzPreset2FSKDev476Async
} subghz_flipper_preset_t;

bool subghz_file_create_root_path(const char *path);
bool subghz_file_create(const char *file_name_path,  subghz_capture_session_t *session, subghz_flipper_preset_t preset);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_FILE_H */
