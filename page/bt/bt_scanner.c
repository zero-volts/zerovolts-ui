#include "bt_scanner.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "components/ui_loading_btn.h"
#include "components/component_helper.h"
#include "bt_device_detail.h"
#include "bt_controller.h"
#include "components/nav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ui_list *scanner_list = NULL;
static ui_loading_button *scann_btn = NULL;
static lv_obj_t *lb_devices_amount = NULL;
static lv_obj_t *device_detail_page = NULL;

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;

    bt_controller_select((const device_t *)item->user_data);
    nav_ctx_t *ctx = (nav_ctx_t *)user_data;
    if (!ctx || !ctx->menu)
        return;

    if (device_detail_page != NULL)
    {
        lv_obj_delete(device_detail_page);
        device_detail_page = NULL;
    }

    device_detail_page = bt_device_detail_page_create(ctx->menu);
    ctx->page = device_detail_page;

    if (!ctx->page)
        return;

    lv_menu_set_page(ctx->menu, ctx->page);
    zv_nav_update_group(ctx->menu, ctx->page);
}

static void handler_devices(const char *event, device_t *device)
{
    if (scanner_list == NULL)
        return;

    if (strstr(event, "SCAN:DONE") != NULL)
        loading_button_set_loading(scann_btn, false);

    if (strstr(event, "SCAN:UPDATE") != NULL)
    {
        char rssi_buffer[16];
        snprintf(rssi_buffer, sizeof(rssi_buffer), "Rssi: %d", device->rssi);
        
        list_item_t item = {
            .text = device->name,
            .subtitle = device->mac,
            .left_badge = {
                .type = BAGE_IMG_TYPE,
                .icon = {
                    .path = "/home/zerovolts/git/zerovolts-ui/data/assets/tower.png",
                    .size = { .width = 30, .height = 30 }
                }
            },
            .right_badge = {
                .label =  rssi_buffer,
                .type = BAGE_TEXT_TYPE
            },
            .user_data = device,
            .user_data_size = sizeof(device_t)
        };

        add_item(scanner_list, &item);

        int item_count = item_length(scanner_list);

        char device_text[12];
        snprintf(device_text, sizeof(device_text), "Devices %d", item_count);
        lv_label_set_text(lb_devices_amount, device_text);
    }
}

static void handler_scan_btn(lv_event_t *e)
{
    (void)e;
    loading_button_set_loading(scann_btn, true);
    clean_list(scanner_list);
    bt_controller_clean_ctx();
    start_scan();
}

static void create_scaner_panel(lv_obj_t *parent)
{
    lv_obj_t *scann_btn_container = lv_obj_create(parent);
    lv_obj_set_size(scann_btn_container, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(scann_btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scann_btn_container, 0, 0);
    lv_obj_clear_flag(scann_btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(scann_btn_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scann_btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scann_btn_container, 
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lb_devices_amount = lv_label_create(scann_btn_container);
    lv_label_set_text(lb_devices_amount, "Devices ");
    lv_obj_set_style_text_color(lb_devices_amount, ZV_COLOR_TERMINAL, 0);
    lv_obj_set_style_bg_color(lb_devices_amount, ZV_COLOR_WARNING, 0);

    scann_btn = create_loading_btn(scann_btn_container, 77, 40, "Scan");
    loading_set_event_cb(scann_btn, handler_scan_btn, NULL);
}

void create_filter_panel(lv_obj_t *parent)
{
    lv_obj_t *filter_container = lv_obj_create(parent);
    lv_obj_set_size(filter_container, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(filter_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(filter_container, 0, 0);
    lv_obj_set_style_pad_all(filter_container, 0, 0);
    lv_obj_set_style_pad_row(filter_container, 4, 0);
    lv_obj_clear_flag(filter_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(filter_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(filter_container, LV_FLEX_FLOW_ROW);

    lv_obj_t *all_btn = create_btn(filter_container, "All", 50, 40);
    lv_obj_t *near_btn = create_btn(filter_container, "Near", 65, 40);
    lv_obj_t *connect_btn = create_btn(filter_container, "Connectable", 120, 40);
}

lv_obj_t *bt_scanner_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "BLE Scanner");

    set_scanner_cb(handler_devices);
    bt_controller_clean_ctx();

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    create_scaner_panel(root);
    create_filter_panel(root);

    static nav_ctx_t nav_detail;
    nav_detail.menu = menu;
    nav_detail.page = NULL;
    
    scanner_list = create_list(root, 100, 70);
    set_event_data(scanner_list, handler, &nav_detail);

    return page;
}
