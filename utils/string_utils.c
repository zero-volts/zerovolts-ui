#include "string_utils.h"

#include <ctype.h>
#include <string.h>

bool zv_is_valid_name_char(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_' || c == '-' || c == ' ';
}

bool zv_sanitize_name(const char *src, char *dst, size_t dst_sz)
{
    size_t j = 0;

    if (!src || !dst || dst_sz == 0)
        return false;

    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_sz; i++) {
        char c = src[i];
        if (!zv_is_valid_name_char(c))
            c = '_';
        dst[j++] = c;
    }

    while (j > 0 && dst[j - 1] == ' ')
        j--;

    dst[j] = '\0';
    return j > 0;
}

bool zv_has_whitespace(const char *src)
{
    if (!src)
        return false;

    for (size_t i = 0; src[i] != '\0'; i++) {
        if (isspace((unsigned char)src[i]))
            return true;
    }

    return false;
}

void zv_trim_inplace(char *s)
{
    size_t start = 0;
    size_t end;
    size_t len;

    if (!s)
        return;

    len = strlen(s);
    while (start < len && isspace((unsigned char)s[start]))
        start++;

    end = len;
    while (end > start && isspace((unsigned char)s[end - 1]))
        end--;

    if (start > 0)
        memmove(s, s + start, end - start);

    s[end - start] = '\0';
}
