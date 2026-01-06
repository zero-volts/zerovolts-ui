#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "config.h"
#include "utils/file.h"

char config_path[PATH_MAX];
static zv_config _config;

void config_set_defaults(void)
{
    memset(&_config, 0, sizeof(_config));

    snprintf(_config.version, sizeof(_config.version), "%s", "0.1");
    _config.hid.selected_file[0] = '\0';
    _config.hid.is_enabled = false;
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

static cJSON *cfg_to_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "version", _config.version);

    cJSON *hid = cJSON_AddObjectToObject(root, "hid");
    cJSON_AddStringToObject(hid, "script_selected_path", _config.hid.selected_file);
    cJSON_AddStringToObject(hid, "script_list_path", _config.hid.list_path);
    cJSON_AddBoolToObject(hid, "is_enabled", _config.hid.is_enabled);

    return root;
}

static void json_to_cfg(cJSON *root)
{
    // Partimos con defaults para compatibilidad hacia atrás
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
