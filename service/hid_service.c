#include "hid_service.h"

#include "utils/logger.h"
#include "utils/string_utils.h"
#include "utils/error_handler.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

typedef struct {
    char config_scripts[MAX_PATH_SIZE];
} hid_service_ctx;

typedef struct {
    file_callback callback;
    void *user_data;
} list_scripts_ctx;

static hid_service_ctx service_ctx;

static hid_status_t run_command(char *const argv[], const char *error_msg)
{
    pid_t child;
    int result = posix_spawn(&child, argv[0], NULL, NULL, argv, environ);
    int status = 0;
    if (result == 0)
    {
        pid_t waited;
        do {
            waited = waitpid(child, &status, 0);
        } while (waited < 0 && errno == EINTR);

        if (waited < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
            result = -1;
    }

    if (result != 0)
    {
        set_last_error(error_msg);
        return HID_ERR_IO;
    }

    set_last_error(NULL);
    return HID_OK;
}

hid_status_t hid_service_init(const zv_config *config)
{
    if (config == NULL || zv_is_empty(config->hid.config_scripts))
    {
        set_last_error("Configure hid.config_scripts before enabling HID");
        return HID_ERR_CONFIG;
    }

    int n = snprintf(service_ctx.config_scripts, sizeof(service_ctx.config_scripts), "%s", config->hid.config_scripts);
    if (n < 0 || (size_t)n >= sizeof(service_ctx.config_scripts))
    {
        set_last_error("HID scripts path is too long");
        return HID_ERR_INVALID;
    }

    return HID_OK;
}

hid_status_t hid_service_enable(void)
{
    if (zv_is_empty(service_ctx.config_scripts))
    {
        return HID_ERR_INVALID;
    }

    char command [MAX_PATH_SIZE];
    int wr = snprintf(command, sizeof(command), "%s/zv-hid-gadget-bind.sh", service_ctx.config_scripts);
    if (wr < 0 || (size_t)wr >= sizeof (command))
    {
        return HID_ERR_INVALID;
    }

    char *args[] = {"/usr/bin/sudo", command, NULL};
    return run_command(args, "Failed to enable HID; check gadget service and sudo permissions");
}

hid_status_t hid_service_disable(void)
{
     if (zv_is_empty(service_ctx.config_scripts))
    {
        return HID_ERR_INVALID;
    }

    char command [MAX_PATH_SIZE];
    int wr = snprintf(command, sizeof(command), "%s/zv-hid-gadget-unbind.sh", service_ctx.config_scripts);
    if (wr < 0 || (size_t)wr >= sizeof (command))
    {
        return HID_ERR_INVALID;
    }

    char *args[] = {"/usr/bin/sudo", command, NULL};
    return run_command(args, "Failed to disable HID");
}

hid_status_t hid_service_start_session(void)
{
    char *args[] = {"/usr/bin/sudo", "/usr/bin/systemctl", "start", "zv-hid-session.service", NULL};
    return run_command(args, "Failed to start HID session");
}

hid_status_t hid_service_stop_session(void)
{
    char *args[] = {"/usr/bin/sudo", "/usr/bin/systemctl", "stop", "zv-hid-session.service", NULL};
    return run_command(args, "Failed to stop HID session");
}

static void list_scripts_adapter(const file_desc *description, void *user_data)
{
    list_scripts_ctx *ctx = (list_scripts_ctx *)user_data;
    if (!ctx || !ctx->callback || !description)
        return;

    if (!description->is_file || !file_has_extension(description->file_name, ".py"))
        return;

    ctx->callback(description, ctx->user_data);
}

hid_status_t hid_service_list_scripts_cb(const char *scripts_dir, file_callback callback, void *user_data)
{
    if (!scripts_dir || !scripts_dir[0])
    {
        set_last_error("Scripts directory is required");
        return HID_ERR_INVALID;
    }

    if (!file_is_directory(scripts_dir))
    {
        set_last_error("Scripts directory not found");
        return HID_ERR_IO;
    }

    if (!callback)
    {
        set_last_error("Callback is required");
        return HID_ERR_INVALID;
    }

    list_scripts_ctx ctx;
    ctx.callback = callback;
    ctx.user_data = user_data;
    get_file_list(scripts_dir, list_scripts_adapter, &ctx);

    set_last_error(NULL);

    return HID_OK;
}
