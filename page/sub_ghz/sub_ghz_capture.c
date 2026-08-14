#include "sub_ghz_capture.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "components/ui_theme.h"
#include "components/ui_pills.h"
#include "components/component_helper.h"
#include "page/sub_ghz/sub_ghz_controller.h"

#define DEFAULT_CAPTURE_TIMEFRAME_MS 30000 // 30seg
typedef struct  {
    int option_index;
    int value;
    char *name;
} frecuency_t;

static frecuency_t frecuencies[] = {
    {0, 315, "315 MHz"},
    {1, 433920000, "433.92 MHz"},
    {2, 868, "868 MHz"},
    {3, 915, "915 MHz"},
};

typedef struct {
    int selected_frecuency;
    ui_pills *frecuency_pills;
    lv_obj_t *chart;
} subghz_capture_ctx;

static subghz_capture_ctx local_context;

static void add_data(subghz_data_chunk_t *data)
{
    if (local_context.chart == NULL)
        return;

    lv_chart_series_t * ser = lv_chart_get_series_next(local_context.chart, NULL);
    for (int i = 0; i < data->count; i++)
    {
        printf("timing[%d]: %ld,", i, (long)data->timings[i]);
        lv_chart_set_next_value(local_context.chart, ser, data->timings[i]);
    }

    uint32_t p = lv_chart_get_point_count(local_context.chart);
    uint32_t s = lv_chart_get_x_start_point(local_context.chart, ser);
    int32_t * a = lv_chart_get_series_y_array(local_context.chart, ser);

    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;

    lv_chart_refresh(local_context.chart);
}

static void capture_handler(ui_status_t status, subghz_data_chunk_t *data)
{
    printf("seq: %u\n", data->seq);
    printf("count: %u\n", data->count);
    printf("chunks: %u\n", data->chunks);
    add_data(data);

    printf("\n");   
}

static void subghz_capture_handler(lv_event_t *e)
{
    subghz_start_capture(frecuencies[local_context.selected_frecuency].value, DEFAULT_CAPTURE_TIMEFRAME_MS);
}

static void on_frecuency_change(ui_pills *pills, int index, const char *label, void *user_data)
{
    local_context.selected_frecuency = index;
}

static void create_frecuency_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    
    lv_obj_set_size(panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(panel, 10, 0);

    if (local_context.frecuency_pills == NULL)
    {
        local_context.frecuency_pills = create_pills(panel);
        pills_change_flex_flow(local_context.frecuency_pills, LV_FLEX_FLOW_ROW);
        pills_change_scroll_mode(local_context.frecuency_pills, LV_SCROLLBAR_MODE_AUTO);
    }

    size_t count = sizeof(frecuencies) / sizeof(frecuencies[0]);
    for (size_t i = 0; i< count; i++ )
    {
        pills_add(local_context.frecuency_pills, frecuencies[i].name);
    }

    pills_set_active(local_context.frecuency_pills, 0);
    pills_set_event_cb(local_context.frecuency_pills, on_frecuency_change, NULL);
}

static void create_signal_chart(lv_obj_t *parent)
{
    //TODO: esta no es la forma de representar la señal, corregir mas adelante!
    //https://lvgl.io/docs/open/widgets/chart
    if (local_context.chart != NULL)
        return;

    local_context.chart = lv_chart_create(parent);
    lv_chart_set_update_mode(local_context.chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_obj_set_size(local_context.chart, lv_pct(100), lv_pct(40));
    lv_obj_set_style_bg_color(local_context.chart, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_border_color(local_context.chart, ZV_COLOR_BORDER, 0);
    lv_chart_set_type(local_context.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(local_context.chart, 512);

    lv_chart_series_t * lv_chart_series_0 = lv_chart_add_series(local_context.chart, ZV_COLOR_TERMINAL, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(local_context.chart, lv_chart_series_0, 0);

    // static const int32_t chart_1_values_0[] = {10, 35, 25, 55, 40, 70, 60, 85};
    // lv_chart_set_series_values(chart_1, lv_chart_series_0, chart_1_values_0, 8);
    lv_chart_set_axis_min_value(local_context.chart, LV_CHART_AXIS_PRIMARY_Y, -12000);
    lv_chart_set_axis_max_value(local_context.chart, LV_CHART_AXIS_PRIMARY_Y, 12000);
}

static void create_capture_button(lv_obj_t *parent)
{
    lv_obj_t *capture_btn = lv_btn_create(parent);
    lv_obj_set_size(capture_btn, LV_PCT(100), 40);
    lv_obj_set_style_bg_color(capture_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(capture_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(capture_btn, 2, 0);
    lv_obj_set_style_border_color(capture_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(capture_btn, 12, 0);
    lv_obj_add_event_cb(capture_btn, subghz_capture_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *capture_label = lv_label_create(capture_btn);
    lv_label_set_text(capture_label, "Start Capture");
    lv_obj_set_style_text_color(capture_label, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_text_font(capture_label, &lv_font_montserrat_10, 0);
    lv_obj_center(capture_label);
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

    create_section_label(root, "FRECUENCY");
    create_frecuency_panel(root);
    create_signal_chart(root);
    create_capture_button(root);
    

    subghz_set_capture_cb(capture_handler);

    return page;
}
