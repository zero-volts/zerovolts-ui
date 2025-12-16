TARGET := zero-volts-ui
CC := g++
LVPORT := $(HOME)/git/lv_port_linux

SRC := \
	main.c \
	components/top_bar.c \
	page/hid.c

INCLUDES := \
	-I. \
	-I$(LVPORT)/lvgl \
	-I$(LVPORT)/lvgl/src/drivers/display/fb \
	-I$(LVPORT)/lvgl/src/drivers/evdev \
	-I$(LVPORT)/build

CFLAGS := -DLV_CONF_INCLUDE_SIMPLE -Wall
LDFLAGS := -lm -lpthread -ldl

LIBS := $(LVPORT)/build/lvgl/lib/liblvgl.a

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(INCLUDES) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)