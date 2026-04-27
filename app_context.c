#include "app_context.h"
#include "utils/logger.h"

#include <string.h>

struct bt_context_t {
    device_t devices[BT_ALLOWD_MAX_DEVICES];
    int current_device_amount;
    device_t selected;
};

static bt_context_t bt_ctx;
static app_context_t ctx = {
    .bt = &bt_ctx
};

app_context_t *app_context_get(void)
{
    return &ctx;
}

static device_t *bt_find_device(const char *mac)
{
    if (mac == NULL || mac[0] == '\0')
        return NULL;

    bt_context_t *bt = ctx.bt;
    for (int index = 0; index < bt->current_device_amount; index++)
    {
        if (strcmp(bt->devices[index].mac, mac) == 0)
            return &bt->devices[index];
    }

    return NULL;
}

int bt_context_devices_length(void)
{
    return ctx.bt->current_device_amount;
}

device_t *bt_context_get_devices(void)
{
    return ctx.bt->devices;
}

const device_t *bt_context_get_selected(void)
{
    if (ctx.bt->selected.mac[0] == '\0')
        return NULL;

    return &ctx.bt->selected;
}

void bt_context_set_selected(device_t *device)
{
    if (device == NULL)
    {
        memset(&ctx.bt->selected, 0, sizeof(ctx.bt->selected));
        return;
    }

    ctx.bt->selected = *device;
}

void bt_context_add_device(device_t *device)
{
    if (device == NULL)
        return;

    if (device->mac[0] == '\0')
        return;

    bt_context_t *bt = ctx.bt;
    device_t *dev_found = bt_find_device(device->mac);
    if (dev_found)
    {
        dev_found->rssi = device->rssi;
        dev_found->connectable = device->connectable;

        if (device->name[0] && strcmp(dev_found->name, UNKNOWN_NAME) == 0)
            snprintf(dev_found->name, sizeof(dev_found->name), "%s", device->name);

        if (device->manufacturer[0] && strcmp(dev_found->manufacturer, UNKNOWN_NAME) == 0)
            snprintf(dev_found->manufacturer, sizeof(dev_found->manufacturer), "%s", device->manufacturer);

        if (device->service[0] && strcmp(dev_found->service, UNKNOWN_NAME) == 0)
            snprintf(dev_found->service, sizeof(dev_found->service), "%s", device->service);

        if (device->appearance[0] && strcmp(dev_found->appearance, UNKNOWN_NAME) == 0)
            snprintf(dev_found->appearance, sizeof(dev_found->appearance), "%s", device->appearance);

        return;
    }

    if (bt->current_device_amount >= BT_ALLOWD_MAX_DEVICES)
    {
        log_info("Can't save more devices, is in the limit of %d", BT_ALLOWD_MAX_DEVICES);
        return;
    }

    dev_found = &bt->devices[bt->current_device_amount++];

    dev_found->rssi = device->rssi;
    dev_found->connectable = device->connectable;

    snprintf(dev_found->name, sizeof(dev_found->name), "%s",
             device->name[0] ? device->name : UNKNOWN_NAME);

    snprintf(dev_found->mac, sizeof(dev_found->mac), "%s",
             device->mac);

    snprintf(dev_found->manufacturer, sizeof(dev_found->manufacturer), "%s",
             device->manufacturer[0] ? device->manufacturer : UNKNOWN_NAME);

    snprintf(dev_found->service, sizeof(dev_found->service), "%s",
             device->service[0] ? device->service : UNKNOWN_NAME);

    snprintf(dev_found->appearance, sizeof(dev_found->appearance), "%s",
             device->appearance[0] ? device->appearance : UNKNOWN_NAME);
}