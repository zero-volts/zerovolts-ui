#include "sub_ghz_capture.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "utils/logger.h"
#include "components/ui_theme.h"
#include "components/ui_pills.h"
#include "components/dropdown/ui_dropdown.h"
#include "components/component_helper.h"
#include "components/ui_loading_btn.h"
#include "page/sub_ghz/sub_ghz_controller.h"
#include "service/subghz/subghz_service.h"
#include "app_context.h"

#define DEFAULT_CAPTURE_TIMEFRAME_MS 30000 // 30seg

#define CAPTURE_MAX_PULSES       256
#define CHART_POINTS_PER_PULSE     2
#define CHART_MAX_POINTS          (CAPTURE_MAX_PULSES * CHART_POINTS_PER_PULSE)

/* Dejamos margen vertical para que HIGH y LOW no queden bajo el borde. */
#define CHART_LEVEL_LOW            2
#define CHART_LEVEL_HIGH           40
#define CHART_Y_MIN                 0
#define CHART_Y_MAX                50

// STRINGS
#define STR_UNKWNOWN_ERROR          "Unknown error"
#define STR_SAVE_BUTTON             "Save"
#define STR_CAPTURE_BUTTON          "Capture"
#define STR_CAPTURING_BUTTON        "Capturing"
#define STR_STATUS_INITIAL          "Start a capture pressing the capture\nbutton."
#define STR_STATUS_CAPTURE_WAITING  "Waiting for signal... Press the remote \ncontrol"
#define STR_STATUS_CAPTURE_READY    "Captured, not saved. Review the signal, \nthen save or discard."

typedef struct  {
    int option_index;
    uint32_t value;
    const char *name;
} frequency_t;

static const dropdown_item_t frequencies[] = {
    {0, 315000000U, "315 MHz"},
    {1, 433920000U, "433.92 MHz"},
    {2, 868000000U, "868 MHz"},
    {3, 915000000U, "915 MHz"},
};

typedef struct {
    int selected_frequency;
    uint64_t sessions_ended;

    // UI componentes
    ui_pills *frequency_pills;
    lv_obj_t *chart;
    ui_loading_button *capture_btn;
    lv_obj_t *save_btn;

    lv_obj_t *status_lbl;
    lv_obj_t *discard_btn;

    // chart variables
    int32_t x_accumulator;
    uint32_t point_idx;
    uint16_t expected_seq;
} subghz_capture_ctx;

static subghz_capture_ctx local_ctx;

static void reset_chart_capture(lv_chart_series_t *ser, uint32_t max_points)
{
    local_ctx.x_accumulator = 0;
    local_ctx.point_idx = 0;
    local_ctx.expected_seq = 0;

    for (uint32_t i = 0; i < max_points; i++) 
    {
        lv_chart_set_series_value_by_id2(local_ctx.chart, ser, i,
            LV_CHART_POINT_NONE,
            LV_CHART_POINT_NONE
        );
    }

    lv_chart_set_axis_range(local_ctx.chart, LV_CHART_AXIS_PRIMARY_X, 0, 1);
}

static void update_chart_time_range(void)
{
    int32_t x_max = local_ctx.x_accumulator;
    int32_t margin = x_max / 20; /* 5 % */

    if (margin < 1)
        margin = 1;

    if (x_max <= INT32_MAX - margin)
        x_max += margin;

    if (x_max < 1)
        x_max = 1;

    lv_chart_set_axis_range(local_ctx.chart, LV_CHART_AXIS_PRIMARY_X, 0, x_max);
}

