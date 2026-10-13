#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h> // PATH_MAX
#include <string.h>

#include "lvgl.h"
#include "lv_linux_fbdev.h"
#include "lv_evdev.h"

#include "gpio_buttons.h"

#include "components/top_bar.h"
#include "components/ui_theme.h"
#include "components/component_helper.h"
#include "page/navigation_page.h"
#include "page/home_view.h"
#include "page/hid/hid_view.h"
#include "page/ir/ir.h"
#include "page/ir/learn_button.h"
#include "page/ir/new_remote.h"
#include "page/bt/bt_view.h"
#include "page/sub_ghz/sub_ghz_view.h"
#include "config.h"
#include "service/uart_service.h"
#include "utils/error_handler.h"
#include "utils/file.h"
#include "utils/logger.h"

#define NAV_GPIO    21  // Physical pin 40 (moves focus)
#define SELECT_GPIO 26  // Physical pin 37 (enter/select)

/* ================= TOUCH ================= */

int driver_initialization(lv_display_t *display, const zv_config *config)
{
    lv_linux_fbdev_set_file(display, config->display.fb_device);

    // Auto-detect touchscreen via /proc/bus/input/devices (INPUT_PROP_DIRECT)
    char touch_path[128];
    if (detect_touch_event_path(touch_path, sizeof(touch_path)) != 0)
    {
        log_error("Touch auto-detect failed, no touchscreen found\n");
        return -1;
    }

    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, touch_path);
    if (!touch)
        return -1;

    // We swap axes because LVGL don't know that the screen is rotated 90degrees, so with
    // this swap the input are fixed internally.
    lv_evdev_set_swap_axes(touch, true);

    // We use evtest /dev/input/event4 command to know the screen edges points.
    lv_evdev_set_calibration(touch, 296, 294, 3931, 3843);

    return 0;
}

const zv_config *setup_config()
{
    char exe_dir[PATH_MAX];
    if (get_executable_dir(exe_dir, sizeof(exe_dir)) == 0) 
    {
        const char *app_config_path = "/../app-config.json";
        size_t size = strlen(exe_dir) + strlen(app_config_path) + 1;

        char config_path[PATH_MAX];
        if (size > sizeof(config_path)) {
            log_error("Config path too long: %s%s\n", exe_dir, app_config_path);
            return NULL;
        }

        int written = snprintf(config_path, sizeof(config_path), "%s%s", exe_dir, app_config_path);
        if (written < 0 || (size_t)written >= sizeof(config_path)) {
            log_error("Config path truncated: %s%s\n", exe_dir, app_config_path);
            return NULL;
        }

        log_debug("config_path=%s\n", config_path);

        initialize_config(config_path);
        config_load();

        char project_root[PATH_MAX];
        written = snprintf(project_root, sizeof(project_root), "%s/..", exe_dir);
        if (written < 0 || (size_t)written >= sizeof(project_root)) {
            log_error("Project root path too long: %s/..\n", exe_dir);
            return NULL;
        }

        config_resolve_paths(project_root);
    }

    return config_get();
}

/* ================= NAV ================= */

static struct gpio_btn *nav_btn = NULL;
static struct gpio_btn *select_btn = NULL;
static int last_nav_state = 1;
static int last_select_state = 1;
static uint32_t last_nav_ms = 0;
static uint32_t last_select_ms = 0;

static uint32_t resolve_nav_key(lv_indev_t *indev)
{
    lv_group_t *group;
    lv_obj_t *focused;

    group = lv_indev_get_group(indev);
    focused = group ? lv_group_get_focused(group) : NULL;
    if (!focused)
        return LV_KEY_NEXT;

    /* Keep classic focus navigation unless we are inside a widget that needs directional keys. */
    if (lv_obj_check_type(focused, &lv_keyboard_class))
        return LV_KEY_RIGHT;

    if (lv_obj_check_type(focused, &lv_dropdown_class) &&
        lv_dropdown_is_open(focused)) {
        return LV_KEY_DOWN;
    }

    if (lv_obj_check_type(focused, &lv_dropdownlist_class) &&
        !lv_obj_has_flag(focused, LV_OBJ_FLAG_HIDDEN)) {
        return LV_KEY_DOWN;
    }

    return LV_KEY_NEXT;
}

