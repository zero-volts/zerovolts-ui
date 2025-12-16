#pragma once
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *scripts_dir;     // ej: "/opt/zerovolts/hid-scripts"
    const char *selected_path;   // ej: "/var/lib/zerovolts/hid-selected.txt"
} zv_hid_cfg_t;

/**
 * Crea la página HID y devuelve el lv_obj_t* de la page.
 * Se integra con lv_menu (LVGL v9).
 */
lv_obj_t *zv_hid_page_create(lv_obj_t *menu, const zv_hid_cfg_t *cfg);

/**
 * Opcional: refresca lista desde fuera (si en el futuro quieres).
 */
void zv_hid_refresh_scripts(void);

/**
 * Opcional: obtener el script actualmente seleccionado (path completo).
 * Devuelve puntero interno (no free).
 */
const char *zv_hid_get_selected_script(void);

#ifdef __cplusplus
}
#endif