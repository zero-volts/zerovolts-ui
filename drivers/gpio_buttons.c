#include "gpio_buttons.h"

#include <stdio.h>
#include <stdlib.h>

struct gpio_btn *gpio_btn_init(int pin)
{
    const char *chipname = "gpiochip0";
    struct gpio_btn *button = (struct gpio_btn *)malloc(sizeof(struct gpio_btn));
    if (!button) 
    {
        perror("malloc failed");
        return NULL;
    }

    struct gpiod_chip *chip = gpiod_chip_open_by_name(chipname);
    if (!chip) 
    {
        perror("Open chip failed");
        free(button);
        return NULL;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, pin);
    if (!line) 
    {
        fprintf(stderr, "Cannot find line with pin %d\n", pin);
        gpiod_chip_close(chip);
        free(button);
        return NULL;
    }

    if (gpiod_line_request_input(line, "gpio_button") < 0) 
    {
        perror("Request line as input failed");
        gpiod_chip_close(chip);
        free(button);
        return NULL;
    }

    button->line = line;
    button->chip = chip;
    return button;
}

int gpio_btn_get(struct gpio_btn *button)
{
    if (!button || !button->line)
        return -1;

    return gpiod_line_get_value(button->line);
}

void gpio_btn_close(struct gpio_btn *button)
{
    if (!button)
        return;

    if (button->line)
        gpiod_line_release(button->line);

    if (button->chip)
        gpiod_chip_close(button->chip);

    free(button);
}
