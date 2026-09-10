#include "app_context.h"
#include "utils/logger.h"

#include <string.h>

struct bt_context_t {
    device_t devices[BT_ALLOWED_MAX_DEVICES];
    int current_device_amount;
    device_t selected;
};

struct subghz_sessions {
    subghz_capture_session_t sessions[SUBGHZ_ALLOWED_MAX_SESSIONS];
    int current_sessions_amount;
};

static bt_context_t bt_ctx;
static subghz_sessions sessions_ctx;

static app_context_t ctx = {
    .bt = &bt_ctx,
    .sessions = &sessions_ctx
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

void bt_context_set_selected(const device_t *device)
{
    if (device == NULL)
    {
        memset(&ctx.bt->selected, 0, sizeof(ctx.bt->selected));
        return;
    }

    ctx.bt->selected = *device;
}

void bt_context_clear_devices(void)
{
    ctx.bt->current_device_amount = 0;
    memset(&ctx.bt->selected, 0, sizeof(ctx.bt->selected));
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

    if (bt->current_device_amount >= BT_ALLOWED_MAX_DEVICES)
    {
        log_info("Can't save more devices, is in the limit of %d", BT_ALLOWED_MAX_DEVICES);
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

static subghz_capture_session_t *subghz_find_session(const uint64_t session_id)
{
    subghz_sessions *session = ctx.sessions;
    for (int index = 0; index < session->current_sessions_amount; index++)
    {
        if (session->sessions[index].capture_id == session_id)
            return &session->sessions[index];
    }

    return NULL;
}

void subghz_add_session_chunk(const subghz_data_chunk_t chunk)
{
    if (chunk.capture_id == 0 || chunk.chunks == 0 || chunk.seq >= chunk.chunks ||
            chunk.count == 0 || chunk.count > SUBGHZ_DATA_CHUNK_MAX_VALUES)
    {
        log_info("Can't save more sessions, is in the limit of %d", SUBGHZ_ALLOWED_MAX_SESSIONS);
        return;
    }

    subghz_capture_session_t *session_found = subghz_find_session(chunk.capture_id);
    if (!session_found)
    {
        if (ctx.sessions->current_sessions_amount >= SUBGHZ_ALLOWED_MAX_SESSIONS)
        {
            log_warning("se excede la capacidad de sessiones %d ", SUBGHZ_ALLOWED_MAX_SESSIONS);
            return;
        }

        session_found = &ctx.sessions->sessions[ctx.sessions->current_sessions_amount++];
        memset(session_found, 0, sizeof(*session_found));

        session_found->capture_id = chunk.capture_id;
        session_found->expected_chunks = chunk.chunks;    
    }

    if (session_found->next_seq != chunk.seq)
    {
        log_warning("no es la misma cantidad de secuencia, saliendo");
        return;   
    }

    if (session_found->expected_chunks != chunk.chunks)
    {
        log_warning("no es la misma cantidad de chunks, saliendo");
        return;   
    }

    if (chunk.count > SUBGHZ_CAPTURE_MAX_TIMINGS - session_found->count)
    {
        log_warning("Signal exceeds session capacity");
        return;
    }

    uint16_t base = session_found->count;
    for (uint16_t x = 0; x < chunk.count; x++)
    {
        session_found->timings[base + x] = chunk.timings[x];
    }

    session_found->count += chunk.count;
    session_found->next_seq++;
    session_found->received_chunks++;
}

bool subghz_set_session_completed(uint64_t session_id)
{
    subghz_capture_session_t *session_found = subghz_find_session(session_id);
    if (session_found == NULL)
        return false;

    if (session_found->expected_chunks != session_found->received_chunks)
        return false;

    if (session_found->next_seq != session_found->expected_chunks)
        return false;

    session_found->completed = true;

    return true;
}

subghz_capture_session_t *subghz_get_session_by(uint64_t session_id)
{
    subghz_capture_session_t *session_found = subghz_find_session(session_id);
    if (session_found == NULL)
        return NULL;

    return session_found;
}