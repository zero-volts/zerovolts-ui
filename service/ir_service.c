#include "service/ir_service.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include "utils/error_handler.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define IR_DEFAULT_TX_DEV "/dev/lirc0"
#define IR_DEFAULT_RX_DEV "/dev/lirc1"
#define IR_DEFAULT_TIMEOUT_MS 5000

typedef enum {
    SUCCESS = 0,
    TIMEOUT = 124,
} system_status;

// The memory used by this struct is used a long the whole program.
static ir_context context; 

static void shell_escape_single_quotes(const char *src, char *dst, size_t dst_sz)
{
    size_t j = 0;

    if (!dst || dst_sz == 0)
        return;

    dst[0] = '\0';
    if (!src)
        return;

    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_sz; i++) {
        if (src[i] == '\'') {
            const char *esc = "'\\''";
            size_t need = strlen(esc);
            if (j + need >= dst_sz)
                break;
            memcpy(dst + j, esc, need);
            j += need;
        } else {
            dst[j++] = src[i];
        }
    }

    dst[j] = '\0';
}

static int system_status_code(int rc)
{
    if (rc == -1)
        return -1;

    if (WIFEXITED(rc))
        return WEXITSTATUS(rc);

    return -1;
}

static ir_status_t irctl_send_raw(const char *raw_path)
{
    if ( zv_is_empty(raw_path))
    {
        set_last_error("The raw path is empty, can't send the signal");
        return IR_ERR_INVALID;
    }

    char escaped_dev[EXCAPED_DEV_PATH];
    char escaped_path[ESCAPED_TMP_PATH];
    char cmd[COMMAND_SIZE];
    const int send_timeout_sec = 3;

    shell_escape_single_quotes(context.tx_dev, escaped_dev, sizeof(escaped_dev));
    shell_escape_single_quotes(raw_path, escaped_path, sizeof(escaped_path));

    snprintf(cmd, sizeof(cmd),
             "timeout %ds ir-ctl -d '%s' -s '%s' >/dev/null 2>&1", send_timeout_sec, escaped_dev, escaped_path);

    log_debug("[IR][service]::irctl_send_raw send cmd: %s\n", cmd);

    // TODO: dont use system to make a call to the sustem, use fork or something like that
    int exit_code = system_status_code(system(cmd));
    if (exit_code != 0)
    {
        set_last_error("ir-ctl send failed");
        log_error("[IR][service]::irctl_send_raw send failed exit_code=%d tx_dev=%s raw=%s", exit_code, context.tx_dev, raw_path);
        return IR_ERR_IO;
    }

    set_last_error(NULL);
    log_debug("[IR][service]::irctl_send_raw send ok tx_dev=%s raw=%s", context.tx_dev, raw_path);
    return IR_OK;
}

static ir_status_t irctl_learn_raw (const char *out_raw_path)
{
    if (zv_is_empty(out_raw_path))
    {
        log_error("[IR][service]::irctl_learn_raw Empty out path to learn the signal\n");
        set_last_error("Empty out path to learn the signal");
        return IR_ERR_INVALID;
    }

    char escaped_dev[EXCAPED_DEV_PATH];
    char escaped_tmp[ESCAPED_TMP_PATH];
    char tmp_path[PATH_MAX];
    struct stat st;

    if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", out_raw_path) <= 0) 
    {
        set_last_error("Invalid output path");
        return IR_ERR_INVALID;
    }

    shell_escape_single_quotes(context.rx_dev, escaped_dev, sizeof(escaped_dev));
    shell_escape_single_quotes(tmp_path, escaped_tmp, sizeof(escaped_tmp));

    char cmd[COMMAND_SIZE];
    int sec = context.timeout_ms / 1000;
    int watchdog_sec = sec + 2;

    // This will be store a raw signal file in tmp directory
    snprintf(cmd, sizeof(cmd),
             "timeout %ds ir-ctl -r -d '%s' > '%s'", watchdog_sec, escaped_dev, escaped_tmp);

    log_debug("[IR][service]::irctl_learn_raw learn signal command: %s", cmd);
    int exit_code = system_status_code(system(cmd));
    if (exit_code == TIMEOUT)
    {
        /* timeout can still leave a valid capture in the output file */
        struct stat st;
        if (stat(tmp_path, &st) == 0 && st.st_size > 0) 
        {
            if (rename(tmp_path, out_raw_path) != 0) 
            {
                remove(tmp_path);
                set_last_error("[IR][service]::irctl_learn_raw Failed to store raw file");
                log_error("[IR][service]::irctl_learn_raw learn rename after-timeout failed src=%s dst=%s errno=%d(%s)",
                         tmp_path, out_raw_path, errno, strerror(errno));
                           
                return IR_ERR_IO;
            }

            set_last_error(NULL);
            log_debug("learn captured before timeout bytes=%ld path=%s", (long)st.st_size, out_raw_path);
            return IR_OK;
        }

        remove(tmp_path);
        set_last_error("Learn timed out");
        log_debug("learn timeout after %ds", sec);

        return IR_ERR_TIMEOUT;
    }

    if (exit_code != 0) 
    {
        remove(tmp_path);
        set_last_error("ir-ctl learn failed");
        log_error("learn command failed (exit=%d)", exit_code);
        return IR_ERR_IO;
    }

    if (stat(tmp_path, &st) != 0 || st.st_size <= 0) {
        remove(tmp_path);
        set_last_error("Empty signal captured");
        return IR_ERR_IO;
    }

    if (rename(tmp_path, out_raw_path) != 0) 
    {
        remove(tmp_path);
        set_last_error("Failed to store raw file");
        log_error("learn rename failed src=%s dst=%s errno=%d(%s)",
                 tmp_path, out_raw_path, errno, strerror(errno));
        return IR_ERR_IO;
    }

    set_last_error(NULL);
    log_debug("learn stored bytes=%ld path=%s", (long)st.st_size, out_raw_path);
    return IR_OK;
}

