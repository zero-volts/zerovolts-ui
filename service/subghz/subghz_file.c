#include "subghz_file.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "utils/file.h"
#include "utils/logger.h"

#define RAW_LINE_PER_LINE 512

// TODO: mejorar nombres de estructura y arreglo!
typedef struct {
    subghz_flipper_preset_t preset;
    const char *preset_name;
} subghz_flipper_preset;

static const subghz_flipper_preset flipper_presets[] = {
    {FuriHalSubGhzPresetOok270Async, "FuriHalSubGhzPresetOok270Async" },
    {FuriHalSubGhzPresetOok650Async, "FuriHalSubGhzPresetOok650Async" },
    {FuriHalSubGhzPreset2FSKDev238Async, "FuriHalSubGhzPreset2FSKDev238Async" },
    {FuriHalSubGhzPreset2FSKDev12KAsync, "FuriHalSubGhzPreset2FSKDev12KAsync" },
    {FuriHalSubGhzPreset2FSKDev476Async, "FuriHalSubGhzPreset2FSKDev476Async" }
};

// 1.- Validar que existe directorio para guardar las señales
bool subghz_file_create_root_path(const char *path)
{
    if (!path)
        return false;

    int created = file_ensure_dir_recursive(path);
    if (created != 0)
        return false;

    return true;
}

// RAW format: https://developer.flipper.net/flipperzero/doxygen/subghz_file_format.html
bool subghz_file_create(const char *file_name_path, subghz_capture_session_t *session, subghz_flipper_preset_t preset)
{
    if (!file_name_path || !file_name_path[0] || !session ||
        !session->completed || !session->frequency || session->count == 0 ||
        session->count > SUBGHZ_CAPTURE_MAX_TIMINGS)
        return false;

    char *preset_str = NULL;
    for (size_t i = 0; i < sizeof(flipper_presets) / sizeof(flipper_presets[0]); i++)
    {
        if (preset == flipper_presets[i].preset) 
        {
            preset_str = flipper_presets[i].preset_name;
            break;
        }
    }

    if (preset_str == NULL)
        return false;

    // Each signed int32 needs at most 11 characters plus a separator.
    const size_t buffer_size = 256 + (size_t)session->count * 12;
    char *buffer_data = (char *)malloc(buffer_size);
    if (!buffer_data)
        return false;

    int written = snprintf(buffer_data, buffer_size,
            "Filetype: Flipper SubGhz RAW File\n"
            "Version: 1\n"
            "Frequency: %" PRIu32 "\n"
            "Preset: %s\n"
            "Protocol: RAW\n"
            "RAW_Data:", session->frequency, preset_str);

    if (written < 0 || (size_t)written >= buffer_size)
    {
        free(buffer_data);
        return false;
    }

    size_t offset = (size_t)written;
    for (uint16_t i = 0; i < session->count; i++)
    {
        if (i == 0 && session->timings[i] < 0) 
        {
            log_warning("subghz_file::subghz_file_create The first timing can't be negative!");
            continue;
        }

        if ( i > 0 && i % RAW_LINE_PER_LINE == 0)
        {
            written = snprintf(buffer_data + offset, buffer_size - offset, "\nRAW_Data:");
            if (written < 0 || (size_t)written >= buffer_size - offset)
            {
                free(buffer_data);
                return false;
            }
            
            offset += (size_t)written;
        }

        written = snprintf(buffer_data + offset, buffer_size - offset, " %" PRId32, session->timings[i]);
        if (written < 0 || (size_t)written >= buffer_size - offset)
        {
            free(buffer_data);
            return false;
        }

        offset += (size_t)written;
    }

    buffer_data[offset++] = '\n';

    bool saved = write_entire_file(file_name_path, buffer_data, offset) == 0;
    free(buffer_data);

    return saved;
}
