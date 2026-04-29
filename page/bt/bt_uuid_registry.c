#include "bt_uuid_registry.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define BT_BASE_UUID_TAIL "-0000-1000-8000-00805f9b34fb"

static const uuid_name_t known_services[] = {
    {"0x1800", "GAP"},
    {"0x1801", "GATT"},
    {"0x180A", "Device Info"},
    {"0x180D", "Heart Rate"},
    {"0x180F", "Battery"},
    {"0x1812", "HID"},
    {"0x1816", "Cycling Speed"},
    {"0x181A", "Environmental"},
    {"0x181C", "User Data"},
    {"0xFE9F", "Apple Continuity"},
};

static const uuid_name_t known_chars[] = {
    {"0x2A00", "Device Name"},
    {"0x2A01", "Appearance"},
    {"0x2A19", "Battery Lvl"},
    {"0x2A24", "Model Number"},
    {"0x2A25", "Serial Number"},
    {"0x2A26", "Firmware Rev"},
    {"0x2A27", "Hardware Rev"},
    {"0x2A29", "Manufacturer"},
    {"0x2A37", "HR Meas."},
    {"0x2A38", "Body Loc."},
    {"0x2A4D", "HID Report"},
};

static int is_bt_base_uuid(const char *uuid)
{
    return strlen(uuid) == 36 &&
           strncasecmp(uuid, "0000", 4) == 0 &&
           strncasecmp(uuid + 8, BT_BASE_UUID_TAIL, 28) == 0;
}

static void extract_uuid16(const char *uuid, char *out, size_t size)
{
    snprintf(out, size, "0x%c%c%c%c",
             toupper((unsigned char)uuid[4]),
             toupper((unsigned char)uuid[5]),
             toupper((unsigned char)uuid[6]),
             toupper((unsigned char)uuid[7]));
}

const char *lookup_name(const char *uuid, char *normalized_uuid, size_t normalized_size)
{
    if (!uuid || !normalized_uuid || normalized_size == 0)
        return NULL;

    normalized_uuid[0] = '\0';

    if (is_bt_base_uuid(uuid)) {
        extract_uuid16(uuid, normalized_uuid, normalized_size);
    } else {
        snprintf(normalized_uuid, normalized_size, "%s", uuid);
    }

    size_t size = sizeof(known_services) / sizeof(known_services[0]);
    for (size_t i = 0; i < size; i++) {
        if (strcmp(normalized_uuid, known_services[i].uuid) == 0)
            return known_services[i].name;
    }

    size = sizeof(known_chars) / sizeof(known_chars[0]);
    for (size_t i = 0; i < size; i++) {
        if (strcmp(normalized_uuid, known_chars[i].uuid) == 0)
            return known_chars[i].name;
    }

    return NULL;
}
