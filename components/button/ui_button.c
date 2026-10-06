#include "ui_button.h"
#include "components/ui_theme.h"

#include <stdlib.h>

static uint32_t initial_button_id = 100;

struct ui_button {
    uint32_t id;
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *spinner;
    bool enabled;
    bool loading;
    ui_button_on_click on_click;
    void *user_data;
};

static void button_delete_handler(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

static void update_disabled_state(ui_button *button)
{
    if (button->loading || !button->enabled)
        lv_obj_add_state(button->btn, LV_STATE_DISABLED);
    else
        lv_obj_remove_state(button->btn, LV_STATE_DISABLED);
}

static void click_button_handler(lv_event_t *e)
{
    ui_button *button = (ui_button *)lv_event_get_user_data(e);
    if (!button || button->loading || !button->enabled)
        return;

    if (button->on_click != NULL)
    {
        event_data_btn event = {
            .btn_id = button->id,
            .user_data = button->user_data
        };

        button->on_click(&event);
    }
}

ui_button *ui_button_create(lv_obj_t *parent, int width, int height, const char *text)
{
    ui_button *button = (ui_button *)calloc(1, sizeof(ui_button));
    if (!button)
        return NULL;

    button->id  = initial_button_id++;
    button->enabled = true;
    button->on_click = NULL;
    button->user_data = NULL;

    button->btn = lv_btn_create(parent);
    if (!button->btn)
    {
        free(button);
        return NULL;
    }

    lv_obj_add_event_cb(button->btn, button_delete_handler, LV_EVENT_DELETE, button);

    lv_obj_set_size(button->btn, width, height);
    lv_obj_set_style_bg_color(button->btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(button->btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button->btn, 2, 0);
    lv_obj_set_style_border_color(button->btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(button->btn, 12, 0);
    lv_obj_add_event_cb(button->btn, click_button_handler, LV_EVENT_CLICKED, button);
    lv_obj_set_layout(button->btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button->btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button->btn, LV_FLEX_ALIGN_CENTER,
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(button->btn, 6, 0);

    button->label = lv_label_create(button->btn);
    if (!button->label)
    {
        lv_obj_delete(button->btn);
        return NULL;
    }

    lv_label_set_text(button->label, text ? text : "");
    lv_obj_set_style_text_color(button->label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_clear_flag(button->label, LV_OBJ_FLAG_CLICKABLE);

    return button;
}

ui_button *ui_icon_button_create(lv_obj_t *parent, int width, int height, const char *icon)
{
    return ui_button_create(parent, width, height, icon);
}

void ui_button_set_on_click(ui_button *button, ui_button_on_click callback, void *user_data)
{
    if (!button)
        return;

    button->on_click = callback;
    button->user_data = user_data;
}

void ui_button_hide(ui_button *button, bool hide)
{
    if (!button)
        return;

    if (hide)
        lv_obj_add_flag(button->btn, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_remove_flag(button->btn, LV_OBJ_FLAG_HIDDEN);
}

void ui_button_enable(ui_button *button, bool enable)
{
    if (!button)
        return;

    button->enabled = enable;
    update_disabled_state(button);
}

void ui_button_set_text(ui_button *button, const char *text)
{
    if (button)
        lv_label_set_text(button->label, text ? text : "");
}

void ui_button_set_loading(ui_button *button, bool loading)
{
    if (!button)
        return;

    if (loading && !button->spinner)
    {
        button->spinner = lv_spinner_create(button->btn);
        if (!button->spinner)
            return;

        lv_spinner_set_anim_params(button->spinner, 1000, 60);
        lv_obj_set_size(button->spinner, 16, 16);
        lv_obj_clear_flag(button->spinner, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_to_index(button->spinner, 0);
    }

    button->loading = loading;
    if (button->spinner)
    {
        if (loading)
            lv_obj_remove_flag(button->spinner, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(button->spinner, LV_OBJ_FLAG_HIDDEN);
    }

    update_disabled_state(button);
}

lv_obj_t *ui_button_get_obj(ui_button *button)
{
    return button ? button->btn : NULL;
}

void ui_button_destroy(ui_button *button)
{
    if (button)
        lv_obj_delete(button->btn);
}