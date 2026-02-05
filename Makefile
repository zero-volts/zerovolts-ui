
APP_TARGET := bin/zero-volts-ui
CC := g++
LVPORT := $(HOME)/git/lv_port_linux
EXAMPLE_SRCS := $(wildcard examples/main_*.c)
EXAMPLE_TARGETS := $(patsubst examples/main_%.c,bin/example-%,$(EXAMPLE_SRCS))

SRC := \
	main.c \
	config.c \
	components/top_bar.c \
	components/component_helper.c \
	components/nav.c \
	page/hid.c \
	page/ir/ir.c \
	page/ir/learn_button.c \
	page/ir/new_remote.c \
	page/ir/remotes.c \
	page/ir/send_signal.c \
	ir/ir_service.c \
	utils/file.c \
	utils/string_utils.c \
	utils/cJSON.c \
	../tactile_switch/gpio_buttons.c


INCLUDES := \
	-I. \
	-I../tactile_switch \
	-I$(LVPORT)/lvgl \
	-I$(LVPORT)/lvgl/src/drivers/display/fb \
	-I$(LVPORT)/lvgl/src/drivers/evdev \
	-I$(LVPORT)/build

CFLAGS := -DLV_CONF_INCLUDE_SIMPLE -Wall
LDFLAGS := -lm -lpthread -ldl -lgpiod

LIBS := $(LVPORT)/build/lvgl/lib/liblvgl.a

.PHONY: all setup clean run examples example

all: setup $(APP_TARGET)

$(APP_TARGET): $(SRC)
	$(CC) $(SRC) -o $(APP_TARGET) $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)

setup:
	mkdir -p bin

examples: setup $(EXAMPLE_TARGETS)

bin/example-%: examples/main_%.c
	$(CC) $< -o $@ $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)

example: setup
ifndef NAME
	$(error Debes indicar NAME. Ejemplo: make example NAME=hello_world)
endif
	$(MAKE) bin/example-$(NAME)

clean:
	rm -f $(APP_TARGET) $(EXAMPLE_TARGETS)

run: $(APP_TARGET)
	./$(APP_TARGET)
