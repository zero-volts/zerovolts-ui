#include "logger.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

static FILE *log_file = NULL;
static LOG_LEVEL current_log_level = INFO;
static const char *level_strings[] = { "DEBUG", "INFO", "WARNING", "ERROR" };

/* ANSI colors (console only) */
#define ANSI_RESET   "\x1b[0m"
#define ANSI_DIM     "\x1b[2m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_CYAN    "\x1b[36m"

static const char *level_color(LOG_LEVEL level)
{
    switch (level) {
        case DEBUG:   return ANSI_CYAN;
        case INFO:    return ANSI_GREEN;
        case WARNING: return ANSI_YELLOW;
        case ERROR:   return ANSI_RED;
        default:      return ANSI_RESET;
    }
}

static FILE *console_stream(LOG_LEVEL level)
{
    /* common convention: warnings/errors to stderr */
    return (level >= WARNING) ? stderr : stdout;
}

void init_logger(const char *file_path_name, LOG_LEVEL log_level)
{
    current_log_level = log_level;

    if (!file_path_name) {
        fprintf(stderr, "No file path to create the log file\n");
        return;
    }

    log_file = fopen(file_path_name, "a");
    if (!log_file) {
        fprintf(stderr, "Can't create the file log!\n");
        log_file = NULL;
    }
}

void close_logger(void)
{
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

static void log_message_v(LOG_LEVEL level, const char *fmt, va_list args)
{
    if (!fmt)
        return;

    if (level < current_log_level)
        return;

    /* timestamp */
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now);

    /* build message */
    char msg[1024];
    vsnprintf(msg, sizeof(msg), fmt, args);

    /* final line (no color) */
    char line[1200];
    snprintf(line, sizeof(line), "[%s]::[%s] - %s\n", ts, level_strings[level], msg);

    /* 1) Always write to file if available */
    if (log_file) {
        fputs(line, log_file);
        fflush(log_file);
    }

    /* 2) Always write to console (with ANSI) */
    FILE *out = console_stream(level);
    const char *color = level_color(level);

    fprintf(out, "%s[%s]::[%s]%s - %s\n",
            color,
            ts,
            level_strings[level],
            ANSI_RESET,
            msg);

    fflush(out);
}

void log_debug(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    log_message_v(DEBUG, message, args);
    va_end(args);
}

void log_info(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    log_message_v(INFO, message, args);
    va_end(args);
}

void log_warning(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    log_message_v(WARNING, message, args);
    va_end(args);
}

void log_error(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    log_message_v(ERROR, message, args);
    va_end(args);
}