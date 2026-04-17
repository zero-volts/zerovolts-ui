#include "bt_scanner.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "components/ui_loading_btn.h"
#include "bt_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ui_list *scanner_list = NULL;
static ui_loading_button *scann_btn = NULL;
static lv_obj_t *lb_devices_amount = NULL;

static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    printf("Seleccionado :%s, MAC: %s\n", item->text, item->subtitle);
}

static void handler_devices(const char *event, device_t *device)
{
    if (scanner_list == NULL)
        return;

    if (strstr(event, "SCAN:DONE") != NULL)
    {
        loading_button_set_loading(scann_btn, false);
    }

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
            }
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
    loading_button_set_loading(scann_btn, true);
    clean_list(scanner_list);
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
    lv_obj_clear_flag(filter_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(filter_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(filter_container, LV_FLEX_FLOW_ROW);
}

lv_obj_t *bt_scanner_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "BLE Scanner");

    set_scanner_cb(handler_devices);

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
    // create_filter_panel(root);

    /// --- listado
    scanner_list = create_list(root, 100, 70);
    set_event_data(scanner_list, handler, NULL);

    return page;
}