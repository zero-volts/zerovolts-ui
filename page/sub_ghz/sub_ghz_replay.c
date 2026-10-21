#include "sub_ghz_replay.h"

#include "components/button/ui_button.h"
#include "components/list/ui_list.h"
#include "components/ui_pills.h"
#include "components/ui_theme.h"

typedef struct {
    const char *file_paths;
    ui_list *list;
} subghz_replay_ctx;

static subghz_replay_ctx local_ctx;

static const char *load_signals(ui_list *list)
{
    subghz_signal_list signals;
    if (subghz_controller_list_signals(local_ctx.file_paths, &signals) != SUBGHZ_OK)
        return subghz_controller_last_error();

    const char *message = signals.count == 0 ? "No saved signals found." : NULL;
    for (size_t i = 0; i < signals.count; i++)
    {
        list_item_t item = {
            .text = signals.signals[i].name,
            .left_badge = {
                .label = LV_SYMBOL_FILE,
                .type = BADGE_TEXT_TYPE,
                .text_color = COLOR_ACCENT,
                .has_text_color = true,
            },
            .raw_value = signals.signals[i].path,
        };

        if (!add_item(list, &item))
        {
            message = "Unable to display signal list.";
            break;
        }
    }

    subghz_controller_free_signal_list(&signals);
    return message;
}


static void on_refresh_handler(event_data_btn *event)
{
    load_signals(local_ctx.list);
}

zv_view_t *subghz_replay_create(const zv_config *cfg)
{
    zv_view_t *view = zv_view_new("Replay");
    if (!view)
        return NULL;

    lv_obj_t *root = view->content;
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 5, 0);
    lv_obj_set_style_pad_row(root, 5, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    // lv_obj_t *center_container = lv_obj_create(root);
    // lv_obj_set_size(center_container, LV_PCT(100), 45);
    // lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    // lv_obj_set_style_border_width(center_container, 0, 0);
    // lv_obj_set_style_pad_all(center_container, 0, 0);
    // lv_obj_set_layout(center_container, LV_LAYOUT_FLEX);
    // lv_obj_set_flex_flow(center_container, LV_FLEX_FLOW_ROW);
    // lv_obj_set_flex_align(center_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // lv_obj_t *selected_lbl = lv_label_create(center_container);
    // lv_obj_set_style_text_color(selected_lbl, COLOR_TEXT, 0);
    // lv_label_set_text(selected_lbl, "Stored signals");

    // ui_button *refresh_btn = ui_icon_button_create(center_container, 45, 35, LV_SYMBOL_REFRESH);
    // ui_button_set_on_click(refresh_btn, on_refresh_handler, NULL);

    local_ctx.list = create_list(root, 100, 50);
    if (!local_ctx.list)
    {
        zv_view_destroy(view);
        return NULL;
    }

    set_list_border(local_ctx.list, false);
    set_list_bg_color_op(local_ctx.list, LV_OPA_TRANSP);

    lv_obj_t *repeat_panel = lv_obj_create(root);
    lv_obj_set_size(repeat_panel, LV_PCT(100), 45);
    lv_obj_set_style_bg_opa(repeat_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(repeat_panel, 0, 0);
    lv_obj_set_style_pad_all(repeat_panel, 0, 0);
    lv_obj_set_layout(repeat_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(repeat_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(repeat_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui_pills *times_repeat = create_pills(repeat_panel);
    pills_change_flex_flow(times_repeat, LV_FLEX_FLOW_ROW);
    pills_add(times_repeat, "1x");
    pills_add(times_repeat, "5x");
    pills_add(times_repeat, "10x");
    pills_add(times_repeat, "15x");
    pills_add(times_repeat, "20x");
    pills_set_active(times_repeat, 0);
    //pills_set_event_cb(times_repeat, NULL, NULL);

    ui_button *repeat_btn = ui_button_create(root, LV_PCT(100), 50, "Send Signal");
    ui_button_set_on_click(repeat_btn, NULL, NULL);
    ui_button_set_action_style(repeat_btn, Success);


    local_ctx.file_paths = cfg->subghz.signals_path;

    const char *message = load_signals(local_ctx.list);
    if (message)
    {
        destroy_list(local_ctx.list);
        lv_obj_t *status = lv_label_create(root);
        lv_obj_set_width(status, LV_PCT(100));
        lv_obj_set_style_text_color(status, COLOR_TEXT_MUTED, 0);
        lv_label_set_text(status, message);
    }

    return view;
}
