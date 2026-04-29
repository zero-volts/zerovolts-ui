#include "uart_service.h"

#include "utils/error_handler.h"
#include "utils/logger.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdarg.h>

#define MAX_HANDLERS 5
#define TAG_MAX_LEN 32

// File descriptor that represent the open connection throught UART
static int uart_fd = -1;

static char uart_rx_accum[512];
static size_t uart_rx_len = 0;

// Internal container to keep track of any "service/object" that want to be notify
// by this service.
typedef struct {
    char tag[TAG_MAX_LEN];
    uart_event_cb callback;
} event_t;

static event_t events[MAX_HANDLERS];
static int events_count = 0;

static speed_t uart_get_speed(int baudrate)
{
    // Parsing baudrate numbers to be used in termios.
    switch (baudrate)
    {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return 0;
    }
}

static uart_status_t uart_write_all(int fd, const char *buffer, size_t len)
{
    size_t total = 0;
    while (total < len)
    {
        ssize_t written = write(fd, buffer + total, len - total);
        if (written < 0)
        {
            set_last_error("UART write failed");
            log_error("[UART][service] write failed errno=%d (%s)", errno, strerror(errno));
            return UART_ERR_IO;
        }

        total += (size_t)written;
    }

    return UART_OK;
}

uart_status_t uart_service_init(const char *device, int baudrate)
{
    if (!device || device[0] == '\0')
    {
        set_last_error("UART device is empty");
        return UART_ERR_INVALID;
    }

    if (uart_fd >= 0)
    {
        close(uart_fd);
        uart_fd = -1;
    }

    /*
     * O_RDWR: Open the device to write and read
     * O_NOCTTY: Avoid that the port converts into terminal. wwe dont want an interactiver terminal 
     *          we want a UART communiction link
     * O_NONBLOCK: Non blocking port 
     */
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (uart_fd == -1)
    {
        set_last_error("Failed to open UART device");
        log_error("[UART][service] open failed dev=%s errno=%d (%s)", device, errno, strerror(errno));
        return UART_ERR_IO;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(uart_fd, &tty) != 0)
    {
        set_last_error("Failed to read current UART config");
        log_error("[UART][service] tcgetattr failed dev=%s errno=%d (%s)", device, errno, strerror(errno));
        close(uart_fd);
        uart_fd = -1;
        return UART_ERR_CONFIG;
    }

    speed_t speed = uart_get_speed(baudrate);
    if (speed == 0)
    {
        set_last_error("Unsupported UART baudrate");
        close(uart_fd);
        uart_fd = -1;
        return UART_ERR_INVALID;
    }

    cfsetispeed(&tty, speed);  // in speed
    cfsetospeed(&tty, speed);  // out speed

    /*
     * c_cflag: control flags
     * Defines the physical format of UART frames.
     */
    tty.c_cflag &= ~PARENB;   /* PARENB: disables parity. This is the N in "8N1". */
    tty.c_cflag &= ~CSTOPB;   /* CSTOPB: uses 1 stop bit instead of 2. This is the 1 in "8N1". */
    tty.c_cflag &= ~CSIZE;    /* CSIZE: clears the current word-size mask. */
    tty.c_cflag |= CS8;       /* CS8: selects 8 data bits. This is the 8 in "8N1". */
    tty.c_cflag &= ~CRTSCTS;  /* CRTSCTS: disables RTS/CTS hardware flow control. */
    tty.c_cflag |= CREAD;     /* CREAD: enables the receiver. Without this, no data is read. */
    tty.c_cflag |= CLOCAL;    /* CLOCAL: ignores modem-style control signals. */

    /*
     * c_iflag: input flags
     * Controls how incoming bytes are interpreted.
     * For microcontroller communication, disable automatic
     * transformations and work in raw mode.
     */
    tty.c_iflag &= ~IXON;     /* IXON: disables received XON/XOFF software flow control. */
    tty.c_iflag &= ~IXOFF;    /* IXOFF: disables transmitted XON/XOFF software flow control. */
    tty.c_iflag &= ~IXANY;    /* IXANY: prevents any byte from resuming an XON/XOFF pause. */
    tty.c_iflag &= ~ICRNL;    /* ICRNL: does not convert received '\r' into '\n'. */
    tty.c_iflag &= ~INLCR;    /* INLCR: does not convert received '\n' into '\r'. */
    tty.c_iflag &= ~IGNCR;    /* IGNCR: does not ignore '\r'; receive it and decide in code. */

    /*
     * c_lflag: local flags
     * These are typical flags for human terminals. For UART with an ESP32,
     * use byte-by-byte reads instead of terminal behavior.
     */
    tty.c_lflag &= ~ICANON;   /* ICANON: disables canonical mode; read() does not wait for a full line. */
    tty.c_lflag &= ~ECHO;     /* ECHO: does not echo incoming data to the screen. */
    tty.c_lflag &= ~ECHOE;    /* ECHOE: prevents visual erase behavior. */
    tty.c_lflag &= ~ISIG;     /* ISIG: prevents Ctrl-C, Ctrl-Z, etc. from being interpreted as signals. */

    /*
     * c_oflag: output flags
     * Controls post-processing of outgoing data.
     * Disable it so write() sends exactly what was requested.
     */
    tty.c_oflag &= ~OPOST;    /* OPOST: disables automatic output processing. */

    /*
     * c_cc: special control characters
     *
     * VMIN = 0:
     * read() can return immediately if no data has arrived.
     *
     * VTIME = 0:
     * do not wait in termios. The UI strategy is:
     * "check whether data exists" and consume only what is available.
     *
     * This prevents the LVGL thread from freezing while waiting for UART.
     */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    /*
     * Clears pending input and output buffers before starting.
     * This avoids processing stale data if the port already had data.
     */
    if (tcflush(uart_fd, TCIOFLUSH) != 0)
    {
        set_last_error("Failed to flush UART buffers");
        log_error("[UART][service] tcflush failed dev=%s errno=%d (%s)", device, errno, strerror(errno));
        close(uart_fd);
        uart_fd = -1;
        return UART_ERR_IO;
    }

    /*
     * TCSANOW:
     * applies the configuration immediately.
     */
    if (tcsetattr(uart_fd, TCSANOW, &tty) != 0)
    {
        set_last_error("Failed to apply UART config");
        log_error("[UART][service] tcsetattr failed dev=%s errno=%d (%s)", device, errno, strerror(errno));
        close(uart_fd);
        uart_fd = -1;
        return UART_ERR_CONFIG;
    }

    set_last_error(NULL);
    log_info("[UART][service] uart ready dev=%s baud=%d", device, baudrate);

    return UART_OK;
}

