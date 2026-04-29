#include "bt_device_detail.h"
#include "bt_controller.h"
#include "components/ui_info_panel.h"
#include "components/ui_loading_btn.h"
#include "components/ui_pills.h"
#include "components/ui_theme.h"
#include "bt_uuid_registry.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define BT_BASE_UUID_TAIL "-0000-1000-8000-00805f9b34fb"

static lv_obj_t *page_ref = NULL;
static bool is_active = false;

typedef struct {
    ui_info_panel *device_info;

    lv_obj_t *services_container;
    lv_obj_t *services_placeholder;

    lv_obj_t *status_label;
    ui_loading_button *connect_btn;
} view_ctx;

static view_ctx own_ctx;

static const char *status_text(bt_conn_status_t s)
{
    switch (s) {
        case BT_CONN_IDLE:         return "Idle";
        case BT_CONN_CONNECTING:   return "Connecting...";
        case BT_CONN_CONNECTED:    return "Connected";
        case BT_CONN_DISCOVERING:  return "Discovering...";
        case BT_CONN_READY:        return "Ready";
        case BT_CONN_FAILED:       return "Failed";
        case BT_CONN_LOST:         return "Lost";
        case BT_CONN_DISCONNECTED: return "Disconnected";
        default:                   return "?";
    }
}

// props bitmask = ESP_GATT_CHAR_PROP_BIT_*: READ=0x02, WRITE_NR=0x04,
// WRITE=0x08, NOTIFY=0x10, INDICATE=0x20.
static void add_property_pills(ui_pills *pills, unsigned int props)
{
    if (props & 0x10) pills_add(pills, "NOTIFY");
    if (props & 0x20) pills_add(pills, "INDICATE");
    if (props & 0x02) pills_add(pills, "READ");
    if (props & 0x08) pills_add(pills, "WRITE");
    if (props & 0x04) pills_add(pills, "WRITE NR");
}

static void render_service(lv_obj_t *parent, const bt_service_t *svc)
{
    ui_info_panel *panel = create_info_panel(parent, LV_PCT(100), LV_SIZE_CONTENT);

    char buf[BT_UUID_STR_LEN];
    const char *name = lookup_name(svc->uuid, buf, sizeof(buf));

    char header_text[80];
    if (name)
        snprintf(header_text, sizeof(header_text), "%s - %s", buf, name);
    else
        snprintf(header_text, sizeof(header_text), "%s", buf);

    add_info_panel_header(panel, header_text, ZV_COLOR_ACCENT);

    for (int j = 0; j < svc->chars_count; j++)
    {
        const bt_characteristic_t *ch = &svc->chars[j];

        char ch_buf[BT_UUID_STR_LEN];
    const char *ch_name = lookup_name(ch->uuid, ch_buf, sizeof(ch_buf));

        char ch_label[80];
        if (ch_name)
            snprintf(ch_label, sizeof(ch_label), "%s  %s", ch_buf, ch_name);
        else
            snprintf(ch_label, sizeof(ch_label), "%s", ch_buf);

        lv_obj_t *right = add_info_panel_custom_row(panel, ch_label);
        if (right)
        {
            ui_pills *prop_pills = create_pills_sized(right, LV_SIZE_CONTENT, 30);
            add_property_pills(prop_pills, ch->props);
        }
    }
}

static void rebuild_services_list(void)
{
    if (!own_ctx.services_container)
        return;

    lv_obj_clean(own_ctx.services_container);

    bt_service_t *svcs = bt_get_services();
    int len = bt_get_services_length();

    if (len == 0)
    {
        if (own_ctx.services_placeholder)
            lv_obj_clear_flag(own_ctx.services_placeholder, LV_OBJ_FLAG_HIDDEN);

        return;
    }

    if (own_ctx.services_placeholder)
        lv_obj_add_flag(own_ctx.services_placeholder, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < len; i++) {
        render_service(own_ctx.services_container, &svcs[i]);
    }
}

static void update_btn_state(bt_conn_status_t s)
{
    if (!own_ctx.connect_btn)
        return;

    bool loading = (s == BT_CONN_CONNECTING || s == BT_CONN_DISCOVERING);
    loading_button_set_loading(own_ctx.connect_btn, loading);

    if (s == BT_CONN_CONNECTED || s == BT_CONN_READY) {
        loading_button_set_text(own_ctx.connect_btn, "Disconnect");
    } else {
        loading_button_set_text(own_ctx.connect_btn, "Connect");
    }
}

