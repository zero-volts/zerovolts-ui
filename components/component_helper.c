#include "component_helper.h"
#include "components/ui_theme.h"

#define ICON_IDENTIFYER 100

lv_obj_t *create_square_main_button(lv_obj_t *parent, const char *text, const char *icon, lv_event_cb_t cb, void *user_data)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_PCT(48), 100);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(btn, 10, 0);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    // estados
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_BUTTON_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, ZV_COLOR_TERMINAL, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, ZV_COLOR_TERMINAL, LV_STATE_CHECKED);

    // layout interno
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn, 6, 0);

    lv_obj_t *ic = lv_label_create(btn);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_user_data(ic, (void *)ICON_IDENTIFYER); // Usamos 100 como "TAG_ICONO"

    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, text);
    lv_obj_set_style_text_color(lb, ZV_COLOR_TEXT_MAIN, 0);

    return btn;
}

void rotate_icon_by_tag(lv_obj_t * btn, int32_t angle)
{
    uint32_t i;
    for(i = 0; i < lv_obj_get_child_cnt(btn); i++) 
    {
        lv_obj_t * child = lv_obj_get_child(btn, i);
        // Buscamos el objeto que tenga nuestro tag 100
        if(lv_obj_get_user_data(child) == (void *)ICON_IDENTIFYER) 
        {
            // 1. Forzar actualización para tener dimensiones correctas
            lv_obj_update_layout(child);
            
            // 2. Establecer pivote en el centro exacto del icono
            int width = lv_obj_get_width(child);
            int height = lv_obj_get_height(child);
            lv_obj_set_style_transform_pivot_x(child, width / 2, 0);
            lv_obj_set_style_transform_pivot_y(child, height / 2, 0);

            // 3. Aplicar rotación (angle * 10 para décimas de grado)
            lv_obj_set_style_transform_rotation(child, angle * 10, 0);

            // 4. EVITAR EL DESPLAZAMIENTO: 
            // Como la rotación es visual, a veces el layout flex lo empuja.
            // Esto centra el dibujo respecto a su caja original.
            lv_obj_set_style_translate_y(child, 0, 0); 
            return; // Encontrado y rotado
        }
    }
}