uart_status_t uart_send_formatted_line(const char *message, ...)
{
    if (!message)
        return UART_ERR_INVALID;

    va_list args;
    va_start(args, message);

    char msg[1024];
    vsnprintf(msg, sizeof(msg), message, args);

    va_end(args);

    return uart_send_line(msg);
}

uart_status_t uart_send_line(const char *cmd)
{
    if (uart_fd < 0)
    {
        set_last_error("UART is not initialized");
        return UART_ERR_CONFIG;
    }

    if (!cmd || cmd[0] == '\0')
    {
        set_last_error("UART command is empty");
        return UART_ERR_INVALID;
    }

    /*
     * The ESP32 firmware builds commands until it finds '\n'.
     * That is why a newline is appended at the end here.
     *
     * Example:
     *   "PING"  -> sent as "PING\n"
     */
    char frame[256];
    int frame_len = snprintf(frame, sizeof(frame), "%s\n", cmd);
    if (frame_len <= 0 || (size_t)frame_len >= sizeof(frame))
    {
        set_last_error("UART command is too long");
        return UART_ERR_INVALID;
    }

    uart_status_t rc = uart_write_all(uart_fd, frame, (size_t)frame_len);
    if (rc != UART_OK)
        return rc;

    /*
     * tcdrain() waits until the system finishes physically transmitting
     * the pending bytes from the output buffer.
     */
    if (tcdrain(uart_fd) != 0)
    {
        set_last_error("UART tcdrain failed");
        log_error("[UART][service] tcdrain failed errno=%d (%s)", errno, strerror(errno));
        return UART_ERR_IO;
    }

    set_last_error(NULL);
    log_debug("[UART][service] sent cmd=%s", cmd);

    return UART_OK;
}

