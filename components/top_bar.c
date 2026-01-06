#include "top_bar.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ui_theme.h"

#define TOP_BAR_HEIGHT 25

typedef struct {
    top_bar_t *bar;
    lv_timer_t *timer;
    lv_timer_t *temp_timer;
    bool auto_clock;
} top_bar_internal_t;

static void clock_timer_cb(lv_timer_t *t)
{
    top_bar_internal_t *it = (top_bar_internal_t *)lv_timer_get_user_data(t);
    if(!it || !it->bar || !it->bar->clock) 
        return;

    if(!it->auto_clock) 
        return;

    time_t now = time(NULL);
    struct tm lt;
    localtime_r(&now, &lt);

    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M", &lt);
    lv_label_set_text(it->bar->clock, buf);
}

static double read_cpu_temp(void) {
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return -1.0;

    int raw;
    if (fscanf(f, "%d", &raw) != 1) {
        fclose(f);
        return -1.0;
    }
    fclose(f);
    return raw / 1000.0;
} 

static void cpu_temp_timer_cb(lv_timer_t *t)
{
    top_bar_internal_t *it = (top_bar_internal_t *)lv_timer_get_user_data(t);
    if(!it || !it->bar || !it->bar->cpu_temp) 
        return;

    double temp = read_cpu_temp();
    char buf[16];

    if(temp < 0) {
        snprintf(buf, sizeof(buf), "--.-°C");
    } else {
        snprintf(buf, sizeof(buf), "%.1f°C", temp);
    }

    lv_label_set_text(it->bar->cpu_temp, buf);
}

static void apply_default_styles(top_bar_t *bar)
{
    /* Contenedor */
    lv_obj_set_size(bar->container, LV_PCT(100), TOP_BAR_HEIGHT);
    lv_obj_set_style_pad_left(bar->container, 10, 0);
    lv_obj_set_style_pad_right(bar->container, 10, 0);
    lv_obj_set_style_pad_top(bar->container, 6, 0);
    lv_obj_set_style_pad_bottom(bar->container, 6, 0);

    lv_obj_set_style_border_width(bar->container, 0, 0);
    lv_obj_set_style_radius(bar->container, 0, 0);

    lv_obj_set_style_bg_opa(bar->container, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar->container, ZV_COLOR_BG_PANEL, 0);

    lv_obj_set_style_text_color(bar->container, ZV_COLOR_TERMINAL, 0);
}

top_bar_t *top_bar_create(lv_obj_t *parent)
{
    top_bar_t *bar = (top_bar_t *)malloc(sizeof(top_bar_t));
    if(!bar) 
        return NULL;

    bar->container = NULL;
    bar->title = NULL;
    bar->clock = NULL;
    bar->cpu_temp = NULL;

    bar->container = lv_obj_create(parent);
    lv_obj_clear_flag(bar->container, LV_OBJ_FLAG_SCROLLABLE);

    apply_default_styles(bar);

    /* Layout principal: izquierda y derecha */
    lv_obj_set_layout(bar->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar->container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        bar->container,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    bar->title = lv_label_create(bar->container);
    lv_label_set_text(bar->title, "v0.1");
    lv_obj_set_style_text_font(bar->title, LV_FONT_DEFAULT, 0);

    lv_obj_t *right = lv_obj_create(bar->container);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(right, 8, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    bar->cpu_temp = lv_label_create(right);
    lv_label_set_text(bar->cpu_temp, "--.-°C");

    bar->clock = lv_label_create(right);
    lv_label_set_text(bar->clock, "--:--:--");

    /* Internals + timer */
    top_bar_internal_t *it = (top_bar_internal_t *)malloc(sizeof(top_bar_internal_t));
    if(it) 
    {
        it->bar = bar;
        it->auto_clock = true;
        it->timer = lv_timer_create(clock_timer_cb, 1000, it);
        it->temp_timer = lv_timer_create(cpu_temp_timer_cb, 3000, it);

        /* Mostrar hora inmediatamente (sin esperar 1s) */
        clock_timer_cb(it->timer);
        cpu_temp_timer_cb(it->temp_timer);
        lv_obj_set_user_data(bar->container, it);
    }

    return bar;
}

void top_bar_set_title(top_bar_t *bar, const char *title)
{
    if(!bar || !bar->title) 
        return;

    lv_label_set_text(bar->title, title ? title : "");
}

void top_bar_set_time(top_bar_t *bar, const char *time_str)
{
    if(!bar || !bar->clock) 
        return;

    lv_label_set_text(bar->clock, time_str ? time_str : "--:--");
}
