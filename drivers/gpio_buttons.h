#ifndef GPIO_BUTTONS_H
#define GPIO_BUTTONS_H

#include <gpiod.h>

struct gpio_btn {
    struct gpiod_line *line;
    struct gpiod_chip *chip;
};

struct gpio_btn *gpio_btn_init(int pin);
int gpio_btn_get(struct gpio_btn *button);
void gpio_btn_close(struct gpio_btn *button);

#endif