uart_status_t uart_poll_line(char *buffer, size_t buffer_size)
{
    if (uart_fd < 0)
    {
        set_last_error("UART is not initialized");
        return UART_ERR_CONFIG;
    }

    if (!buffer || buffer_size == 0)
    {
        set_last_error("UART poll buffer is invalid");
        return UART_ERR_INVALID;
    }

    while (1)
    {
        char ch = '\0';
        ssize_t n = read(uart_fd, &ch, 1);

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                return UART_ERR_TIMEOUT;

            set_last_error("UART poll read failed");
            log_error("[UART][service] poll read failed errno=%d (%s)", errno, strerror(errno));
            return UART_ERR_IO;
        }

        if (n == 0)
            return UART_ERR_TIMEOUT;

        if (ch == '\r')
            continue;

        if (ch == '\n')
        {
            size_t copy_len = uart_rx_len;
            if (copy_len >= buffer_size)
                copy_len = buffer_size - 1;

            memcpy(buffer, uart_rx_accum, copy_len);
            buffer[copy_len] = '\0';
            uart_rx_len = 0;

            if (copy_len == 0)
                continue;

            set_last_error(NULL);
            return UART_OK;
        }

        if (uart_rx_len < sizeof(uart_rx_accum) - 1)
        {
            uart_rx_accum[uart_rx_len++] = ch;
            uart_rx_accum[uart_rx_len] = '\0';
        }
        else
        {
            /*
             * The line exceeds the accumulation buffer.
             * Consume the remaining bytes up to '\n' so the
             * next call to uart_poll_line starts clean,
             * without reading stale data from the middle of this corrupt line.
             */
            uart_rx_len = 0;
            while (1)
            {
                char discard;
                ssize_t d = read(uart_fd, &discard, 1);
                if (d <= 0 || discard == '\n')
                    break;
            }
            set_last_error("UART line too long, discarded");
            return UART_ERR_IO;
        }
    }
}

void uart_process_loop()
{
    char line[512];
    if (uart_poll_line(line, sizeof(line)) == UART_OK) 
    {
        for (int index = 0; index < events_count; index++)
        {
            event_t event = events[index];
            if (event.callback != NULL)
                event.callback(event.tag, line);
        }
    }
}

void uart_service_close(void)
{
    if (uart_fd >= 0)
    {
        close(uart_fd);
        uart_fd = -1;
    }

    uart_rx_len = 0;

    log_info("[UART][service] uart closed");
}

static event_t *find_handler(const char *tag)
{
    for (int index = 0; index < events_count; index++)
    {
        const char *event_tag = events[index].tag;
        if (strcmp(event_tag, tag) == 0)
            return &events[index];
    }

    return NULL;
}

void add_event_callback(uart_event_cb new_cb, const char *tag_id)
{
    event_t *event = find_handler(tag_id);
    if (event != NULL)
        return;

    if (events_count >= MAX_HANDLERS)
    {
        log_warning("Can't save more events, is in the limit of %d", MAX_HANDLERS);
        return;
    }

    event = &events[events_count++];
    event->callback = new_cb;

    strncpy(event->tag, tag_id, TAG_MAX_LEN - 1);
    event->tag[TAG_MAX_LEN - 1] = '\0';
}