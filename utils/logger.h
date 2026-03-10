#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { 
    DEBUG = 0, 
    INFO, 
    WARNING, 
    ERROR 
} LOG_LEVEL;

void init_logger(const char *file_path_name, LOG_LEVEL log_level);
void log_debug(const char *message, ...);
void log_info(const char *message, ...);
void log_warning(const char *message, ...);
void log_error(const char *message, ...);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
