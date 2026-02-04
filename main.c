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
#include "components/nav.h"
#include "page/hid.h"
#include "page/ir/ir.h"
#include "page/ir/learn_button.h"
#include "page/ir/new_remote.h"
#include "ir/ir_service.h"
#include "config.h"
#include "utils/file.h"

/* ================= TOUCH ================= */

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


/* Crea un root container full size dentro de una page */
static lv_obj_t *page_root_full(lv_obj_t *page)
{
    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(root, 16, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    return root;
}

lv_obj_t *create_home_page(lv_obj_t *parent)
{
    lv_obj_t *home = page_root_full(parent);
    
    lv_obj_set_layout(home, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(home, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(home, 14, 0);

    return home;
}

static lv_obj_t * find_label_child(lv_obj_t *parent)
{
    uint32_t n = lv_obj_get_child_count(parent);
    for(uint32_t i = 0; i < n; i++) 
    {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if(lv_obj_check_type(child, &lv_label_class)) return child;
    }

    return NULL;
}

static lv_obj_t * find_btn_child(lv_obj_t *parent)
{
    uint32_t n = lv_obj_get_child_count(parent);
    for(uint32_t i = 0; i < n; i++) 
    {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if(lv_obj_check_type(child, &lv_button_class)) return child;
    }

    return NULL;
}

void zv_menu_header_style(lv_obj_t *menu)
{
    lv_obj_t *header = lv_menu_get_main_header(menu);
    lv_obj_t *title = find_label_child(header);

    if(title) 
        lv_obj_set_style_text_color(title, ZV_COLOR_TEXT_MAIN, 0);
    
    lv_obj_t *back_btn = find_btn_child(header);
    if(back_btn) 
    {
        lv_obj_set_size(back_btn, 30, 30);
        lv_obj_set_style_radius(back_btn, 22, 0);

        lv_obj_t *icon = lv_obj_get_child(back_btn, 0);
        if(icon)
            lv_obj_set_style_text_color(icon, ZV_COLOR_TEXT_MAIN, 0);
    }
}


int main(void)
{
    lv_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (!display) {
        printf("Can't create the display\n");
        return -1;
    }

    if (driver_initialization(display) != 0)
        return -1;

    char exe_dir[PATH_MAX];
    if (get_executable_dir(exe_dir, sizeof(exe_dir)) == 0) 
    {
        char config_path[PATH_MAX];
        snprintf(config_path, sizeof(config_path), "%s/../app-config.json", exe_dir);

        printf("[CFG] exe_dir=%s\n", exe_dir);
        printf("[CFG] config_path=%s\n", config_path);

        initialize_config(config_path);
        config_load();
    }

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_pad_all(scr, 0, 0);

    /* Root layout: column */
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr, 0, 0);

    const zv_config *config = config_get();

    ir_service_cfg_t ir_cfg = {
        .backend = config->ir.backend,
        .tx_dev = config->ir.tx_device,
        .rx_dev = config->ir.rx_device,
        .remotes_root = config->ir.remotes_path,
        .learn_timeout_ms = config->ir.learn_timeout_ms
    };
    ir_service_init(&ir_cfg);

    /* -------- TOP BAR -------- */
    
    top_bar_t *top_bar = top_bar_create(scr);

    char version[3];
    snprintf(version, sizeof(version) + sizeof(config->version), "v%s", config->version);
    top_bar_set_title(top_bar, version);

    /* -------- MENU (full restante) -------- */
    lv_obj_t *menu = lv_menu_create(scr);
    zv_menu_header_style(menu);
    lv_obj_set_size(menu, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(menu, 1);
    lv_obj_set_style_pad_all(menu, 0, 0);
    lv_obj_set_style_bg_color(menu, ZV_COLOR_BG_MAIN, 0);

    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);

    /* -------- PAGES -------- */
    lv_obj_t *home_page = lv_menu_page_create(menu, NULL);

    lv_obj_t *hid_page = hid_page_create(menu, config);
    lv_obj_t *ir_page = ir_page_create(menu, config);
    lv_obj_t *home = create_home_page(home_page);

    static nav_ctx_t nav1;
    nav1.menu = menu; 
    nav1.page = hid_page;

    static nav_ctx_t nav2;
    nav2.menu = menu; 
    nav2.page = ir_page;

    create_square_main_button(home, "Bad USB", LV_SYMBOL_USB, zv_goto_page_cb, &nav1);
    lv_obj_t *ir_button = create_square_main_button(home, "Infrared", LV_SYMBOL_WIFI, zv_goto_page_cb, &nav2);
    rotate_icon_by_tag(ir_button, 90);

    lv_menu_set_page(menu, home_page);

    lv_group_t *group = lv_group_create();
    lv_group_set_wrap(group, true);
    zv_nav_set_group(group);
    zv_nav_update_group(menu, home_page);

    unsigned int NAV_GPIO = 21;     // 40 pin fisico (mueve foco)
    unsigned int SELECT_GPIO = 26;  // 37 pin fisico (enter/seleccion)

    nav_btn = gpio_btn_init(NAV_GPIO);
    select_btn = gpio_btn_init(SELECT_GPIO);

    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, keypad_read);
    lv_indev_set_group(keypad, group);

    lv_obj_t *last_page = lv_menu_get_cur_main_page(menu);
    while (1) {
        lv_timer_handler();
        lv_obj_t *cur_page = lv_menu_get_cur_main_page(menu);
        if (cur_page && cur_page != last_page) {
            zv_nav_update_group(menu, cur_page);
            last_page = cur_page;
        }
        usleep(5000);
    }

    return 0;
}
