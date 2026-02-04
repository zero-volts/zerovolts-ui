
TARGET := bin/zero-volts-ui
CC := g++
LVPORT := $(HOME)/git/lv_port_linux

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

all: setup $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)

setup:
	mkdir -p bin

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
