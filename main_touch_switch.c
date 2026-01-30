#include "lvgl.h"
#include "lv_linux_fbdev.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>


static void btn_event_cb(lv_event_t * e)
{
     printf("button pressed \n");
}

struct gpio_btn {
    struct gpiod_line *line;
    struct gpiod_chip *chip;
};

static struct gpio_btn *nav_btn = NULL;
static struct gpio_btn *select_btn = NULL;
static int last_nav_state = 1;
static int last_select_state = 1;
static uint32_t last_nav_ms = 0;
static uint32_t last_select_ms = 0;

static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void)indev;
    static bool send_release = false;
    static uint32_t pending_key = 0;
    const uint32_t debounce_ms = 50;

    if (send_release) {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = pending_key;
        send_release = false;
        return;
    }

    uint32_t now = lv_tick_get();

    if (nav_btn) {
        int nav_state = gpiod_line_get_value(nav_btn->line);
        if (nav_state >= 0) {
            if (last_nav_state == 1 && nav_state == 0 && lv_tick_elaps(last_nav_ms) > debounce_ms) {
                pending_key = LV_KEY_NEXT;
                data->state = LV_INDEV_STATE_PRESSED;
                data->key = pending_key;
                send_release = true;
                last_nav_ms = now;
                last_nav_state = nav_state;
                return;
            }
            last_nav_state = nav_state;
        }
    }

    if (select_btn) {
        int select_state = gpiod_line_get_value(select_btn->line);
        if (select_state >= 0) {
            if (last_select_state == 1 && select_state == 0 && lv_tick_elaps(last_select_ms) > debounce_ms) {
                pending_key = LV_KEY_ENTER;
                data->state = LV_INDEV_STATE_PRESSED;
                data->key = pending_key;
                send_release = true;
                last_select_ms = now;
                last_select_state = select_state;
                return;
            }
            last_select_state = select_state;
        }
    }

    data->state = LV_INDEV_STATE_RELEASED;
}

static struct gpio_btn *initialize_gpio(int pin)
{
    const char *chipname = "gpiochip0";
    struct gpio_btn *button = (struct gpio_btn *)malloc(sizeof(struct gpio_btn));
    if (!button) {
        perror("malloc failed");
        return NULL;
    }

    struct gpiod_chip *chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Open chip failed");
        free(button);
        return NULL;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, pin);
    if (!line) {
        fprintf(stderr, "Cannot find line with pin %d\n", pin);
        gpiod_chip_close(chip);
        free(button);
        return NULL;
    }

    if (gpiod_line_request_input(line, "lvgl_button") < 0) {
        perror("Request line as input failed");
        gpiod_chip_close(chip);
        free(button);
        return NULL;
    }

    button->line = line;
    button->chip = chip;
    return button;
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

    lv_obj_t *active_screen = lv_screen_active();

    lv_obj_t *buttons[4];
    const char *labels[4] = { "Button 1", "Button 2", "Button 3", "Button 4" };

    for (int i = 0; i < 4; i++) {
        buttons[i] = lv_button_create(active_screen);
        lv_obj_set_size(buttons[i], 140, 50);
        lv_obj_align(buttons[i], LV_ALIGN_CENTER, 0, -90 + (i * 60));
        lv_obj_add_event_cb(buttons[i], btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *label = lv_label_create(buttons[i]);
        lv_label_set_text(label, labels[i]);
        lv_obj_center(label);
    }

    lv_group_t *group = lv_group_create();
    for (int i = 0; i < 4; i++) {
        lv_group_add_obj(group, buttons[i]);
    }
    lv_group_focus_obj(buttons[0]);

    unsigned int NAV_GPIO = 21;     // 40 pin fisico (mueve foco)
    unsigned int SELECT_GPIO = 26;  // 37 pin fisico (enter/seleccion)

    nav_btn = initialize_gpio(NAV_GPIO);
    select_btn = initialize_gpio(SELECT_GPIO);

    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, keypad_read);
    lv_indev_set_group(keypad, group);

    while(1) {
        lv_timer_handler();

        usleep(5000);
    }

    return 0;
}
