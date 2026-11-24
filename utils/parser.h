#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool get_field_value(const char *kv_buffer, const char *key, char *out, size_t out_size);
bool protocol_message_is(const char *line, const char *token);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
