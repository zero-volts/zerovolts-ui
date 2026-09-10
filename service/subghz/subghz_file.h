#ifndef SUBGHZ_FILE_H
#define SUBGHZ_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "app_context.h"

bool subghz_file_create_root_path(const char *path);
bool subghz_file_create(const char *file_name_path,  subghz_capture_session_t *session);

#ifdef __cplusplus
}
#endif

#endif /* SUBGHZ_FILE_H */
