#include "subghz_file.h"

#include <stdlib.h>
#include <inttypes.h>

#include "utils/file.h"
#include "utils/logger.h"
#include "utils/string_utils.h"

#define SUBGHZ_MAX_PATH_LENGTH  256


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

// 3.- Crear el archivo de la señal capturada con identificador pasado mas timestamp "capture_id_timestamp.sub"
// https://developer.flipper.net/flipperzero/doxygen/subghz_file_format.html
// Filetype: Flipper SubGhz RAW File
// Version: 1
// Frequency: 433920000
// Preset: FuriHalSubGhzPresetOok650Async
// Protocol: RAW
// RAW_Data: 29262 361 -68 2635 -66 24113 -66 11 ...
// RAW_Data: -424 205 -412 159 -412 381 -240 181 ...
// RAW_Data: -1448 361 -17056 131 -134 233 -1462 131 -166 953 -100 ...
bool subghz_file_create(const char *file_name_path, subghz_capture_session_t *session)
{
    size_t buffer_size = session->count + SUBGHZ_CAPTURE_MAX_TIMINGS;
    char *buffer_data = (char *)malloc(buffer_size);

    int offset = 0;
    int written = snprintf(buffer_data, buffer_size,
            "Filetype: Flipper SubGhz Key File\n"
            "Version: 1\n"
            "Frequency: %" PRIu32"\n"
            "Preset: \n"
            "Protocol: RAW\n", session->frequency);

    offset += written;
    written = snprintf(buffer_data + offset, buffer_size, "RAW_Data: ");
    offset += written;

    for (int  i = 0; i < 512; i++)
    {
        if (written >= buffer_size)
        {
            log_warning("llego al maximo");
            break;
        }
        written = snprintf(buffer_data + offset, buffer_size - offset, " %" PRIi32 " ",  session->timings[i]);
        offset += written;

    }

    write_entire_file(file_name_path, buffer_data, (size_t)offset);

    free(buffer_data);
    return true;
}