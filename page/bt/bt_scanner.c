#include "bt_scanner.h"
#include "components/ui_theme.h"
#include "components/list/ui_list.h"
#include "bt_controller.h"

#include <stdio.h>
#include <stdlib.h>

static ui_list *scanner_list = NULL;
static void handler(ui_list *list, const list_item_t *item, void *user_data)
{
    printf("Seleccionado :%s, MAC: %s\n", item->text, item->subtitle);
}

static void handler_devices(device_t *device)
{
    if (scanner_list == NULL)
        return;

    char rssi_buffer[16];
    snprintf(rssi_buffer, sizeof(rssi_buffer), "Rssi: %d", device->rssi);
    
    list_item_t item = {
        .text = device->name,
        .subtitle = device->mac,
        .left_bage = {
            .type = BAGE_IMG_TYPE,
            .icon = {
                .path = "/home/zerovolts/git/zerovolts-ui/data/assets/tower.png",
                .size = { .width = 30, .height = 30 }
            }
        },
        .right_bage = {
            .label =  rssi_buffer,
            .type = BAGE_TEXT_TYPE
        }
    };

    add_item(scanner_list, &item);
}

static void handler_scan_btn(lv_event_t *e)
{
    clean_list(scanner_list);
    start_scan();
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

    /// --- botones superiores
    lv_obj_t *scann_btn_container = lv_obj_create(root);
    lv_obj_set_size(scann_btn_container, LV_PCT(100), 60);
    lv_obj_set_style_bg_opa(scann_btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scann_btn_container, 0, 0);
    lv_obj_clear_flag(scann_btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(scann_btn_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scann_btn_container, LV_FLEX_FLOW_ROW);

    lv_obj_t *scann_btn = lv_btn_create(scann_btn_container);
    lv_obj_set_size(scann_btn, 70, 60);
    lv_obj_add_event_cb(scann_btn, handler_scan_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lb = lv_label_create(scann_btn);
    lv_label_set_text(lb, "Scan");
    lv_obj_set_style_text_color(lb, ZV_COLOR_TEXT_MAIN, 0);
    lv_obj_center(lb);

    /// --- filtros
    lv_obj_t *filter_container = lv_obj_create(root);
    lv_obj_set_size(filter_container, LV_PCT(100), 60);
    lv_obj_set_style_bg_opa(filter_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(filter_container, 0, 0);
    lv_obj_clear_flag(filter_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(filter_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(filter_container, LV_FLEX_FLOW_ROW);

    /// --- listado
    list = create_list(root, 100, 60);
    set_event_data(list, handler, NULL);

    return page;
}