static void on_conn_event(bt_conn_status_t status, const char *info)
{
    if (!is_active)
        return;

    if (own_ctx.status_label)
        lv_label_set_text(own_ctx.status_label, status_text(status));

    update_btn_state(status);

    // Rebuild solo cuando termina el discovery, asi no parpadea por cada char.
    if (status == BT_CONN_READY)
        rebuild_services_list();
}

static void on_connect_click(lv_event_t *e)
{
    (void)e;

    bt_conn_status_t s = bt_get_conn_status();
    if (s == BT_CONN_CONNECTED || s == BT_CONN_READY) {
        bt_disconnect();
        return;
    }

    const device_t *dev = bt_controller_get_selected();
    if (!dev) {
        if (own_ctx.status_label) lv_label_set_text(own_ctx.status_label, "No device selected");
        return;
    }

    bt_connect(dev);
}

static void on_page_changed(lv_event_t *e)
{
    lv_obj_t *menu = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *cur = lv_menu_get_cur_main_page(menu);

    if (cur == page_ref && !is_active) {
        is_active = true;
        return;
    }

    if (cur != page_ref && is_active) {
        is_active = false;

        bt_conn_status_t s = bt_get_conn_status();
        if (s == BT_CONN_CONNECTED || s == BT_CONN_READY ||
            s == BT_CONN_CONNECTING || s == BT_CONN_DISCOVERING) {
            bt_disconnect();
        }

        bt_clear_discovery();
        if (own_ctx.services_container) lv_obj_clean(own_ctx.services_container);
        if (own_ctx.services_placeholder)
            lv_obj_clear_flag(own_ctx.services_placeholder, LV_OBJ_FLAG_HIDDEN);
        if (own_ctx.status_label) lv_label_set_text(own_ctx.status_label, status_text(BT_CONN_IDLE));
        update_btn_state(BT_CONN_IDLE);
    }
}

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
    page_ref = page;

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_add_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(root, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    own_ctx.device_info = create_info_panel(root, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *action_row = lv_obj_create(root);
    lv_obj_set_size(action_row, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(action_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(action_row, 0, 0);
    lv_obj_clear_flag(action_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(action_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(action_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(action_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    own_ctx.status_label = lv_label_create(action_row);
    lv_label_set_text(own_ctx.status_label, status_text(BT_CONN_IDLE));
    lv_obj_set_style_text_color(own_ctx.status_label, ZV_COLOR_TERMINAL, 0);

    own_ctx.connect_btn = create_loading_btn(action_row, 100, 40, "Connect");
    loading_set_event_cb(own_ctx.connect_btn, on_connect_click, own_ctx.connect_btn);

    create_section_title(root, "SERVICES");

    own_ctx.services_placeholder = lv_label_create(root);
    lv_label_set_text(own_ctx.services_placeholder, "No services enumerated");
    lv_obj_set_style_text_color(own_ctx.services_placeholder, ZV_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(own_ctx.services_placeholder, &lv_font_montserrat_10, 0);


    own_ctx.services_container = lv_obj_create(root);
    lv_obj_remove_style_all(own_ctx.services_container);
    lv_obj_set_width(own_ctx.services_container, LV_PCT(100));
    lv_obj_set_height(own_ctx.services_container, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(own_ctx.services_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(own_ctx.services_container, 0, 0);
    lv_obj_set_style_pad_row(own_ctx.services_container, 8, 0);
    lv_obj_clear_flag(own_ctx.services_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(own_ctx.services_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(own_ctx.services_container, LV_FLEX_FLOW_COLUMN);

    set_conn_cb(on_conn_event);
    lv_obj_add_event_cb(menu, on_page_changed, LV_EVENT_VALUE_CHANGED, NULL);

    return page;
}

void bt_device_detail_refresh(void)
{
    if (own_ctx.device_info == NULL)
        return;

    clear_info_panel(own_ctx.device_info);

    const device_t *device = bt_controller_get_selected();
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
        add_info_panel_item(own_ctx.device_info, items[i]);

    if (own_ctx.services_container) 
        lv_obj_clean(own_ctx.services_container);

    bt_clear_discovery();

    if (own_ctx.services_placeholder)
        lv_obj_clear_flag(own_ctx.services_placeholder, LV_OBJ_FLAG_HIDDEN);

    if (own_ctx.status_label) 
        lv_label_set_text(own_ctx.status_label, status_text(BT_CONN_IDLE));

    update_btn_state(BT_CONN_IDLE);
}
