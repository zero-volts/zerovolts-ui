#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include <libgen.h>   // dirname
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *file_name;
    const char *file_path;
} file_desc;

typedef void (*file_callback)(const file_desc *description, void *obj_target);

int file_exists(const char *path);
int get_executable_dir(char *out, size_t out_size);
bool file_has_extension(const char *name, const char *ext);

char *read_file_as_buffer(const char *path, long* file_size_out);
int write_entire_file(const char *path, const char *data, size_t len);

void get_file_list(const char* directory_path, file_callback callback, void *obj_target);
cJSON *read_json_file(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* FILE_H */