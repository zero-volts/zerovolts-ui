#include "page/hid/hid_controller.h"
#include "utils/logger.h"
#include "utils/error_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZV_HID_DEFAULT_SCRIPTS_DIR "/opt/zerovolts/hid-scripts"
#define ZV_HID_DEFAULT_SELECTED_PATH "/var/lib/zerovolts/hid-selected.txt"
#define HID_DEFAULT_LIST_CAPACITY 8

typedef struct {
    hid_script_list *list;
    size_t capacity;
    int has_error;
} scripts_handler_ctx;

const char *hid_controller_last_error(void)
{
    return last_error();
}

hid_status_t hid_controller_init(hid_controller *controller, const zv_config *cfg)
{
    if (!controller)
        return HID_ERR_INVALID;

    memset(controller, 0, sizeof(*controller));

    const char *scripts_dir =
        (cfg && cfg->hid.list_path[0]) ? cfg->hid.list_path : ZV_HID_DEFAULT_SCRIPTS_DIR;

    const char *selected_path =
        (cfg && cfg->hid.selected_file[0]) ? cfg->hid.selected_file : ZV_HID_DEFAULT_SELECTED_PATH;

    if (snprintf(controller->scripts_dir, sizeof(controller->scripts_dir), "%s", scripts_dir) < 0 ||
        snprintf(controller->selected_path, sizeof(controller->selected_path), "%s", selected_path) < 0 ||
        (cfg && snprintf(controller->selected_script, sizeof(controller->selected_script), "%s",
            cfg->hid.selected_file) < 0))
        return HID_ERR_INVALID;

    controller->hid_enabled = cfg ? cfg->hid.is_enabled : false;
    set_last_error(NULL);

    return hid_service_init(cfg);
}

hid_status_t hid_controller_set_selected_script(hid_controller *controller, const char *script_path)
{
    if (!controller || !script_path || !script_path[0]) {
        set_last_error("Invalid script path");
        return HID_ERR_INVALID;
    }

    int n = snprintf(controller->selected_script, sizeof(controller->selected_script), "%s", script_path);
    if (n < 0 || (size_t)n >= sizeof(controller->selected_script)) {
        set_last_error("Script path is too long");
        return HID_ERR_INVALID;
    }

    set_last_error(NULL);
    return HID_OK;
}

hid_status_t hid_controller_toggle(hid_controller *controller, bool enable)
{
    if (!controller) {
        set_last_error("Controller is required");
        return HID_ERR_INVALID;
    }

    if (!enable) 
    {
        hid_status_t stop_result = hid_service_stop_session();

        char stop_error[256];
        snprintf(stop_error, sizeof(stop_error), "%s", last_error());

        hid_status_t disable_result = hid_service_disable();
        if (stop_result != HID_OK || disable_result != HID_OK)
        {
            if (stop_result != HID_OK)
                set_last_error(stop_error);

            return stop_result != HID_OK ? stop_result : disable_result;
        }

        if (config_set_hid_state("", false) != 0)
        {
            set_last_error("HID stopped, but its configuration could not be saved");
            return HID_ERR_IO;
        }

        controller->hid_enabled = false;
        controller->selected_script[0] = '\0';
        set_last_error(NULL);

        return HID_OK;
    }

    if (!controller->selected_script[0])
    {
        set_last_error("Select a script first.");
        return HID_ERR_INVALID;
    }

    if (controller->hid_enabled)
    {
        set_last_error("Disable the current HID session before starting another");
        return HID_ERR_INVALID;
    }

    if (!file_has_extension(controller->selected_script, ".py") || !file_exists(controller->selected_script))
    {
        set_last_error("Select an existing Python .py HID script");
        return HID_ERR_INVALID;
    }

    // The watcher reads both fields when it starts; persist them together first.
    hid_config_t previous = config_get()->hid;
    if (config_set_hid_state(controller->selected_script, true) != 0)
    {
        set_last_error("Could not save HID selection before starting the session");
        return HID_ERR_IO;
    }

    hid_status_t rc = hid_service_enable();
    if (rc != HID_OK)
    {
        char error[256];
        snprintf(error, sizeof(error), "%s", last_error());
        if (config_set_hid_state(previous.selected_file, previous.is_enabled) != 0)
            log_error("[HID] Could not restore configuration after enable failure\n");

        set_last_error(error);
        return rc;
    }

    rc = hid_service_start_session();
    if (rc != HID_OK)
    {
        char error[256];
        snprintf(error, sizeof(error), "%s", last_error());
        if (hid_service_disable() != HID_OK)
            log_error("[HID] Could not unbind gadget after session failure\n");

        if (config_set_hid_state(previous.selected_file, previous.is_enabled) != 0)
            log_error("[HID] Could not restore configuration after session failure\n");

        set_last_error(error);

        return rc;
    }

    controller->hid_enabled = true;
    set_last_error(NULL);

    return HID_OK;
}

static void collect_scripts_cb(const file_desc *description, void *user_data)
{
    scripts_handler_ctx *ctx = (scripts_handler_ctx *)user_data;
    if (!ctx || !ctx->list || !ctx->list->scripts || !description)
        return;

    if (ctx->list->count == ctx->capacity)
    {
        size_t new_capacity = ctx->capacity * 2;
        hid_script_item *tmp = (hid_script_item *)realloc(
            ctx->list->scripts, new_capacity * sizeof(hid_script_item));
        if (!tmp)
        {
            ctx->has_error = 1;
            return;
        }

        ctx->list->scripts = tmp;
        ctx->capacity = new_capacity;
    }

    size_t idx = ctx->list->count;
    hid_script_item *item = &ctx->list->scripts[idx];
    memset(item, 0, sizeof(*item));

    snprintf(item->name, sizeof(item->name), "%s", description->file_name);
    snprintf(item->path, sizeof(item->path), "%s", description->file_path);
    ctx->list->count++;
}

hid_status_t hid_controller_list_scripts(hid_controller *controller, hid_script_list *out_list)
{
    if (!controller || !out_list)
    {
        set_last_error("Invalid arguments");
        return HID_ERR_INVALID;
    }

    memset(out_list, 0, sizeof(*out_list));
    out_list->scripts = (hid_script_item *)calloc(HID_DEFAULT_LIST_CAPACITY, sizeof(hid_script_item));
    if (!out_list->scripts)
    {
        log_error("[HID::controller]::hid_controller_list_scripts Not enough memory\n");
        set_last_error("Not enough memory");

        return HID_ERR_IO;
    }

    scripts_handler_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.list = out_list;
    ctx.capacity = HID_DEFAULT_LIST_CAPACITY;

    hid_status_t rc = hid_service_list_scripts_cb(controller->scripts_dir, collect_scripts_cb, &ctx);
    if (rc != HID_OK || ctx.has_error)
    {
        hid_controller_free_script_list(out_list);
        set_last_error(rc != HID_OK ? last_error() : "Failed to build script list");
        return rc != HID_OK ? rc : HID_ERR_IO;
    }

    set_last_error(NULL);
    return HID_OK;
}

void hid_controller_free_script_list(hid_script_list *list)
{
    if (!list)
        return;

    free(list->scripts);
    list->scripts = NULL;
    list->count = 0;
}

const char *hid_controller_selected_script(const hid_controller *controller)
{
    if (!controller)
        return "";

    return controller->selected_script;
}
