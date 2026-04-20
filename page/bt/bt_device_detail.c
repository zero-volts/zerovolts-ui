#include "bt_device_detail.h"
#include "bt_controller.h"
#include "components/ui_info_panel.h"
#include "components/ui_theme.h"

#include <stdio.h>

lv_obj_t *bt_device_detail_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "BLE Device");
    
    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    bt_context_t *bt_ctx = bt_controller_ctx();
    const device_t *device = selected_device(bt_ctx);
    if (device != NULL)
    {
        ui_info_panel *device_info = create_info_panel(root, LV_PCT(100), LV_PCT(30));

        char rssi_buffer[16];
        snprintf(rssi_buffer, sizeof(rssi_buffer), "%d dBm", device->rssi);
        kv_item_t items[] = {
            {
                .label = "Name",
                .value = device->name
            },
            {
                .label = "MAC",
                .value = device->mac
            },
            {
                .label = "Manufacturer",
                .value = device->manufacturer
            },
            {
                .label = "RSSI",
                .value = rssi_buffer,
                .value_color = ZV_COLOR_TERMINAL,
                .has_value_color = true
            }
        };

        for (int i = 0; i < 4; i++) {
            add_info_panel_item(device_info, items[i]);
        }
        
    }

    return page;
}