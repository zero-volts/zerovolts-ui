#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char device[20];
    int baudrate;
} uart_config_t;

typedef struct {
    char version[16];
    struct {
        bool is_enabled;
        char list_path[512];
        char selected_file[512];
    } hid;

    struct {
        char remotes_path[512];
        char backend[32];
        char tx_device[128];
        char rx_device[128];
        int learn_timeout_ms;
        bool use_on_screen_keyboard;
    } ir;

    struct {
        char fb_device[128];
    } display;

    uart_config_t uart;
} zv_config;

int initialize_config(const char *path_config);
int config_load();
int config_save();
void config_resolve_paths(const char *project_root);
int config_get_asset_path(const char *asset_path, char *out, size_t out_sz);

const zv_config *config_get();

void config_set_hid_selected_script(const char *path);
void config_set_hid_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
