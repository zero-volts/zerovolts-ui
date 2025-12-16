#include "lvgl.h"
#include "lv_linux_fbdev.h"
#include "lv_evdev.h"
#include <unistd.h>
#include <stdio.h>

static void btn_event_cb(lv_event_t * e)
{
    printf("boton clickeado\n");
}

int main(void)
{ 
    lv_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (!display) {
        printf("No puedo crear el display!\n");
        return -1;
    } 

    lv_linux_fbdev_set_file(display, "/dev/fb0");
    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event4");
     if (!touch) {
        printf("No puedo abrir el touch\n");
        return -1;
    } 

// TODO: validar por que estos valores, son valores en forma vertical en vez de horizontal?
// 280,3840----------------------3900,3840
//  |                            |
//  |                            |
//  |                            |
//  |                            |
//  |                            |
//  280,400---------------------3900,400
    lv_evdev_set_calibration(touch, 
        280,    // min X
        3840,   // min Y
        3900,   // max X  
        400);   // max Y

    
    lv_obj_t *active_screen = lv_screen_active();

    lv_obj_t * btn = lv_button_create(active_screen);
    
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "btn test");
    lv_obj_center(btn_label);

    
     while(1) {
         lv_timer_handler();
         usleep(5000);
    }

    return 0;
}