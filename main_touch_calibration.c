#include "lvgl.h"
#include "lv_linux_fbdev.h"
#include <unistd.h>
#include <stdio.h>


static void btn_event_cb(lv_event_t * e)
{
     printf("button pressed \n");
}

int driver_initialization(lv_display_t *display)
{
    lv_linux_fbdev_set_file(display, "/dev/fb0");
    // To know the event we use "cat /proc/bus/input/devices"
    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event4");
    if (!touch)
        return -1;

    // We swap axes because LVGL don't know that the screen is rotated 90degrees, so with
    // this swap the input are fixed internally.
    lv_evdev_set_swap_axes(touch, true);

    // We use evtest /dev/input/event4 command to know the screen edges points.
    lv_evdev_set_calibration(touch, 296, 294, 3931, 3843);

    return 0;
}

lv_obj_t *create_button(lv_obj_t *parent, const char *text, lv_event_cb_t handler)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 120, 50);

    if (handler)
        lv_obj_add_event_cb(btn, handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

#define BG_COLOR            lv_color_hex(0xB2C0C2)
#define TEXT_COLOR_WHITE    lv_color_hex(0xFFFFFF)

void create_top_bar(lv_obj_t *parent)
{
    lv_obj_t *top_bar = lv_obj_create(parent);
    lv_obj_set_layout(top_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);

    lv_obj_set_size(top_bar, LV_PCT(100), 48);

    lv_obj_set_style_bg_color(top_bar, BG_COLOR, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(top_bar, 0, 0);

    lv_obj_t *title = lv_label_create(top_bar);
    lv_obj_set_style_text_color(title, TEXT_COLOR_WHITE, LV_PART_MAIN);
    lv_label_set_text(title, "zero volts");
}

int main(void)
{ 
    lv_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (!display)
    {
        printf("Can't create the display!\n");
        return -1;
    }

    int sucess = driver_initialization(display);
    if (sucess == -1)
    {
        printf("Can't initialize the driver!\n");
        return -1;
    }

    // root layout
    lv_obj_t *active_screen = lv_screen_active();
    lv_obj_set_layout(active_screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(active_screen, LV_FLEX_FLOW_COLUMN);

    create_top_bar(active_screen);
    create_button(active_screen, "HID", NULL);
    
     while(1) {
         lv_timer_handler();
         usleep(5000);
    }

    return 0;
}