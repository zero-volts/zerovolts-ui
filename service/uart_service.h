#ifndef UART_SERVICE_H
#define UART_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
    UART_OK = 0,
    UART_ERR_CONFIG = -1,
    UART_ERR_IO = -2,
    UART_ERR_TIMEOUT = -3,
    UART_ERR_INVALID = -4
} uart_status_t;

typedef void (*uart_event_cb)(const char *tag_id, char *buffer);

uart_status_t uart_service_init(const char *device, int baudrate);
uart_status_t uart_send_line(const char *cmd);
uart_status_t uart_poll_line(char *buffer, size_t buffer_size);
void add_event_callback(uart_event_cb new_cb, const char *tag_id);
void remove_event_callback(const char *tag_id);
void uart_process_loop();
void uart_service_close(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_SERVICE_H */
