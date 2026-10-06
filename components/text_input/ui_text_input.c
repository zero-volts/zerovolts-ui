#include "ui_text_input.h"
#include <stdlib.h>
#include "utils/logger.h"
#include "components/ui_theme.h"

struct ui_text_input {
    lv_obj_t *keyboard;
    lv_obj_t *active_textarea;
    bool use_on_screen_keyboard;
    
    lv_obj_t *input;
};

static void keyboard_hide(ui_text_input *ui)
{
    lv_group_t *group;
    if (!ui || !ui->use_on_screen_keyboard || !ui->keyboard)
        return;

    log_debug("ui_text_input::keyboard_hide hidding keyboard");
    group = lv_obj_get_group(ui->keyboard);
    if (group)
        lv_group_set_editing(group, false);

    lv_obj_add_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(ui->keyboard, NULL);
    if (group)
        lv_group_remove_obj(ui->keyboard);

    if (ui->active_textarea && group)
        lv_group_focus_obj(ui->active_textarea);

    ui->active_textarea = NULL;
}

static void keyboard_show(ui_text_input *ui, lv_obj_t *target)
{
    log_debug("ui_text_input::keyboard_show 1");
    lv_group_t *group;
    if (!ui || !target)
        return;

    log_debug("ui_text_input::keyboard_show 2");

    if (!ui->use_on_screen_keyboard || !ui->keyboard)
        return;

    log_debug("ui_text_input::keyboard_show");
    
    ui->active_textarea = target;
    lv_keyboard_set_textarea(ui->keyboard, target);
    lv_obj_clear_flag(ui->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui->keyboard);

    group = lv_obj_get_group(target);
    if (group) 
    {
        log_debug("ui_text_input::keyboard_show 4");
        if (!lv_obj_get_group(ui->keyboard))
            lv_group_add_obj(group, ui->keyboard);

        lv_group_focus_obj(ui->keyboard);
        lv_group_set_editing(group, true);
    }
}

static void keyboard_event_handler(lv_event_t *e)
{
    log_debug("ui_text_input::keyboard_event_handler llego al evento del teclado en el componente nuevo");
    ui_text_input *text_input = (ui_text_input *)lv_event_get_user_data(e);
    if (!text_input) 
        return;

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
        keyboard_hide(text_input);
}

static void ui_input_handler(lv_event_t *e)
{
    ui_text_input *text_input = (ui_text_input *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);

    if (!text_input || !target) 
        return;
    
    log_debug("ui_text_input::ui_input_handler event=%d focused=%d\n",
           (int)code, lv_obj_has_state(target, LV_STATE_FOCUSED) ? 1 : 0);

    if (code == LV_EVENT_KEY) 
    {
        uint32_t key = lv_event_get_key(e);
        log_debug("ui_text_input::ui_input_handler key=%u\n", (unsigned)key);
        if (key == LV_KEY_ENTER)
            keyboard_show(text_input, target);

        return;
    }

    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED)
        keyboard_show(text_input, target);
}

ui_text_input *ui_text_create(lv_obj_t *parent, int width, int height, const char *hint)
{
    ui_text_input *text_input = (ui_text_input *)malloc(sizeof(ui_text_input));

    text_input->input = lv_textarea_create(parent);
    text_input->use_on_screen_keyboard = true;

    lv_obj_set_size(text_input->input, width, height);
    lv_textarea_set_placeholder_text(text_input->input, hint);
    lv_obj_set_style_bg_color(text_input->input, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(text_input->input, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(text_input->input, 2, 0);
    lv_obj_set_style_border_color(text_input->input, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(text_input->input, 10, 0);
    lv_obj_set_style_text_color(text_input->input, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_color(text_input->input, ZV_COLOR_TEXT_MUTED, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_left(text_input->input, 10, 0);
    lv_obj_add_event_cb(text_input->input, ui_input_handler, LV_EVENT_FOCUSED, text_input);
    lv_obj_add_event_cb(text_input->input, ui_input_handler, LV_EVENT_DEFOCUSED, text_input);
    lv_obj_add_event_cb(text_input->input, ui_input_handler, LV_EVENT_CLICKED, text_input);
    lv_obj_add_event_cb(text_input->input, ui_input_handler, LV_EVENT_PRESSED, text_input);
    lv_obj_add_event_cb(text_input->input, ui_input_handler, LV_EVENT_KEY, text_input);

    // Creating the keyboard on top layer to avoid menu/page clipping issues.
    text_input->keyboard = lv_keyboard_create(lv_layer_top());
    // TODO: calcular dinamicamente el ancho y alto de la pantalla.
    lv_obj_set_size(text_input->keyboard, lv_pct(100), 120);
    lv_obj_align(text_input->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(text_input->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(text_input->keyboard, keyboard_event_handler, LV_EVENT_READY, text_input);
    lv_obj_add_event_cb(text_input->keyboard, keyboard_event_handler, LV_EVENT_CANCEL, text_input);

    return text_input;
}

const char *ui_text_get_text(ui_text_input *input_text)
{
    if (!input_text)
    {
        log_error("ui_text_input::ui_text_input_get_text input is NULL");
        return NULL;
    }
        
    const char *text = lv_textarea_get_text(input_text->input);
    if (!text || !text[0])
        return NULL;

    return text;
}

void ui_text_clear(ui_text_input *input_text)
{
    if (!input_text)
        return;

    lv_textarea_set_text(input_text->input, "");
}