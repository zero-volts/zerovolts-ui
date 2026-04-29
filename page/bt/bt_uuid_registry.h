#ifndef BT_UUID_REGISTRY_H
#define BT_UUID_REGISTRY_H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const char *uuid;
    const char *name;
} uuid_name_t;

const char *lookup_name(const char *uui, char *normalized_uuid, size_t normalized_size);

#ifdef __cplusplus
}
#endif

#endif /* BT_UUID_REGISTRY_H */
