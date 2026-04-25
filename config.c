#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "config.h"
#include "utils/file.h"

static char config_path[PATH_MAX];
static char project_root[PATH_MAX];
static zv_config _config;

void config_set_defaults(void)
{
    memset(&_config, 0, sizeof(_config));

    snprintf(_config.version, sizeof(_config.version), "%s", "0.1");
    _config.hid.selected_file[0] = '\0';
    _config.hid.is_enabled = false;
    _config.ir.remotes_path[0] = '\0';
    snprintf(_config.ir.backend, sizeof(_config.ir.backend), "%s", "irctl");
    snprintf(_config.ir.tx_device, sizeof(_config.ir.tx_device), "%s", "/dev/lirc0");
    snprintf(_config.ir.rx_device, sizeof(_config.ir.rx_device), "%s", "/dev/lirc1");
    _config.ir.learn_timeout_ms = 5000;
    _config.ir.use_on_screen_keyboard = true;

    snprintf(_config.display.fb_device, sizeof(_config.display.fb_device), "%s", "/dev/fb0");
}

int initialize_config(const char *path_config)
{
    if(!path_config || !path_config[0]) 
        return -1;

    size_t len = strnlen(path_config, sizeof(config_path));
    if (len >= sizeof(config_path))
        return -1;

    snprintf(config_path, sizeof(config_path), "%s", path_config);
    config_set_defaults();

    return 0;
}

static void json_get_string(cJSON *obj, const char *key, const char *fallback, char *out, size_t out_sz)
{
    if(!out || out_sz == 0) 
        return;

    out[0] = '\0';

    cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
    if(cJSON_IsString(v) && v->valuestring) {
        snprintf(out, out_sz, "%s", v->valuestring);
    } else if (fallback) {
        snprintf(out, out_sz, "%s", fallback);
    }
}

static bool json_get_bool(cJSON *obj, const char *key, bool fallback)
{
    cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
    if(cJSON_IsBool(v)) 
        return cJSON_IsTrue(v);

    return fallback;
}

static int json_get_int(cJSON *obj, const char *key, int fallback)
{
    cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(v))
        return v->valueint;
    return fallback;
}

static const char *strip_project_root(const char *path)
{
    if (!project_root[0] || !path || !path[0])
        return path;

    size_t root_len = strlen(project_root);

    // Match "project_root/" prefix
    if (strncmp(path, project_root, root_len) == 0 && path[root_len] == '/')
        return path + root_len + 1;

    return path;
}

static cJSON *cfg_to_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "version", _config.version);

    cJSON *hid = cJSON_AddObjectToObject(root, "hid");
    cJSON_AddStringToObject(hid, "script_selected_path", _config.hid.selected_file);
    cJSON_AddStringToObject(hid, "script_list_path", strip_project_root(_config.hid.list_path));
    cJSON_AddBoolToObject(hid, "is_enabled", _config.hid.is_enabled);

    cJSON *ir = cJSON_AddObjectToObject(root, "ir");
    cJSON_AddStringToObject(ir, "remotes_path", strip_project_root(_config.ir.remotes_path));
    cJSON_AddStringToObject(ir, "backend", _config.ir.backend);
    cJSON_AddStringToObject(ir, "tx_device", _config.ir.tx_device);
    cJSON_AddStringToObject(ir, "rx_device", _config.ir.rx_device);
    cJSON_AddNumberToObject(ir, "learn_timeout_ms", _config.ir.learn_timeout_ms);
    cJSON_AddBoolToObject(ir, "use_on_screen_keyboard", _config.ir.use_on_screen_keyboard);

    cJSON *display = cJSON_AddObjectToObject(root, "display");
    cJSON_AddStringToObject(display, "fb_device", _config.display.fb_device);

    return root;
}

static void json_to_cfg(cJSON *root)
{
    // Start with defaults for backward compatibility
    config_set_defaults();

    json_get_string(root, "version", _config.version, _config.version, sizeof(_config.version));

    cJSON *hid = cJSON_GetObjectItemCaseSensitive(root, "hid");
    if(cJSON_IsObject(hid)) 
    {
        json_get_string(hid, "script_selected_path", _config.hid.selected_file, _config.hid.selected_file, 
            sizeof(_config.hid.selected_file));

        json_get_string(hid, "script_list_path", _config.hid.list_path, _config.hid.list_path, 
            sizeof(_config.hid.list_path));

        _config.hid.is_enabled = json_get_bool(hid, "is_enabled", _config.hid.is_enabled);
    }

    cJSON *ir = cJSON_GetObjectItemCaseSensitive(root, "ir");
    if(cJSON_IsObject(ir)) 
    {
        json_get_string(ir, "remotes_path", _config.ir.remotes_path, _config.ir.remotes_path,
            sizeof(_config.ir.remotes_path));
        json_get_string(ir, "backend", _config.ir.backend, _config.ir.backend,
            sizeof(_config.ir.backend));
        json_get_string(ir, "tx_device", _config.ir.tx_device, _config.ir.tx_device,
            sizeof(_config.ir.tx_device));
        json_get_string(ir, "rx_device", _config.ir.rx_device, _config.ir.rx_device,
            sizeof(_config.ir.rx_device));
        _config.ir.learn_timeout_ms = json_get_int(ir, "learn_timeout_ms", _config.ir.learn_timeout_ms);
        _config.ir.use_on_screen_keyboard =
            json_get_bool(ir, "use_on_screen_keyboard", _config.ir.use_on_screen_keyboard);
    }

    cJSON *display = cJSON_GetObjectItemCaseSensitive(root, "display");
    if (cJSON_IsObject(display))
    {
        json_get_string(display, "fb_device", _config.display.fb_device,
            _config.display.fb_device, sizeof(_config.display.fb_device));
    }
}

int config_load()
{
    if(!config_path[0]) 
        return -1;

    if(!file_exists(config_path)) 
    {
        config_set_defaults();
        return 0;
    }

    cJSON *root = read_json_file(config_path);
    if(!root) 
    {    
        config_set_defaults();
        return -3;
    }

    json_to_cfg(root);
    cJSON_Delete(root);

    return 0;
}

int config_save()
{
    if(!config_path[0]) 
        return -1;

    cJSON *root = cfg_to_json();
    if(!root) 
        return -2;

    char *printed = cJSON_Print(root); // pretty
    cJSON_Delete(root);

    if(!printed) 
        return -3;

    int rc = write_entire_file(config_path, printed, strlen(printed));
    cJSON_free(printed);

    return rc;
}

const zv_config *config_get()
{
    return &_config;
}

void config_set_hid_selected_script(const char *path)
{
    snprintf(_config.hid.selected_file, sizeof(_config.hid.selected_file), "%s", path);
    config_save();
}

void config_set_hid_enabled(bool enabled)
{
    _config.hid.is_enabled = enabled;
    config_save();
}

void config_resolve_paths(const char *root)
{
    if (!root || !root[0])
        return;

    snprintf(project_root, sizeof(project_root), "%s", root);

    // If path starts with '/', it's already absolute — leave it alone.
    // Otherwise, prepend project_root to make it absolute.
    if (_config.ir.remotes_path[0] && _config.ir.remotes_path[0] != '/') {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "%s/%s", root, _config.ir.remotes_path);
        snprintf(_config.ir.remotes_path, sizeof(_config.ir.remotes_path), "%s", tmp);
    }

    if (_config.hid.list_path[0] && _config.hid.list_path[0] != '/') {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "%s/%s", root, _config.hid.list_path);
        snprintf(_config.hid.list_path, sizeof(_config.hid.list_path), "%s", tmp);
    }
}