static void add_data(subghz_data_chunk_t *data)
{
    if (!data || !local_ctx.chart)
        return;

    lv_chart_series_t *serie = lv_chart_get_series_next(local_ctx.chart, NULL);
    if (!serie)
        return;

    uint32_t max_points = lv_chart_get_point_count(local_ctx.chart);
    if (data->chunks == 0 || data->seq >= data->chunks)
        return;

    /* Una nueva captura comienza con el chunk cero. */
    if (data->seq == 0)
    {
        // A new capture start, maybe another signal entered
        reset_chart_capture(serie, max_points);
    }

    // waiting for ordered sequencies
    if (data->seq != local_ctx.expected_seq)
        return;

    const uint16_t timings_capacity = sizeof(data->timings) / sizeof(data->timings[0]);
    const uint16_t timings_count = data->count < timings_capacity ? data->count : timings_capacity;
    for (uint16_t i = 0; i < timings_count; i++) 
    {
        if (local_ctx.point_idx + 2 > max_points)
            break;

        int32_t timing = data->timings[i];
        if (timing == 0)
            continue;

        int32_t duration = timing < 0 ? -timing : timing;
        int32_t level = timing > 0 ? CHART_LEVEL_HIGH : CHART_LEVEL_LOW;

        if (duration > INT32_MAX - local_ctx.x_accumulator)
            break;

        lv_chart_set_series_value_by_id2(
            local_ctx.chart, serie, local_ctx.point_idx++, local_ctx.x_accumulator, level);

        local_ctx.x_accumulator += duration;
        lv_chart_set_series_value_by_id2(local_ctx.chart, serie, 
            local_ctx.point_idx++, local_ctx.x_accumulator, level);
    }

    local_ctx.expected_seq++;

    update_chart_time_range();

    lv_chart_refresh(local_ctx.chart);
}

