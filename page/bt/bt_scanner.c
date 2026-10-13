#include "bt_scanner.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "components/button/ui_button.h"
#include "components/ui_pills.h"
#include "bt_device_detail.h"
#include "bt_controller.h"
#include "page/navigation_page.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ui_list *scanner_list = NULL;
static ui_button *scan_btn = NULL;
static lv_obj_t *lb_devices_amount = NULL;
static lv_obj_t *scanner_root = NULL;
static ui_pills *filter_pills = NULL;

static void on_filter_change(ui_pills *pills, int index, const char *label, void *user_data);

static lv_color_t rssi_color(int rssi)
{
    if (rssi >= -60) 
        return COLOR_SUCCESS;
    if (rssi >= -80) 
        return COLOR_TEXT;

    return COLOR_WARNING;
}

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    (void)list;

    bt_controller_select_device((const device_t *)item->user_data);

    nav_ctx_t ctx = { bt_device_detail_page_create, (const zv_config *)user_data };
    zv_nav_open(&ctx);
}

static list_item_t create_list_item(device_t *device, char *rssi_buffer, size_t rssi_buffer_size)
{
    snprintf(rssi_buffer, rssi_buffer_size, "%d", device->rssi);

    const char *icon_path;
    if (strcmp(device->manufacturer, "Microsoft") == 0)
        icon_path = "icons/microsoft.png";
    else if (strcmp(device->manufacturer, "Apple") == 0)
        icon_path = "icons/apple.png";
    else if (strcmp(device->manufacturer, "Samsung") == 0)
        icon_path = "icons/samsung.png";
    else
        icon_path = "icons/unknown.png";

    list_item_t item = {
        .text = device->name,
        .subtitle = device->mac,
        .left_badge = {
            .type = BADGE_IMG_TYPE,
            .icon = {
                .path = icon_path,
                .size = { .width = 30, .height = 30 }
            }
        },
        .right_badge = {
            .label = rssi_buffer,
            .type = BADGE_TEXT_TYPE,
            .text_color = rssi_color(device->rssi),
            .has_text_color = true,
        },
        .user_data = device,
        .user_data_size = sizeof(device_t)
    };

    return item;
}

static void handler_devices(device_t *device, ui_status_t status)
{
    if (scanner_list == NULL)
        return;

    if (status == UI_LOADING)
        ui_button_set_loading(scan_btn, true);

    if (status == UI_DONE)
    {
        ui_button_set_loading(scan_btn, false);
        // Reapply current filter so Near re-sorts and any drift gets corrected.
        on_filter_change(NULL, pills_get_active(filter_pills), NULL, NULL);
        return;
    }

    if (device == NULL)
        return;

    // Respect Connectable filter during live scan; Near sorts on UI_DONE.
    int active = pills_get_active(filter_pills);
    if (active == 2 && !device->connectable)
        return;

    char rssi_buffer[16];
    list_item_t item = create_list_item(device, rssi_buffer, sizeof(rssi_buffer));
    add_item(scanner_list, &item);

    char device_text[24];
    int item_count = item_length(scanner_list);
    snprintf(device_text, sizeof(device_text), "Devices: %d", item_count);
    lv_label_set_text(lb_devices_amount, device_text);

    if (scanner_root && !lv_obj_has_flag(scanner_root, LV_OBJ_FLAG_HIDDEN))
        zv_nav_update_group(scanner_root);
}

static void handler_scan_btn(event_data_btn *e)
{
    (void)e;
    ui_button_set_loading(scan_btn, true);
    clean_list(scanner_list);
    bt_controller_reset_devices();

    if (lb_devices_amount)
        lv_label_set_text(lb_devices_amount, "Devices: 0");

    if (start_scan() != UART_OK)
        ui_button_set_loading(scan_btn, false);
}

static void create_scanner_panel(lv_obj_t *parent)
{
    lv_obj_t *scan_btn_container = lv_obj_create(parent);
    lv_obj_set_size(scan_btn_container, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(scan_btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scan_btn_container, 0, 0);
    lv_obj_clear_flag(scan_btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(scan_btn_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scan_btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scan_btn_container,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lb_devices_amount = lv_label_create(scan_btn_container);
    lv_label_set_text(lb_devices_amount, "Devices: 0");
    lv_obj_set_style_text_color(lb_devices_amount, COLOR_TERMINAL, 0);

    scan_btn = ui_button_create(scan_btn_container, 80, 40, "Scan");
    ui_button_set_action_style(scan_btn, Success);
    ui_button_set_on_click(scan_btn, handler_scan_btn, NULL);
}

static void on_filter_change(ui_pills *pills, int index, const char *label, void *user_data)
{
    (void)pills;
    (void)label;
    (void)user_data;

    switch(index)
    {
        case 0:
            bt_reset_visible_devices();
            break;
        case 1:
            bt_sort_visible_devices_by_nearest();
            break;
        case 2:
            bt_apply_connectable_filter();
            break;
        default:
            bt_reset_visible_devices();
            break;
    }

    int devices_length = bt_get_visible_devices_length();
    device_t *devices = bt_get_visible_devices();
    clean_list(scanner_list);

    for (int i = 0; i < devices_length; i++)
    {
        device_t *device = &devices[i];

        char rssi_buffer[16];
        list_item_t item = create_list_item(device, rssi_buffer, sizeof(rssi_buffer));
        add_item(scanner_list, &item);
    }

    char device_text[24];
    snprintf(device_text, sizeof(device_text), "Devices: %d", item_length(scanner_list));
    lv_label_set_text(lb_devices_amount, device_text);
    if (scanner_root && !lv_obj_has_flag(scanner_root, LV_OBJ_FLAG_HIDDEN))
        zv_nav_update_group(scanner_root);
}

static void create_filter_panel(lv_obj_t *parent)
{
    filter_pills = create_pills(parent);
    pills_add(filter_pills, "All");
    pills_add(filter_pills, "Near");
    pills_add(filter_pills, "Connectable");
    pills_set_active(filter_pills, 0);
    pills_set_event_cb(filter_pills, on_filter_change, NULL);
}

static void scanner_delete_cb(lv_event_t *e)
{
    (void)e;

    set_scanner_cb(NULL);
    scanner_list = NULL;
    scanner_root = NULL;
    scan_btn = NULL;
    filter_pills = NULL;
    lb_devices_amount = NULL;
    bt_controller_reset_devices();
}

zv_view_t *bt_scanner_page_create(const zv_config *cfg)
{
    zv_view_t *view = zv_view_new("BLE Scanner");
    if (!view)
        return NULL;

    scanner_root = view->main_container;
    lv_obj_add_event_cb(scanner_root, scanner_delete_cb, LV_EVENT_DELETE, NULL);
    lv_obj_t *root = view->content;
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    create_scanner_panel(root);
    create_filter_panel(root);

    scanner_list = create_list(root, 100, 70);
    if (!scanner_list)
    {
        zv_view_destroy(view);
        return NULL;
    }

    set_event_data(scanner_list, handler, (void *)cfg);

    set_scanner_cb(handler_devices);
    return view;
}
