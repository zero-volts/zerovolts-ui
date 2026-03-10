#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *last_error(void);
void set_last_error(const char *msg);
void print_all_errors();


#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLER_H */
