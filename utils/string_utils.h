#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool zv_is_valid_name_char(char c);
bool zv_sanitize_name(const char *src, char *dst, size_t dst_sz);
bool zv_has_whitespace(const char *src);
void zv_trim_inplace(char *s);

#ifdef __cplusplus
}
#endif

#endif /* STRING_UTILS_H */
