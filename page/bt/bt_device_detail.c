#include "bt_device_detail.h"
#include "bt_controller.h"
#include "components/ui_info_panel.h"
#include "components/ui_theme.h"
#include "app_context.h"

#include <stdio.h>

static ui_info_panel *device_info = NULL;
static lv_obj_t *services_placeholder = NULL;

static lv_obj_t *create_section_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, text);
    lv_obj_set_style_text_color(title, ZV_COLOR_TERMINAL, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_border_color(title, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(title, 1, 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_bottom(title, 3, 0);
    return title;
}

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

    device_info = create_info_panel(root, LV_PCT(100), LV_PCT(30));

    create_section_title(root, "SERVICES");

    services_placeholder = lv_label_create(root);
    lv_label_set_text(services_placeholder, "No services enumerated");
    lv_obj_set_style_text_color(services_placeholder, ZV_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(services_placeholder, &lv_font_montserrat_10, 0);

    return page;
}

void bt_device_detail_refresh(void)
{
    if (device_info == NULL)
        return;

    clear_info_panel(device_info);

    const device_t *device = bt_context_get_selected();
    if (device == NULL)
        return;

    char rssi_buffer[16];
    snprintf(rssi_buffer, sizeof(rssi_buffer), "%d dBm", device->rssi);
    kv_item_t items[] = {
        { .label = "Name",         .value = device->name },
        { .label = "MAC",          .value = device->mac },
        { .label = "Manufacturer", .value = device->manufacturer },
        {
            .label = "RSSI",
            .value = rssi_buffer,
            .value_color = ZV_COLOR_TERMINAL,
            .has_value_color = true,
        },
    };

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++)
        add_info_panel_item(device_info, items[i]);
}
