#include "lvgl.h"
#include "lv_linux_fbdev.h"
#include "lv_evdev.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "components/top_bar.h"
#include "components/ui_theme.h"
#include "page/hid.h"

// https://www.youtube.com/@techcifer/videos
typedef struct {
    lv_obj_t *menu;
    lv_obj_t *page;
} nav_ctx_t; 

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

static void goto_page_cb(lv_event_t *e)
{
    nav_ctx_t *ctx = (nav_ctx_t *)lv_event_get_user_data(e);
    if (!ctx || !ctx->menu || !ctx->page) return;

    lv_menu_set_page(ctx->menu, ctx->page);
}

/* ================= UI HELPERS ================= */

static lv_obj_t *create_button(lv_obj_t *parent, const char *text, const char *icon, lv_event_cb_t cb, void *user_data)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_PCT(48), 100);  // ajusta alto según tu pantalla
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2A3340), 0);
    lv_obj_set_style_radius(btn, 10, 0);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    // estados
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x101826), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, ZV_COLOR_TERMINAL, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, ZV_COLOR_TERMINAL, LV_STATE_CHECKED);

    // layout interno
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn, 6, 0);

    lv_obj_t * ic = lv_label_create(btn);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t * lb = lv_label_create(btn);
    lv_label_set_text(lb, text);
    lv_obj_set_style_text_color(lb, ZV_COLOR_TEXT_MAIN, 0);

    return btn;
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

int main(void)
{
    lv_init();

    lv_display_t *display = lv_linux_fbdev_create();
    if (!display) {
        printf("No puedo crear el display\n");
        return -1;
    }

    if (driver_initialization(display) != 0)
        return -1;

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_pad_all(scr, 0, 0);

    /* Root layout: column */
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(scr, 0, 0);

    /* -------- TOP BAR -------- */
    
    top_bar_t *top_bar = top_bar_create(scr);
    top_bar_set_title(top_bar, "ZERO VOLTS");

    /* -------- MENU (full restante) -------- */
    lv_obj_t *menu = lv_menu_create(scr);
    lv_obj_set_size(menu, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(menu, 1);
    lv_obj_set_style_pad_all(menu, 0, 0);
    lv_obj_set_style_bg_color(menu, ZV_COLOR_BG_MAIN, 0);

    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);


    /* -------- PAGES -------- */
    lv_obj_t *home_page = lv_menu_page_create(menu, NULL);
    //lv_obj_t *page1     = lv_menu_page_create(menu, "Page 1");
    zv_hid_cfg_t hid_cfg = {
        .scripts_dir = "/home/zerovolts/git/pi-hid-keyboad",
        .selected_path = "/var/lib/zerovolts/hid-selected.txt"
    };

    lv_obj_t *page1 = zv_hid_page_create(menu, &hid_cfg);

    /* -------- PAGE 1 CONTENT -------- */
    lv_obj_t *p1 = page_root_full(page1);
    lv_obj_set_layout(p1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(p1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(p1, 12, 0);

    lv_obj_t *p1_lbl = lv_label_create(p1);
    lv_label_set_text(p1_lbl, "Hello from Page 1");

    /* -------- HOME CONTENT (botones azules) -------- */
    lv_obj_t *home = page_root_full(home_page);
    
    lv_obj_set_layout(home, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(home, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(home, 14, 0);

    static nav_ctx_t nav1;
    nav1.menu = menu; 
    nav1.page = page1;

    create_button(home, "Bad USB", LV_SYMBOL_USB, goto_page_cb, &nav1);
    
        
    lv_menu_set_page(menu, home_page);

    while (1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
