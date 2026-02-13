#include "hid_service.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char g_last_error[256];

static void set_last_error(const char *msg)
{
    if (!msg) {
        g_last_error[0] = '\0';
        return;
    }

    snprintf(g_last_error, sizeof(g_last_error), "%s", msg);
}

const char *hid_service_last_error(void)
{
    return g_last_error;
}

static hid_status_t run_command(const char *cmd, const char *error_msg)
{
    if (!cmd || !cmd[0]) {
        set_last_error("Invalid command");
        return HID_ERR_INVALID;
    }

    int rc = system(cmd);
    if (rc != 0) {
        set_last_error(error_msg);
        return HID_ERR_IO;
    }

    set_last_error(NULL);
    return HID_OK;
}

hid_status_t hid_service_enable(void)
{
    return run_command("sudo /home/zerovolts/git/zerovolts-ui/scripts/zv-hid-enable.sh",
        "Failed to enable HID");
}

hid_status_t hid_service_disable(void)
{
    return run_command("sudo /home/zerovolts/git/zerovolts-ui/scripts/zv-hid-disable.sh",
        "Failed to disable HID");
}

hid_status_t hid_service_start_session(void)
{
    return run_command("sudo systemctl start zv-hid-session.service",
        "Failed to start HID session");
}

hid_status_t hid_service_stop_session(void)
{
    return run_command("sudo systemctl stop zv-hid-session.service",
        "Failed to stop HID session");
}

typedef struct {
    file_callback callback;
    void *user_data;
} list_scripts_ctx;

static void list_scripts_adapter(const file_desc *description, void *user_data)
{
    list_scripts_ctx *ctx = (list_scripts_ctx *)user_data;
    if (!ctx || !ctx->callback || !description)
        return;

    if (!description->is_file)
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
