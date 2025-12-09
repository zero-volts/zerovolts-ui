#include "lvgl.h"
#include "lv_linux_fbdev.h"
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    /*LVGL init*/
    lv_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (!display) {
        printf("No puedo crear el display!\n");
        return -1;
    }

    lv_linux_fbdev_set_file(display, "/dev/fb0");

    /*Create a "Hello world!" label*/
    lv_obj_t *active_screen = lv_screen_active();

     lv_obj_t * label = lv_label_create(active_screen);
     lv_label_set_text(label, "Hello world!");
     lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    /*Handle LitlevGL tasks (tickless mode)*/
     while(1) {
         lv_timer_handler();
         usleep(5000);
    }

    return 0;
}
