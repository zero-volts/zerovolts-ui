#include "ir_controller.h"
#include "page/ir/ir_raw_helper.h"
#include "utils/string_utils.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define DEFUALT_LIST_AMOUNT 8
#define IR_LEARN_MAX_ATTEMPTS 3

typedef struct {
    ir_remote_list *list;
    size_t capacity;
    int has_error;
} remotes_handler_ctx;

typedef struct {
    ir_button_list *list;
    size_t capacity;
    int has_error;
} buttons_handler_ctx;

static ir_remote_ctx remote_context;

static void count_raw_file_cb(const file_desc *desc, void *obj)
{
    (void)desc;

    int *counter = (int *)obj;
    if (counter)
        (*counter)++;
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

static void handle_file_list(const file_desc *description, void *obj_target)
{
    remotes_handler_ctx *handler_ctx = (remotes_handler_ctx *)obj_target;
    if (!handler_ctx || !handler_ctx->list->remotes || !description)
        return;

    if (!description->is_dir)
        return;

    if (handler_ctx->list->count == handler_ctx->capacity) 
    {
        // we need to add more space to the list because we have more remotes than expected.

        size_t new_capacity = handler_ctx->capacity * 2;
        ir_remote_info *tmp_remotes = (ir_remote_info *)realloc(
            handler_ctx->list->remotes, new_capacity * sizeof(ir_remote_info));
        if (!tmp_remotes) 
        {
            handler_ctx->has_error = 1;
            return;
        }

        handler_ctx->list->remotes = tmp_remotes;
        handler_ctx->capacity = new_capacity;
    }

    int next_slot = handler_ctx->list->count;
    ir_remote_info *remote = &handler_ctx->list->remotes[next_slot];
    memset(remote, 0, sizeof(*remote));

    // getting the remote directory name
    strncpy(remote->name, description->file_name, sizeof(remote->name) - 1);
    remote->name[sizeof(remote->name) - 1] = '\0';

    char buttons_dir[PATH_MAX];
    if (snprintf(buttons_dir, sizeof(buttons_dir), "%s/buttons", description->file_path) <= 0 ||
        strnlen(buttons_dir, sizeof(buttons_dir)) >= sizeof(buttons_dir)) 
    {
        remote->button_count = 0;
        handler_ctx->list->count++;
        return;
    }

    if (!file_is_directory(buttons_dir)) {
        remote->button_count = 0;
        handler_ctx->list->count++;
        return;
    }

    int raw_counter = 0;
    ir_callback_event event;
    memset(&event, 0, sizeof(event));
    event.cb = count_raw_file_cb;
    event.data = &raw_counter;

    if (ir_list_raw_files_cb(buttons_dir, &event) != IR_OK) {
        remote->button_count = 0;
    } else {
        remote->button_count = raw_counter;
    }

    handler_ctx->list->count++;
}

static void handle_raw_filelist(const file_desc *description, void *obj_target)
{
    buttons_handler_ctx *handler_ctx = (buttons_handler_ctx *)obj_target;
    if (!handler_ctx || !handler_ctx->list || !handler_ctx->list->buttons || !description)
        return;

    if (!description->is_file)
        return;

    if (!file_has_extension(description->file_name, ".raw"))
        return;

    if (handler_ctx->list->count == handler_ctx->capacity)
    {
        size_t new_capacity = handler_ctx->capacity * 2;
        ir_button *tmp_buttons = (ir_button *)realloc(
            handler_ctx->list->buttons, new_capacity * sizeof(ir_button));

        if (!tmp_buttons)
        {
            handler_ctx->has_error = 1;
            return;
        }

        handler_ctx->list->buttons = tmp_buttons;
        handler_ctx->capacity = new_capacity;
    }

    size_t next_slot = handler_ctx->list->count;
    ir_button *button = &handler_ctx->list->buttons[next_slot];
    memset(button, 0, sizeof(*button));

    strncpy(button->name, description->file_name, sizeof(button->name) - 1);
    button->name[sizeof(button->name) - 1] = '\0';

    size_t len = strlen(button->name);
    if (len >= 4 && strcmp(button->name + len - 4, ".raw") == 0)
        button->name[len - 4] = '\0';

    handler_ctx->list->count++;
}

static ir_status_t create_remote_directory(const char *remote_name, char *out_sanitized_name,
    size_t out_sanitized_size, char *out_remote_directory, size_t out_remote_directory_size)
{
     if (!zv_sanitize_name(remote_name, out_sanitized_name, out_sanitized_size) ||
        zv_has_whitespace(remote_name)) {
        return IR_ERR_INVALID;
    }

    int ret = create_directory_path(remote_context.remotes_root, 
            out_sanitized_name, out_remote_directory, out_remote_directory_size);

    if (ret < 0 )
        return IR_ERR_INVALID;

    int created = file_ensure_dir_recursive(out_remote_directory);
    if (created != 0) 
        return IR_ERR_IO;

    return IR_OK;
}

static ir_status_t create_buttons_directory(const char *remote_directory,
    char *out_buttons_directory, size_t out_size)
{
    int ret = create_directory_path(remote_directory, "buttons",
        out_buttons_directory, out_size);
    if (ret < 0)
        return IR_ERR_INVALID;

    if (file_ensure_dir_recursive(out_buttons_directory) != 0)
        return IR_ERR_IO;

    return IR_OK;
}

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

ir_status_t ir_controller_create_remote(const char *remote_name)
{
    if (zv_is_empty(remote_name))
        return IR_ERR_INVALID;
    
    if (remote_context.remotes_root == NULL)
        return IR_ERR_CONFIG;

    char remote_directory[IR_MAX_NAME];
    char remote_name_sanitize[IR_MAX_NAME];

    // Making the remote control directory
    ir_status_t created = create_remote_directory(
        remote_name,
        remote_name_sanitize, sizeof(remote_name_sanitize),
        remote_directory, sizeof(remote_directory));
    if (created != 0) 
        return created;

    // Making the button directory
    char buttons_directory[IR_MAX_NAME];
    created = create_buttons_directory(remote_directory, buttons_directory, sizeof(buttons_directory));
    if (created != 0) 
        return created;

    // Making the metadata file
    char meta_file_path[IR_MAX_NAME];
    int ret = 0;
    ret = create_file_path(remote_directory, "meta.json", meta_file_path, sizeof(meta_file_path));
    if (ret < 0 ) {
        return IR_ERR_INVALID;
    }

    if (access(meta_file_path, F_OK) != 0) 
    {
        char meta_content[IR_MAX_NAME];
        int n = snprintf(meta_content, IR_MAX_NAME, "{\n  \"name\": \"%s\"\n}\n", remote_name_sanitize);

        if (n < 0 || (size_t)n >= sizeof(meta_content))
            return IR_ERR_IO;

        if (write_entire_file(meta_file_path, meta_content, (size_t)n) != 0)
            return IR_ERR_IO;
    }

    return IR_OK;
}

ir_status_t ir_controller_list_remotes(ir_remote_list *out_list)
{
    if (!out_list)
        return IR_ERR_INVALID;

    if (remote_context.remotes_root == NULL)
        return IR_ERR_CONFIG;
    
    memset(out_list, 0, sizeof(*out_list));
    out_list->remotes = (ir_remote_info *) calloc (DEFUALT_LIST_AMOUNT, sizeof(ir_remote_info));
    if (!out_list->remotes)
        return IR_ERR_IO;

    remotes_handler_ctx handler_ctx;
    memset(&handler_ctx, 0, sizeof(handler_ctx));
    handler_ctx.capacity = DEFUALT_LIST_AMOUNT;
    handler_ctx.list = out_list;

    get_file_list(remote_context.remotes_root, handle_file_list, &handler_ctx);
    if (handler_ctx.has_error != 0) 
    {
        free(out_list->remotes);
        out_list->remotes = NULL;
        out_list->count = 0;
        
        return IR_ERR_IO;
    }

    return IR_OK;
}

ir_status_t ir_controller_list_buttons(const char *remote_name, ir_button_list *out_list)
{
    if (!out_list)
        return IR_ERR_INVALID;

    if (zv_is_empty(remote_name))
        return IR_ERR_INVALID;

    if (remote_context.remotes_root == NULL)
        return IR_ERR_CONFIG;

    memset(out_list, 0, sizeof(*out_list));

    char remote_name_sanitize[IR_MAX_NAME];
    if (!zv_sanitize_name(remote_name, remote_name_sanitize, sizeof(remote_name_sanitize)) ||
        zv_has_whitespace(remote_name)) {
        return IR_ERR_INVALID;
    }

    char remote_directory[PATH_MAX];
    int ret = create_directory_path(remote_context.remotes_root, 
        remote_name_sanitize,
        remote_directory, 
        sizeof(remote_directory)
    );

    if (ret < 0) {
        return IR_ERR_INVALID;
    }

    char buttons_directory[PATH_MAX];
    ret = create_directory_path(remote_directory, "buttons", 
        buttons_directory, 
        sizeof(buttons_directory)
    );

    if (ret < 0)
        return IR_ERR_INVALID;

    out_list->buttons = (ir_button *)calloc(DEFUALT_LIST_AMOUNT, sizeof(ir_button));
    if (!out_list->buttons)
        return IR_ERR_IO;

    buttons_handler_ctx handler_ctx;
    memset(&handler_ctx, 0, sizeof(handler_ctx));
    handler_ctx.capacity = DEFUALT_LIST_AMOUNT;
    handler_ctx.list = out_list;

    ir_callback_event event;
    memset(&event, 0, sizeof(event));
    event.cb = handle_raw_filelist;
    event.data = &handler_ctx;

    if (ir_list_raw_files_cb(buttons_directory, &event) != IR_OK)
    {
        free(out_list->buttons);
        out_list->buttons = NULL;
        out_list->count = 0;
        return IR_ERR_IO;
    }

    if (handler_ctx.has_error != 0) 
    {
        free(out_list->buttons);
        out_list->buttons = NULL;
        out_list->count = 0;
        
        return IR_ERR_IO;
    }

    return IR_OK;
}

ir_status_t ir_controller_learn_button(const char *remote_name, const char *button_name)
{
    if (zv_is_empty(remote_name) || zv_is_empty(button_name))
        return IR_ERR_INVALID;

    if (remote_context.remotes_root == NULL)
        return IR_ERR_CONFIG;

    char remote_name_sanitize[IR_MAX_NAME];
    char button_name_sanitize[IR_MAX_NAME];
    if (!zv_sanitize_name(button_name, button_name_sanitize, sizeof(button_name_sanitize)) ||
        zv_has_whitespace(button_name)) {
        return IR_ERR_INVALID;
    }

    // checking that remote directory and buttons directory exists
    char remote_directory[PATH_MAX];
    ir_status_t created = create_remote_directory(
        remote_name,
        remote_name_sanitize, sizeof(remote_name_sanitize),
        remote_directory, sizeof(remote_directory));
    if (created != IR_OK)
        return created;

    char buttons_directory[PATH_MAX];
    created = create_buttons_directory(remote_directory, buttons_directory, sizeof(buttons_directory));
    if (created != IR_OK)
        return created;

    // making the name for the new .raw file, it will be "buttons_name.raw"
    char button_raw_name[IR_MAX_NAME];
    int n = snprintf(button_raw_name, sizeof(button_raw_name), "%s.raw", button_name_sanitize);
    if (n < 0 || (size_t)n >= sizeof(button_raw_name))
        return IR_ERR_INVALID;

    // The final path will be .../the_remote/buttons/the_button.raw
    char raw_path[PATH_MAX];
    int ret = create_file_path(buttons_directory, button_raw_name, raw_path, sizeof(raw_path));
    if (ret < 0)
        return IR_ERR_INVALID;

    ir_status_t rc = IR_ERR_IO;
    for (int attempt = 1; attempt <= IR_LEARN_MAX_ATTEMPTS; attempt++)
    {
        char invalid_path[PATH_MAX];
        int token_count = 0;
        int last_sign = 0;

        rc = ir_learn_raw(raw_path);
        if (rc != IR_OK)
            continue;

        if (ir_validate_raw_capture_file(raw_path, &token_count, &last_sign))
            return IR_OK;

        if (ir_append_synthetic_gap_for_odd_capture(raw_path, token_count, last_sign)) {
            int fixed_tokens = 0;
            int fixed_last_sign = 0;
            if (ir_validate_raw_capture_file(raw_path, &fixed_tokens, &fixed_last_sign))
                return IR_OK;
        }

        if (snprintf(invalid_path, sizeof(invalid_path), "%s.invalid%d", raw_path, attempt) > 0 &&
            strnlen(invalid_path, sizeof(invalid_path)) < sizeof(invalid_path)) {
            if (rename(raw_path, invalid_path) != 0) {
                (void)errno;
            }
        }
    }

    return rc;
}

ir_status_t ir_controller_send_button(const char *remote_name, const char *button_name)
{
    if (zv_is_empty(remote_name) || zv_is_empty(button_name))
        return IR_ERR_INVALID;

    if (remote_context.remotes_root == NULL)
        return IR_ERR_CONFIG;

    char remote_name_sanitize[IR_MAX_NAME];
    char button_name_sanitize[IR_MAX_NAME];
    if (!zv_sanitize_name(remote_name, remote_name_sanitize, sizeof(remote_name_sanitize)) ||
        zv_has_whitespace(remote_name)) {
        return IR_ERR_INVALID;
    }
    if (!zv_sanitize_name(button_name, button_name_sanitize, sizeof(button_name_sanitize)) ||
        zv_has_whitespace(button_name)) {
        return IR_ERR_INVALID;
    }

    char remote_directory[PATH_MAX];
    int ret = create_directory_path(remote_context.remotes_root,
        remote_name_sanitize, remote_directory, sizeof(remote_directory));
    if (ret < 0)
        return IR_ERR_INVALID;

    char buttons_directory[PATH_MAX];
    ret = create_directory_path(remote_directory, "buttons",
        buttons_directory, sizeof(buttons_directory));
    if (ret < 0)
        return IR_ERR_INVALID;

    char button_raw_name[IR_MAX_NAME];
    int n = snprintf(button_raw_name, sizeof(button_raw_name), "%s.raw", button_name_sanitize);
    if (n < 0 || (size_t)n >= sizeof(button_raw_name))
        return IR_ERR_INVALID;

    char raw_path[PATH_MAX];
    ret = create_file_path(buttons_directory, button_raw_name, raw_path, sizeof(raw_path));
    if (ret < 0)
        return IR_ERR_INVALID;

    if (access(raw_path, R_OK) != 0)
        return IR_ERR_IO;

    return ir_send_raw(raw_path);
}

const char *ir_controller_last_error(void)
{
    return ir_service_last_error();
}