ir_status_t ir_learn_raw (const char *out_raw_path) 
{
    if (context.backend == NULL)
    {
        log_error("[IR][service]::ir_learn_raw backend is null\n");
        set_last_error("[ir_learn_raw]:: backend is null");
        return IR_ERR_INVALID;
    }

    if ( strcmp(context.backend, BACKEND_TYPE_IRCTL) == 0) {
       return irctl_learn_raw(out_raw_path);
    }
    else if (strcmp(context.backend, BACKEND_TYPE_LIRC) == 0) 
    {
        log_error("[IR][service]::ir_learn_raw lircdev backend not implemented yet\n");
        set_last_error("lircdev backend not implemented yet");
        return IR_ERR_UNSUPPORTED;
    } 
    else 
    {
        log_warning("[IR][service]::ir_learn_raw Unknown IR backend\n");
        set_last_error("Unknown IR backend");
        return IR_ERR_CONFIG;
    }
}

ir_status_t ir_send_raw(const char *raw_path)
{
    if (context.backend == NULL)
    {
        log_error("[IR][service]::ir_send_raw backend is null\n");
        set_last_error("[ir_send_raw]:: backend is null");
        return IR_ERR_INVALID;
    }

    if ( strcmp(context.backend, BACKEND_TYPE_IRCTL) == 0) {
        return irctl_send_raw(raw_path);
    }
    else if (strcmp(context.backend, BACKEND_TYPE_LIRC) == 0) {
        log_error("[IR][service::ir_send_raw lircdev backend not implemented yet\n");
        set_last_error("lircdev backend not implemented yet");
        return IR_ERR_UNSUPPORTED;
    } 
    else 
    {
        log_warning("[IR][service]::ir_send_raw Unknown IR backend\n");
        set_last_error("Unknown IR backend");
        return IR_ERR_CONFIG;
    }
}

ir_status_t ir_service_init(const ir_context *ctx)
{
    if (!ctx) 
    {
        log_error("[IR][service]::ir_service_init Missing IR config...\n");
        set_last_error("Missing IR config");
        return IR_ERR_CONFIG;
    }

    memset(&context, 0, sizeof(ir_context));
    set_last_error(NULL);

    snprintf(context.tx_dev, sizeof(context.tx_dev), "%s",
             (ctx->tx_dev && ctx->tx_dev[0]) ? ctx->tx_dev : IR_DEFAULT_TX_DEV);

    snprintf(context.rx_dev, sizeof(context.rx_dev), "%s",
             (ctx->rx_dev && ctx->rx_dev[0]) ? ctx->rx_dev : IR_DEFAULT_RX_DEV);

    context.timeout_ms = ctx->timeout_ms > 0 ? ctx->timeout_ms : IR_DEFAULT_TIMEOUT_MS;
    context.backend = (ctx->backend && ctx->backend[0]) ? ctx->backend : BACKEND_TYPE_IRCTL;

    log_debug("[IR][service]::ir_service_init backend=%s tx=%s, rx=%s, timeout_ms=%d",
             context.backend ? context.backend : BACKEND_TYPE_IRCTL,
             context.tx_dev, context.rx_dev, context.timeout_ms);

    return IR_OK;
}

static void collect_raw_item(const file_desc *desc, void *obj_target)
{
    ir_callback_event *event = (ir_callback_event *)obj_target;

    if (!event || !desc || !event->cb)
        return;

    if (!desc->is_file)
        return;

    if (!file_has_extension(desc->file_name, ".raw"))
        return;

    event->cb(desc, event->data);
}

ir_status_t ir_list_raw_files_cb(const char *dir, ir_callback_event *event)
{
    if (!event) 
    {
        log_error("[IR][service]::ir_list_raw_files_cb Callback is required\n");
        set_last_error("Callback is required");
        return IR_ERR_INVALID;
    }

    if (!dir || !dir[0]) 
    {
        log_error("[IR][service]::ir_list_raw_files_cb Directory is required\n");
        set_last_error("Directory is required");
        return IR_ERR_INVALID;
    }

    if (!file_is_directory(dir)) 
    {
        log_error("[IR][service]::ir_list_raw_files_cb Directory not found\n");
        set_last_error("Directory not found");
        return IR_ERR_IO;
    }

    get_file_list(dir, collect_raw_item, event);

    set_last_error(NULL);
    return IR_OK;
}
