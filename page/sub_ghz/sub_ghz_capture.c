#include "sub_ghz_capture.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "utils/logger.h"
#include "components/ui_theme.h"
#include "components/ui_pills.h"
#include "components/component_helper.h"
#include "components/ui_loading_btn.h"
#include "page/sub_ghz/sub_ghz_controller.h"

#define DEFAULT_CAPTURE_TIMEFRAME_MS 30000 // 30seg

#define CAPTURE_MAX_PULSES       256
#define CHART_POINTS_PER_PULSE     2
#define CHART_MAX_POINTS          (CAPTURE_MAX_PULSES * CHART_POINTS_PER_PULSE)

/* Dejamos margen vertical para que HIGH y LOW no queden bajo el borde. */
#define CHART_LEVEL_LOW            2
#define CHART_LEVEL_HIGH           40
#define CHART_Y_MIN                 0
#define CHART_Y_MAX                50

typedef struct  {
    int option_index;
    uint32_t value;
    const char *name;
} frequency_t;

static const frequency_t frequencies[] = {
    //{0, 315000000U, "315 MHz"},
    {0, 433920000U, "433.92 MHz"},
    //{2, 868000000U, "868 MHz"},
    //{3, 915000000U, "915 MHz"},
};

typedef struct {
    int selected_frequency;
    ui_pills *frequency_pills;
    lv_obj_t *chart;
    ui_loading_button *capture_btn;

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
        const char *message = event && event->message ? event->message : "Unknown capture error";
        
        log_error("capture_handler: %s\n", message);

        loading_button_set_text(local_ctx.capture_btn, "Start Capture");
        loading_button_set_loading(local_ctx.capture_btn, false);

        return;
    }

    if (status == UI_DONE)
    {
        loading_button_set_text(local_ctx.capture_btn, "Start Capture");
        loading_button_set_loading(local_ctx.capture_btn, false);
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
    loading_button_set_text(local_ctx.capture_btn, "Capturing");
    loading_button_set_loading(local_ctx.capture_btn, true);

    uart_status_t result = subghz_start_capture(frequencies[local_ctx.selected_frequency].value, DEFAULT_CAPTURE_TIMEFRAME_MS);
    if (result != UART_OK)
    {
        loading_button_set_text(local_ctx.capture_btn, "Start Capture");
        loading_button_set_loading(local_ctx.capture_btn, false);

        log_error("Unable to start Sub-GHz capture");
    }
}

static void on_frequency_change(ui_pills *pills, int index, const char *label, void *user_data)
{
    local_ctx.selected_frequency = index;
}

static void create_frequency_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(panel, 10, 0);

    if (local_ctx.frequency_pills == NULL)
    {
        local_ctx.frequency_pills = create_pills(panel);
        pills_change_flex_flow(local_ctx.frequency_pills, LV_FLEX_FLOW_ROW);
        pills_change_scroll_mode(local_ctx.frequency_pills, LV_SCROLLBAR_MODE_AUTO);
    }

    size_t count = sizeof(frequencies) / sizeof(frequencies[0]);
    for (size_t i = 0; i< count; i++ )
    {
        pills_add(local_ctx.frequency_pills, frequencies[i].name);
    }

    pills_set_active(local_ctx.frequency_pills, 0);
    pills_set_event_cb(local_ctx.frequency_pills, on_frequency_change, NULL);
}

static void create_signal_chart(lv_obj_t *parent)
{
    if (local_ctx.chart != NULL)
        return;

    local_ctx.chart = lv_chart_create(parent);
    
    lv_obj_set_size(local_ctx.chart, lv_pct(100), lv_pct(40));
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

static void create_capture_button(lv_obj_t *parent)
{
    local_ctx.capture_btn = create_loading_btn(parent,  LV_PCT(100), 40, "Start Capture");
    loading_set_event_cb(local_ctx.capture_btn, subghz_capture_handler, NULL);
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

    lv_obj_t *freq_lbl = create_section_label(root, "FREQUENCY");
    lv_obj_set_style_text_font(freq_lbl, &lv_font_montserrat_10, 0);
    create_frequency_panel(root);
    create_signal_chart(root);
    create_capture_button(root);
    

    subghz_set_capture_cb(capture_handler);

    return page;
}
