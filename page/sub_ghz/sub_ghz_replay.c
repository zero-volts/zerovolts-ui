#include "sub_ghz_replay.h"

#include "components/button/ui_button.h"
#include "components/list/ui_list.h"
#include "components/ui_pills.h"
#include "components/ui_theme.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
typedef struct {
    const char *file_paths;
    const char *selected_file;

    ui_list *list;
    ui_pills *times_repeat;
    ui_button *send_button;
    lv_obj_t *status;
    bool sending;
} subghz_replay_ctx;

static subghz_replay_ctx local_ctx;

typedef struct {
    int index;
    const char *text;
    int value;
} subghz_repeat_item;

static subghz_repeat_item repeat_items[] = {
    { 0, "1x", 1}, { 1, "5x", 5}, { 2, "10x", 10}, 
    { 3, "15x", 15}, { 4, "20x", 20},
};

static const char *load_signals(ui_list *list)
{
    zv_file_item_list signals;
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


static void on_signal_selected(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;
    (void)user_data;

    if (local_ctx.sending)
        return;

    local_ctx.selected_file = item->raw_value;
    lv_obj_set_style_text_color(local_ctx.status, COLOR_TEXT_MUTED, 0);
    lv_label_set_text_fmt(local_ctx.status, "Selected: %s", item->text);
}

static void on_replay_finished(ui_status_t status, subghz_event_arg *event)
{
    if (!local_ctx.sending || !event || !event->message)
        return;

    if (status != UI_DONE && status != UI_ERROR)
        return;

    local_ctx.sending = false;
    ui_button_enable(local_ctx.send_button, true);
    lv_obj_set_style_text_color(local_ctx.status, status == UI_DONE ? COLOR_SUCCESS : COLOR_ERROR, 0);
    lv_label_set_text(local_ctx.status, event->message);
}

static void on_replay_page_deleted(lv_event_t *event)
{
    (void)event;
    /* The upload owns its data and can finish after this page is closed. */
    subghz_controller_set_event_cb(NULL);
    local_ctx = (subghz_replay_ctx){0};
}

static void on_send_signal(event_data_btn *event)
{
    (void)event;

    if (!local_ctx.selected_file)
    {
        lv_label_set_text(local_ctx.status, "Select a signal first.");
        return;
    }

    int selected = pills_get_active(local_ctx.times_repeat);
    if (selected < 0 || (size_t)selected >= ARRAY_SIZE(repeat_items))
        return;

    subghz_status_t result = subghz_controller_replay(local_ctx.selected_file, repeat_items[selected].value);
    if (result != SUBGHZ_OK)
    {
        const char *message = subghz_controller_last_error();
        lv_obj_set_style_text_color(local_ctx.status, COLOR_ERROR, 0);
        lv_label_set_text(local_ctx.status, message && message[0] ? message : "Unable to send signal.");
        return;
    }

    local_ctx.sending = true;
    ui_button_enable(local_ctx.send_button, false);
    lv_obj_set_style_text_color(local_ctx.status, COLOR_TEXT_MUTED, 0);
    lv_label_set_text(local_ctx.status, "Sending signal...");
}

zv_view_t *subghz_replay_create(const zv_config *cfg)
{
    zv_view_t *view = zv_view_new("Replay");
    if (!view)
        return NULL;

    local_ctx = (subghz_replay_ctx){ .file_paths = cfg->subghz.signals_path };
    subghz_controller_set_event_cb(on_replay_finished);
    lv_obj_add_event_cb(view->main_container, on_replay_page_deleted, LV_EVENT_DELETE, NULL);

    lv_obj_t *root = view->content;
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 5, 0);
    lv_obj_set_style_pad_row(root, 5, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    local_ctx.list = create_list(root, 100, 50);
    if (!local_ctx.list)
    {
        zv_view_destroy(view);
        return NULL;
    }

    set_list_border(local_ctx.list, false);
    set_list_bg_color_op(local_ctx.list, LV_OPA_TRANSP);
    set_event_data(local_ctx.list, on_signal_selected, NULL);

    lv_obj_t *repeat_panel = lv_obj_create(root);
    lv_obj_set_size(repeat_panel, LV_PCT(100), 45);
    lv_obj_set_style_bg_opa(repeat_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(repeat_panel, 0, 0);
    lv_obj_set_style_pad_all(repeat_panel, 0, 0);
    lv_obj_set_layout(repeat_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(repeat_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(repeat_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    local_ctx.times_repeat = create_pills(repeat_panel);
    pills_change_flex_flow(local_ctx.times_repeat, LV_FLEX_FLOW_ROW);

    size_t size = ARRAY_SIZE(repeat_items);
    for (size_t i = 0; i < size; i++)
    {
        pills_add(local_ctx.times_repeat, repeat_items[i].text);
    }
    
    pills_set_active(local_ctx.times_repeat, 0);
    
    ui_button *repeat_btn = ui_button_create(root, LV_PCT(100), 50, "Send Signal");
    local_ctx.send_button = repeat_btn;
    ui_button_set_on_click(repeat_btn, on_send_signal, NULL);
    ui_button_set_action_style(repeat_btn, Success);

    local_ctx.status = lv_label_create(root);
    lv_obj_set_width(local_ctx.status, LV_PCT(100));
    lv_obj_set_style_text_color(local_ctx.status, COLOR_TEXT_MUTED, 0);
    lv_label_set_text(local_ctx.status, "Select a signal.");

    const char *message = load_signals(local_ctx.list);
    if (message)
    {
        destroy_list(local_ctx.list);
        local_ctx.list = NULL;
        lv_label_set_text(local_ctx.status, message);
    }

    return view;
}
