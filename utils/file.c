#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>

#include "file.h"

#define FILE_PATH_LENGTH 512 

char *read_file_as_buffer(const char *path, long* file_size_out)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) 
    {
        printf("read_file_as_buffer::Error opening file.\n");
        return NULL;
    }

    // geeting the file size
    fseek(file, 0, SEEK_END);
    
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size == -1)
    {
        printf("read_file_as_buffer::Can't get the file size\n");
        fclose(file);
        return NULL;
    }

    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL)
    {
        printf("read_file_as_buffer::Can't allocate memory to read the file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    size_t bytes = fread(buffer, 1, file_size, file);
    if ((long)bytes != file_size) 
    {
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);

    buffer[file_size]= '\0';
    *file_size_out = file_size;

    return buffer;
}

int write_entire_file(const char *path, const char *data, size_t len)
{
    char tmp[600];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "wb");
    if(!f) 
        return -1;

    size_t n = fwrite(data, 1, len, f);
    fflush(f);
    fclose(f);

    if(n != len) 
    {
        remove(tmp);
        return -2;
    }

    if(rename(tmp, path) != 0) 
    {
        remove(tmp);
        return -3;
    }

    return 0;
}

void get_file_list(const char* directory_path, file_callback callback, void *obj_target)
{
    DIR *directory = opendir(directory_path);
    if (!directory) 
    {
        printf("get_file_list::Can't find the directory: %s\n", directory_path);
        return;
    }

    struct dirent *dir;
    while( (dir = readdir(directory)) != NULL)
    {
        if (dir->d_name[0] == '.')
            continue;

        char full_path[FILE_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, dir->d_name);
        if (callback)
        {
            file_desc description = {
                .file_name = dir->d_name,
                .file_path = full_path
            };
      
            callback(&description, obj_target);
        }
    }

    closedir(directory);
}

cJSON *read_json_file(const char *path)
{
    long size = 0;
    char *file_content = read_file_as_buffer(path, &size);

    // printf("file content: %s\n", file_content);

    cJSON *json = cJSON_Parse(file_content);
    if (json == NULL) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
            printf("Error: %s\n", error_ptr);

        cJSON_Delete(json);
        return NULL;
    }

    free(file_content);

    return json;
}

bool file_has_extension(const char *name, const char *ext)
{
    const char *dot = strrchr(name, '.');
    return dot && strcmp(dot, ext) == 0;
}

int file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

int get_executable_dir(char *out, size_t out_size)
{
    char exe_path[PATH_MAX];
    size_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if ((int)len == -1) {
        return -1;
    }

    exe_path[len] = '\0';

    char *dir = dirname(exe_path);
    if (!dir) 
        return -2;

    strncpy(out, dir, out_size);
    out[out_size - 1] = '\0';

    return 0;
}