static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
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
        int nav_state = gpio_btn_get(nav_btn);
        if (nav_state >= 0) {
            if (last_nav_state == 1 && nav_state == 0 && lv_tick_elaps(last_nav_ms) > debounce_ms) {
                pending_key = resolve_nav_key(indev);
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
        int select_state = gpio_btn_get(select_btn);
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

void setup_navigation_groups(lv_obj_t *page)
{
    lv_group_t *group = lv_group_create();
    lv_group_set_wrap(group, true);
    zv_nav_set_group(group);
    zv_nav_update_group(page);

    nav_btn = gpio_btn_init(NAV_GPIO);
    select_btn = gpio_btn_init(SELECT_GPIO);

    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, keypad_read);
    lv_indev_set_group(keypad, group);
}


int main(void)
{
    char exe_dir[PATH_MAX];
    if (get_executable_dir(exe_dir, sizeof(exe_dir)) == 0) 
    {
        const char *path_log = "/../data/app-log.log";
        size_t size = strlen(exe_dir) + strlen(path_log) + 1;

        char config_path[PATH_MAX];
        snprintf(config_path, size, "%s%s", exe_dir, path_log);
        init_logger(config_path, DEBUG);
    }

    log_info("ZeroVolts starting ....");

    const zv_config *config = setup_config();
    if (!config) {
        log_error("config is NULL\n");
        return -1;
    }

    lv_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (!display) {
        log_error("Can't create the display\n");
        return -1;
    }

    if (driver_initialization(display, config) != 0)
        return -1;

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_pad_all(scr, 0, 0);

    /* Root layout: column */
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr, 0, 0);

    /* -------- TOP BAR -------- */

    top_bar_t *top_bar = top_bar_create(scr);

    char version[24];
    snprintf(version, sizeof(version), "v%s", config->version);
    top_bar_set_title(top_bar, version);

    /* -------- HOME (remaining space) -------- */

    static home_item home_items[] = {
        {
            .label = "Bad USB",
            .icon = LV_SYMBOL_USB,
            .rotate_icon_90 = false,
            .create_view = hid_view_create,
        },
        {
            .label = "Infrared",
            .icon = LV_SYMBOL_WIFI,
            .rotate_icon_90 = true,
            .create_view = ir_page_create,
        },
        {
            .label = "Bluethoot",
            .icon = LV_SYMBOL_BLUETOOTH,
            .rotate_icon_90 = false,
            .create_view = bt_page_create,
        },
        {
            .label = "Sub-GHz",
            .icon = LV_SYMBOL_GPS,
            .rotate_icon_90 = false,
            .create_view = subghz_view_create,
        }
    };

    zv_view_t *home = home_view_create(home_items,
            sizeof(home_items) / sizeof(home_items[0]), config);

    if (!home)
    {
        log_error("home_view_create returned NULL (items=%p count=%zu)\n",
               (void *)home_items,
               sizeof(home_items) / sizeof(home_items[0]));
        return -1;
    }

    lv_obj_t *home_page = get_page(home);
    log_debug("home=%p home_page=%p\n", (void *)home, (void *)home_page);
    if (!home_page) {
        log_error("get_page returned NULL\n");
        return -1;
    }

    if (!zv_nav_push(home)) {
        log_error("Could not register Home in navigation stack\n");
        zv_view_destroy(home);
        return -1;
    }

    setup_navigation_groups(home_page);
    while (1)
    {
        lv_timer_handler();

        uart_process_loop();

        usleep(5000);
    }

    return 0;
}