static void capture_handler(ui_status_t status, subghz_event_arg *event)
{
    if (status == UI_ERROR)
    {
        const char *message = event && event->message ? event->message : STR_UNKWNOWN_ERROR;
        
        log_error("capture_handler: %s\n", message);
        lv_label_set_text(local_ctx.status_lbl, message);

        loading_button_set_text(local_ctx.capture_btn, STR_CAPTURE_BUTTON);
        loading_button_set_loading(local_ctx.capture_btn, false);
        lv_obj_add_state(local_ctx.save_btn, LV_STATE_DISABLED);

        return;
    }

    if (status == UI_DONE)
    {
        local_ctx.sessions_ended = event->session_id;

        lv_label_set_text(local_ctx.status_lbl, STR_STATUS_CAPTURE_READY);
        loading_button_set_text(local_ctx.capture_btn, STR_CAPTURE_BUTTON);
        loading_button_set_loading(local_ctx.capture_btn, false);

        lv_obj_remove_flag(local_ctx.discard_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_state(local_ctx.save_btn, LV_STATE_DISABLED);
        return;
    }

    if (status == UI_LOADING && event == NULL)
    {
        log_info("Sub-GHz capture started");
        return;
    }

    if (event == NULL || event->data == NULL)
    {
        log_warning("capture_handler: data is null, current status %d\n", status);
        return;
    }

    subghz_data_chunk_t *data = event->data;

    printf("seq: %u\n", data->seq);
    printf("count: %u\n", data->count);
    printf("chunks: %u\n", data->chunks);
    add_data(data);

    printf("\n");
}

static void subghz_capture_handler(lv_event_t *e)
{
    loading_button_set_text(local_ctx.capture_btn, STR_CAPTURING_BUTTON);
    loading_button_set_loading(local_ctx.capture_btn, true);

    lv_label_set_text(local_ctx.status_lbl, STR_STATUS_CAPTURE_WAITING);

    subghz_status_t result = subghz_controller_start_capture(frequencies[local_ctx.selected_frequency].value, DEFAULT_CAPTURE_TIMEFRAME_MS);
    if (result != SUBGHZ_OK)
    {
        loading_button_set_text(local_ctx.capture_btn, STR_CAPTURE_BUTTON);
        loading_button_set_loading(local_ctx.capture_btn, false);
        log_error("Unable to start Sub-GHz capture");
    }
}

static void subghz_save_capture_handler(lv_event_t *e)
{
    (void)e;

    subghz_capture_session_t *session = subghz_get_session_by(local_ctx.sessions_ended);
    subghz_status_t result = subghz_controller_save_data(session, NULL);
    if (result != SUBGHZ_OK)
        return;

    lv_label_set_text(local_ctx.status_lbl, STR_STATUS_INITIAL);

    local_ctx.sessions_ended = 0;
    lv_obj_add_state(local_ctx.save_btn, LV_STATE_DISABLED);
    lv_obj_add_flag(local_ctx.discard_btn, LV_OBJ_FLAG_HIDDEN);

    lv_chart_series_t *serie = lv_chart_get_series_next(local_ctx.chart, NULL);
    if (!serie)
        return;

    uint32_t max_points = lv_chart_get_point_count(local_ctx.chart);
    reset_chart_capture(serie, max_points);
}

static void subghz_discard_capture_handler(lv_event_t *e)
{
    (void)e;
    subghz_controller_discard_session(local_ctx.sessions_ended);

    local_ctx.sessions_ended = 0;
    lv_obj_add_state(local_ctx.save_btn, LV_STATE_DISABLED);
    lv_obj_add_flag(local_ctx.discard_btn, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(local_ctx.status_lbl, STR_STATUS_INITIAL);

    lv_chart_series_t *serie = lv_chart_get_series_next(local_ctx.chart, NULL);
    if (!serie)
        return;

    uint32_t max_points = lv_chart_get_point_count(local_ctx.chart);
    reset_chart_capture(serie, max_points);
}

static void on_dropdown_changes(dropdown_item_t *item, void *user_data)
{
    local_ctx.selected_frequency = item->position;
}

static void create_frequency_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(panel, 10, 0);

    lv_obj_t *freq_lbl = create_section_label(panel, "FREQUENCY");
    lv_obj_set_style_text_font(freq_lbl, &lv_font_montserrat_10, 0);

    ui_dropdown *dropdown = dropdown_create(panel, LV_PCT(45), 40);
    dropdown_set_on_change_cb(dropdown, on_dropdown_changes, NULL);
    dropdown_set_items(dropdown, frequencies, sizeof(frequencies) / sizeof(frequencies[0]));
}

static void create_signal_chart(lv_obj_t *parent)
{
    if (local_ctx.chart != NULL)
        return;

    local_ctx.chart = lv_chart_create(parent);

    lv_obj_set_size(local_ctx.chart, lv_pct(100), lv_pct(30));
    lv_obj_set_style_bg_color(local_ctx.chart, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_border_color(local_ctx.chart, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_line_width(local_ctx.chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(local_ctx.chart, 0, 0, LV_PART_INDICATOR);
    lv_chart_set_type(local_ctx.chart, LV_CHART_TYPE_SCATTER);
    lv_chart_set_update_mode(local_ctx.chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_point_count(local_ctx.chart, CHART_MAX_POINTS);

    lv_chart_add_series(local_ctx.chart, ZV_COLOR_TERMINAL, LV_CHART_AXIS_PRIMARY_Y);

    lv_chart_set_axis_range(local_ctx.chart, LV_CHART_AXIS_PRIMARY_Y, CHART_Y_MIN, CHART_Y_MAX);
    lv_chart_set_axis_range(local_ctx.chart, LV_CHART_AXIS_PRIMARY_X, 0, 1);
}

static void create_action_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, LV_PCT(100), LV_PCT(40));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_pad_row(panel, 0, 0);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);

    local_ctx.status_lbl = create_section_label(panel, STR_STATUS_INITIAL);
    local_ctx.discard_btn = create_btn(panel, "Discard", LV_PCT(100), 50);
    lv_obj_add_event_cb(local_ctx.discard_btn, subghz_discard_capture_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(local_ctx.discard_btn, LV_OBJ_FLAG_HIDDEN);
        
}

static void create_bottom_panel(lv_obj_t *parent)
{
    lv_obj_t *bottom_panel = lv_obj_create(parent);
    lv_obj_set_size(bottom_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bottom_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_panel, 0, 0);
    lv_obj_set_style_pad_all(bottom_panel, 0, 0);
    lv_obj_set_layout(bottom_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bottom_panel, 10, 0);

    local_ctx.capture_btn = create_loading_btn(bottom_panel,  LV_PCT(48), 50, STR_CAPTURE_BUTTON);
    loading_set_event_cb(local_ctx.capture_btn, subghz_capture_handler, NULL);

    local_ctx.save_btn = create_btn(bottom_panel, STR_SAVE_BUTTON, LV_PCT(48), 50);
    lv_obj_add_event_cb(local_ctx.save_btn, subghz_save_capture_handler, LV_EVENT_CLICKED, NULL);
}

lv_obj_t *subghz_capture_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Signal Capture");

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    create_frequency_panel(root);
    create_signal_chart(root);
    create_action_panel(root);  
    create_bottom_panel(root);

    subghz_controller_set_capture_cb(capture_handler);

    return page;
}
