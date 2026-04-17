#include "ui_loading_btn.h"
#include <stdlib.h>

struct ui_loading_button{
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *spinner;
    lv_event_cb_t cb;
    void *user_data;
};

ui_loading_button *create_loading_btn(lv_obj_t *parent, int width, int height, const char *text)
{
    ui_loading_button *comp = (ui_loading_button *)calloc(1, sizeof(ui_loading_button));
    if (comp == NULL) {
        return NULL;
    }

    // Botón principal
    comp->btn = lv_btn_create(parent);
    lv_obj_set_size(comp->btn, width, height);

    lv_obj_set_layout(comp->btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(comp->btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(comp->btn,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    // Spinner (oculto por defecto)
    comp->spinner = lv_spinner_create(comp->btn);
    lv_spinner_set_anim_params(comp->spinner, 1000, 60);
    lv_obj_set_size(comp->spinner, 16, 16);
    lv_obj_add_flag(comp->spinner, LV_OBJ_FLAG_HIDDEN);

    // Texto
    comp->label = lv_label_create(comp->btn);
    lv_label_set_text(comp->label, text);

    return comp;
}

void loading_set_event_cb(ui_loading_button *comp, lv_event_cb_t cb, void *user_data)
{
    if (comp == NULL || comp->btn == NULL) {
        return;
    }

    comp->user_data = user_data;
    comp->cb = cb;

    lv_obj_add_event_cb(comp->btn, cb, LV_EVENT_CLICKED, user_data);
}

void loading_button_set_text(ui_loading_button *comp, const char *text)
{
    lv_label_set_text(comp->label, text);
}

void loading_button_set_loading(ui_loading_button *comp, bool loading)
{
    if (loading)
    {
        lv_obj_clear_flag(comp->spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(comp->btn, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_flag(comp->spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(comp->btn, LV_STATE_DISABLED);
    }
}

void destroy_loading_btn(ui_loading_button *comp)
{
    if (comp == NULL) {
        return;
    }

    lv_obj_clean(comp->btn);
    free(comp);
}