#include "ir_controller.h"
#include "utils/string_utils.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

static ir_remote_ctx remote_context;

ir_status_t ir_controller_init(const ir_remote_ctx *remote_ctx)
{
    memset(&remote_context, 0, sizeof(ir_remote_ctx));
    if (!remote_ctx || !remote_ctx->remotes_root || !remote_ctx->remotes_root[0])
        return IR_ERR_CONFIG;

    remote_context.remotes_root = strdup(remote_ctx->remotes_root);
    if (!remote_context.remotes_root)
        return IR_ERR_IO;

    remote_context.ir_ctx = remote_ctx->ir_ctx;

    ir_status_t service_success = ir_service_init(&remote_context.ir_ctx);
    if (service_success != IR_OK) 
    {
        free(remote_context.remotes_root);
        remote_context.remotes_root = NULL;
        return service_success;
    }
    
    return IR_OK;
}

static int create_directory_path(const char *parent_directory, const char *new_directory, char *result_directory, size_t out_sz)
{
   int size = snprintf(result_directory, out_sz, "%s/%s", parent_directory, new_directory);
   if (size < 0 || (size_t)size >= out_sz)
        return -1;

    return 0;
}

static int create_file_path(const char *parent_directory, const char *file_name, char *result_file_path, size_t out_sz)
{
    int size = snprintf(result_file_path, out_sz, "%s/%s", parent_directory, file_name);
    if (size < 0 || (size_t)size >= out_sz)
        return -1;

    return 0;
}

ir_status_t ir_controller_create_remote(const char *remote_name)
{
    if (zv_is_empty(remote_name))
        return IR_ERR_INVALID;
    
    if (remote_context.remotes_root == NULL)
        return IR_ERR_CONFIG;

    char remote_directory[IR_MAX_NAME];
    char remote_name_sanitize[IR_MAX_NAME];

    if (!zv_sanitize_name(remote_name, remote_name_sanitize, sizeof(remote_name_sanitize)) ||
        zv_has_whitespace(remote_name)) {
        // set_last_error("Remote name cannot contain spaces or new lines");
        return IR_ERR_INVALID;
    }

    // Making the remote control directory
    int ret = create_directory_path(remote_context.remotes_root, 
            remote_name_sanitize, remote_directory, sizeof(remote_directory));
    if (ret < 0 ) {
        return IR_ERR_INVALID;
    }

    int created = file_ensure_dir_recursive(remote_directory);
    if (created != 0) 
        return IR_ERR_IO;

    // Making the button directory
    char buttons_directory[IR_MAX_NAME];
    ret = create_directory_path(remote_directory, "buttons", 
            buttons_directory, sizeof (buttons_directory));
    if (ret < 0 ) {
        return IR_ERR_INVALID;
    }

    created = file_ensure_dir_recursive(buttons_directory);
    if (created != 0) 
        return IR_ERR_IO;


    char meta_file_path[IR_MAX_NAME];
    // Making the metadata file
    ret = create_file_path(remote_directory, "meta.json", meta_file_path, sizeof(meta_file_path));
    if (ret < 0 ) {
        return IR_ERR_INVALID;
    }

    if (access(meta_file_path, F_OK) != 0) 
    {
        char meta_content[IR_NAME_MAX];
        int n = snprintf(meta_content, IR_NAME_MAX, "{\n  \"name\": \"%s\"\n}\n", remote_name_sanitize);

        if (n < 0 || (size_t)n >= sizeof(meta_content))
            return IR_ERR_IO;

        if (write_entire_file(meta_file_path, meta_content, (size_t)n) != 0)
            return IR_ERR_IO;
    }

    return IR_OK;
}

ir_status_t ir_controller_list_remotes(ir_remote_list *out)
{

}

ir_status_t ir_controller_list_buttons(const char *remote_name, ir_button_list *out)
{

}

ir_status_t ir_controller_learn_button(const char *remote_name, const char *button_name)
{

}

ir_status_t ir_controller_send_button(const char *remote_name, const char *button_name)
{

}

const char *ir_controller_last_error(void)
{

}