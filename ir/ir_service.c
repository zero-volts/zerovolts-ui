#include "ir/ir_service.h"
#include "utils/file.h"
#include "utils/string_utils.h"

#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define IR_DEFAULT_TX_DEV "/dev/lirc0"
#define IR_DEFAULT_RX_DEV "/dev/lirc1"
#define IR_DEFAULT_TIMEOUT_MS 5000
#define IR_LEARN_MAX_ATTEMPTS 3
#define IR_RAW_MIN_TOKENS 160

typedef struct {
    ir_status_t (*learn_button)(const char *rx_dev, const char *out_raw_path, int timeout_ms);
    ir_status_t (*send_button)(const char *tx_dev, const char *raw_path);
} ir_backend_ops_t;

typedef struct {
    char tx_dev[256];
    char rx_dev[256];
    char remotes_root[512];
    int learn_timeout_ms;
    const ir_backend_ops_t *backend;
} ir_ctx_t;

static ir_ctx_t g_ir;
static char g_last_error[256];

static void ir_trace(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "[IR][service] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static void set_last_error(const char *msg)
{
    if (!msg) {
        g_last_error[0] = '\0';
        return;
    }

    snprintf(g_last_error, sizeof(g_last_error), "%s", msg);
}

const char *ir_service_last_error(void)
{
    return g_last_error;
}

static bool build_remote_dir(const char *remote_name, char *out, size_t out_sz)
{
    char safe_remote[IR_NAME_MAX];

    if (!zv_sanitize_name(remote_name, safe_remote, sizeof(safe_remote)))
        return false;

    return snprintf(out, out_sz, "%s/%s", g_ir.remotes_root, safe_remote) > 0;
}

static bool build_buttons_dir(const char *remote_name, char *out, size_t out_sz)
{
    char remote_dir[PATH_MAX];

    if (!build_remote_dir(remote_name, remote_dir, sizeof(remote_dir)))
        return false;

    return snprintf(out, out_sz, "%s/buttons", remote_dir) > 0;
}

static bool build_button_raw_path(const char *remote_name, const char *button_name, char *out, size_t out_sz)
{
    char safe_btn[IR_NAME_MAX];
    char buttons_dir[PATH_MAX];

    if (!zv_sanitize_name(button_name, safe_btn, sizeof(safe_btn)))
        return false;

    if (!build_buttons_dir(remote_name, buttons_dir, sizeof(buttons_dir)))
        return false;

    return snprintf(out, out_sz, "%s/%s.raw", buttons_dir, safe_btn) > 0;
}

typedef struct {
    int count;
} raw_count_ctx_t;

static void count_raw_item(const file_desc *desc, void *obj_target)
{
    raw_count_ctx_t *ctx = (raw_count_ctx_t *)obj_target;
    if (!ctx || !desc)
        return;
    if (!desc->is_file)
        return;
    if (!file_has_extension(desc->file_name, ".raw"))
        return;

    ctx->count++;
}

static int count_raw_files(const char *buttons_dir)
{
    raw_count_ctx_t ctx;

    if (!buttons_dir || !buttons_dir[0])
        return 0;
    if (!file_is_directory(buttons_dir))
        return 0;

    memset(&ctx, 0, sizeof(ctx));
    get_file_list(buttons_dir, count_raw_item, &ctx);
    return ctx.count;
}

static int cmp_remote_info(const void *a, const void *b)
{
    const ir_remote_info_t *ra = (const ir_remote_info_t *)a;
    const ir_remote_info_t *rb = (const ir_remote_info_t *)b;
    return strcmp(ra->name, rb->name);
}

static int cmp_button_info(const void *a, const void *b)
{
    const ir_button_info_t *ba = (const ir_button_info_t *)a;
    const ir_button_info_t *bb = (const ir_button_info_t *)b;
    return strcmp(ba->name, bb->name);
}

typedef struct {
    ir_remote_list_t *list;
    size_t cap;
    bool has_error;
} remotes_collect_ctx_t;

static void collect_remote_item(const file_desc *desc, void *obj_target)
{
    remotes_collect_ctx_t *ctx = (remotes_collect_ctx_t *)obj_target;
    char buttons_dir[PATH_MAX];
    ir_remote_info_t *slot;

    if (!ctx || !ctx->list || !desc || ctx->has_error)
        return;

    if (!desc->is_dir)
        return;

    if (ctx->list->count == ctx->cap) 
    {
        ir_remote_info_t *tmp;
        size_t new_cap = ctx->cap * 2;
        tmp = (ir_remote_info_t *)realloc(ctx->list->items, new_cap * sizeof(ir_remote_info_t));
        if (!tmp) {
            ctx->has_error = true;
            return;
        }
        ctx->list->items = tmp;
        ctx->cap = new_cap;
    }

    slot = &ctx->list->items[ctx->list->count];
    memset(slot, 0, sizeof(*slot));
    strncpy(slot->name, desc->file_name, sizeof(slot->name) - 1);
    slot->name[sizeof(slot->name) - 1] = '\0';

    if (snprintf(buttons_dir, sizeof(buttons_dir), "%s/buttons", desc->file_path) <= 0 ||
        strnlen(buttons_dir, sizeof(buttons_dir)) >= sizeof(buttons_dir)) {
        slot->button_count = 0;
        ctx->list->count++;
        return;
    }

    slot->button_count = count_raw_files(buttons_dir);
    ctx->list->count++;
}

typedef struct {
    ir_button_list_t *list;
    size_t cap;
    bool has_error;
} buttons_collect_ctx_t;

static void collect_button_item(const file_desc *desc, void *obj_target)
{
    buttons_collect_ctx_t *ctx = (buttons_collect_ctx_t *)obj_target;
    ir_button_info_t *slot;
    size_t len;

    if (!ctx || !ctx->list || !desc || ctx->has_error)
        return;
    if (!desc->is_file)
        return;
    if (!file_has_extension(desc->file_name, ".raw"))
        return;

    if (ctx->list->count == ctx->cap) {
        ir_button_info_t *tmp;
        size_t new_cap = ctx->cap * 2;
        tmp = (ir_button_info_t *)realloc(ctx->list->items, new_cap * sizeof(ir_button_info_t));
        if (!tmp) {
            ctx->has_error = true;
            return;
        }
        ctx->list->items = tmp;
        ctx->cap = new_cap;
    }

    slot = &ctx->list->items[ctx->list->count];
    memset(slot, 0, sizeof(*slot));
    strncpy(slot->name, desc->file_name, sizeof(slot->name) - 1);
    slot->name[sizeof(slot->name) - 1] = '\0';
    len = strlen(slot->name);
    if (len >= 4)
        slot->name[len - 4] = '\0';
    ctx->list->count++;
}

static void shell_escape_single_quotes(const char *src, char *dst, size_t dst_sz)
{
    size_t j = 0;

    if (!dst || dst_sz == 0)
        return;

    dst[0] = '\0';
    if (!src)
        return;

    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_sz; i++) {
        if (src[i] == '\'') {
            const char *esc = "'\\''";
            size_t need = strlen(esc);
            if (j + need >= dst_sz)
                break;
            memcpy(dst + j, esc, need);
            j += need;
        } else {
            dst[j++] = src[i];
        }
    }

    dst[j] = '\0';
}

static int system_status_code(int rc)
{
    if (rc == -1)
        return -1;
    if (WIFEXITED(rc))
        return WEXITSTATUS(rc);
    return -1;
}

static void normalize_dir_path(char *path)
{
    size_t len;

    if (!path)
        return;

    len = strlen(path);
    while (len > 1 && path[len - 1] == '/') {
        path[len - 1] = '\0';
        len--;
    }
}

static bool validate_raw_capture_file(const char *raw_path, int *token_count_out, int *last_sign_out)
{
    long sz = 0;
    int token_count = 0;
    int last_sign = 0;
    int first_sign = 0;

    if (token_count_out) 
        *token_count_out = 0;

    if (last_sign_out) 
        *last_sign_out = 0;

    char *buf = read_file_as_buffer(raw_path, &sz);
    if (!buf || sz <= 0) { 
        free(buf); 
        return false; 
    }

    char *p = buf;
    while (*p) 
    {
        char *end = NULL;
        long value;

        while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') p++;
        if (*p == '\0') 
            break;

        if (*p != '+' && *p != '-') { 
            free(buf); 
            return false; 
        }

        last_sign = (*p == '+') ? 1 : -1;
        if (token_count == 0) 
            first_sign = last_sign;

        p++;

        if (*p < '0' || *p > '9') { 
            free(buf); 
            return false; 
        }

        value = strtol(p, &end, 10);
        if (end == p || value <= 0) { 
            free(buf); 
            return false; 
        }

        token_count++;
        p = end;
    }

    free(buf);

    if (token_count_out) 
        *token_count_out = token_count;

    if (last_sign_out) 
        *last_sign_out = last_sign;

    if (token_count < IR_RAW_MIN_TOKENS) 
        return false;

    /* En formato raw “lista”, lo típico es empezar con pulse (+) */
    if (first_sign != 1) 
        return false;

    return true;
}

static bool append_synthetic_gap_for_odd_capture(const char *raw_path, int token_count, int last_sign)
{
    const char *gap = " -20000\n";
    char *buf;
    char *patched;
    long sz = 0;
    size_t gap_len = strlen(gap);
    size_t trimmed_len;
    int rc;

    if (!raw_path || raw_path[0] == '\0')
        return false;

    if (token_count < IR_RAW_MIN_TOKENS || (token_count % 2) == 0 || last_sign <= 0)
        return false;

    buf = read_file_as_buffer(raw_path, &sz);
    if (!buf || sz <= 0) {
        free(buf);
        return false;
    }

    trimmed_len = (size_t)sz;
    while (trimmed_len > 0 && isspace((unsigned char)buf[trimmed_len - 1]))
        trimmed_len--;

    patched = (char *)malloc(trimmed_len + gap_len + 1);
    if (!patched) {
        free(buf);
        return false;
    }

    memcpy(patched, buf, trimmed_len);
    memcpy(patched + trimmed_len, gap, gap_len);
    patched[trimmed_len + gap_len] = '\0';

    rc = write_entire_file(raw_path, patched, trimmed_len + gap_len);

    free(patched);
    free(buf);

    return rc == 0;
}

static ir_status_t irctl_learn_button(const char *rx_dev, const char *out_raw_path, int timeout_ms)
{
    char escaped_dev[512];
    char escaped_tmp[1024];
    char tmp_path[PATH_MAX];
    char cmd[2048];
    struct stat st;
    int sec = timeout_ms / 1000;
    int watchdog_sec;
    int exit_code;

    if (sec <= 0)
        sec = 1;

    watchdog_sec = sec + 2;

    if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", out_raw_path) <= 0) {
        set_last_error("Invalid output path");
        return IR_ERR_INVALID;
    }

    shell_escape_single_quotes(rx_dev, escaped_dev, sizeof(escaped_dev));
    shell_escape_single_quotes(tmp_path, escaped_tmp, sizeof(escaped_tmp));

    snprintf(cmd, sizeof(cmd),
             "timeout %ds ir-ctl -r -d '%s' > '%s'",
             watchdog_sec, escaped_dev, escaped_tmp);

             
    ir_trace("learn cmd: %s", cmd);

    exit_code = system_status_code(system(cmd));
    ir_trace("learn exit_code=%d rx_dev=%s out=%s", exit_code, rx_dev, out_raw_path);
    if (exit_code == 124) {
        /* timeout can still leave a valid capture in the output file */
        if (stat(tmp_path, &st) == 0 && st.st_size > 0) {
            if (rename(tmp_path, out_raw_path) != 0) {
                remove(tmp_path);
                set_last_error("Failed to store raw file");
                ir_trace("learn rename after-timeout failed src=%s dst=%s errno=%d(%s)",
                         tmp_path, out_raw_path, errno, strerror(errno));
                return IR_ERR_IO;
            }
            set_last_error(NULL);
            ir_trace("learn captured before timeout bytes=%ld path=%s", (long)st.st_size, out_raw_path);
            return IR_OK;
        }

        remove(tmp_path);
        set_last_error("Learn timed out");
        ir_trace("learn timeout after %ds", sec);
        return IR_ERR_TIMEOUT;
    }
    if (exit_code != 0) {
        remove(tmp_path);
        set_last_error("ir-ctl learn failed");
        ir_trace("learn command failed (exit=%d)", exit_code);
        return IR_ERR_IO;
    }

    if (stat(tmp_path, &st) != 0 || st.st_size <= 0) {
        remove(tmp_path);
        set_last_error("Empty signal captured");
        ir_trace("learn empty capture tmp=%s stat_errno=%d(%s)", tmp_path, errno, strerror(errno));
        return IR_ERR_IO;
    }

    if (rename(tmp_path, out_raw_path) != 0) {
        remove(tmp_path);
        set_last_error("Failed to store raw file");
        ir_trace("learn rename failed src=%s dst=%s errno=%d(%s)",
                 tmp_path, out_raw_path, errno, strerror(errno));
        return IR_ERR_IO;
    }

    set_last_error(NULL);
    ir_trace("learn stored bytes=%ld path=%s", (long)st.st_size, out_raw_path);
    return IR_OK;
}

static ir_status_t irctl_send_button(const char *tx_dev, const char *raw_path)
{
    char escaped_dev[512];
    char escaped_path[1024];
    char cmd[2048];
    int exit_code;
    const int send_timeout_sec = 3;

    shell_escape_single_quotes(tx_dev, escaped_dev, sizeof(escaped_dev));
    shell_escape_single_quotes(raw_path, escaped_path, sizeof(escaped_path));

    snprintf(cmd, sizeof(cmd),
             "timeout %ds ir-ctl -d '%s' -s '%s' >/dev/null 2>&1",
             send_timeout_sec, escaped_dev, escaped_path);

    ir_trace("send cmd: %s", cmd);

    exit_code = system_status_code(system(cmd));
    if (exit_code != 0) {
        set_last_error("ir-ctl send failed");
        ir_trace("send failed exit_code=%d tx_dev=%s raw=%s", exit_code, tx_dev, raw_path);
        return IR_ERR_IO;
    }

    set_last_error(NULL);
    ir_trace("send ok tx_dev=%s raw=%s", tx_dev, raw_path);
    return IR_OK;
}

static ir_status_t lircdev_learn_button(const char *rx_dev, const char *out_raw_path, int timeout_ms)
{
    (void)rx_dev;
    (void)out_raw_path;
    (void)timeout_ms;
    set_last_error("lircdev backend not implemented yet");
    return IR_ERR_UNSUPPORTED;
}

static ir_status_t lircdev_send_button(const char *tx_dev, const char *raw_path)
{
    (void)tx_dev;
    (void)raw_path;
    set_last_error("lircdev backend not implemented yet");
    return IR_ERR_UNSUPPORTED;
}

static const ir_backend_ops_t g_backend_irctl = {
    .learn_button = irctl_learn_button,
    .send_button = irctl_send_button,
};

static const ir_backend_ops_t g_backend_lircdev = {
    .learn_button = lircdev_learn_button,
    .send_button = lircdev_send_button,
};

ir_status_t ir_service_init(const ir_service_cfg_t *cfg)
{
    memset(&g_ir, 0, sizeof(g_ir));
    set_last_error(NULL);

    if (!cfg || !cfg->remotes_root || !cfg->remotes_root[0]) {
        set_last_error("Missing IR config");
        return IR_ERR_CONFIG;
    }

    snprintf(g_ir.tx_dev, sizeof(g_ir.tx_dev), "%s",
             (cfg->tx_dev && cfg->tx_dev[0]) ? cfg->tx_dev : IR_DEFAULT_TX_DEV);
    snprintf(g_ir.rx_dev, sizeof(g_ir.rx_dev), "%s",
             (cfg->rx_dev && cfg->rx_dev[0]) ? cfg->rx_dev : IR_DEFAULT_RX_DEV);
    snprintf(g_ir.remotes_root, sizeof(g_ir.remotes_root), "%s", cfg->remotes_root);
    normalize_dir_path(g_ir.remotes_root);

    g_ir.learn_timeout_ms = cfg->learn_timeout_ms > 0 ? cfg->learn_timeout_ms : IR_DEFAULT_TIMEOUT_MS;

    if (!cfg->backend || strcmp(cfg->backend, "irctl") == 0) {
        g_ir.backend = &g_backend_irctl;
    } 
    else if (strcmp(cfg->backend, "lircdev") == 0) {
        g_ir.backend = &g_backend_lircdev;
    } 
    else 
    {
        set_last_error("Unknown IR backend");
        return IR_ERR_CONFIG;
    }

    if (file_ensure_dir_recursive(g_ir.remotes_root) != 0) 
    {
        set_last_error("Cannot create remotes root");
        return IR_ERR_IO;
    }

    ir_trace("init backend=%s tx=%s rx=%s remotes_root=%s timeout_ms=%d",
             cfg->backend ? cfg->backend : "irctl",
             g_ir.tx_dev, g_ir.rx_dev, g_ir.remotes_root, g_ir.learn_timeout_ms);

    return IR_OK;
}

ir_status_t ir_service_list_remotes(ir_remote_list_t *out)
{
    remotes_collect_ctx_t ctx;

    if (!out)
        return IR_ERR_INVALID;

    memset(out, 0, sizeof(*out));

    out->items = (ir_remote_info_t *)calloc(8, sizeof(ir_remote_info_t));
    if (!out->items)
        return IR_ERR_IO;

    memset(&ctx, 0, sizeof(ctx));
    ctx.list = out;
    ctx.cap = 8;

    get_file_list(g_ir.remotes_root, collect_remote_item, &ctx);
    if (ctx.has_error) {
        ir_service_free_remotes(out);
        return IR_ERR_IO;
    }

    if (out->count > 1)
        qsort(out->items, out->count, sizeof(ir_remote_info_t), cmp_remote_info);

    return IR_OK;
}

void ir_service_free_remotes(ir_remote_list_t *list)
{
    if (!list)
        return;

    free(list->items);
    list->items = NULL;
    list->count = 0;
}

ir_status_t ir_service_create_remote(const char *remote_name)
{
    char safe_remote[IR_NAME_MAX];
    char remote_dir[PATH_MAX];
    char buttons_dir[PATH_MAX];
    char meta_path[PATH_MAX];
    

    if (!zv_sanitize_name(remote_name, safe_remote, sizeof(safe_remote)) ||
        zv_has_whitespace(remote_name)) {
        set_last_error("Remote name cannot contain spaces or new lines");
        return IR_ERR_INVALID;
    }

    if (!build_remote_dir(safe_remote, remote_dir, sizeof(remote_dir))) {
        set_last_error("Invalid remote name");
        return IR_ERR_INVALID;
    }

    if (!build_buttons_dir(safe_remote, buttons_dir, sizeof(buttons_dir))) {
        set_last_error("Invalid remote name");
        return IR_ERR_INVALID;
    }

    if (file_ensure_dir_recursive(remote_dir) != 0 ||
        file_ensure_dir_recursive(buttons_dir) != 0) {
        set_last_error("Cannot create remote folders");
        return IR_ERR_IO;
    }

    if (snprintf(meta_path, sizeof(meta_path), "%s/meta.json", remote_dir) <= 0 ||
        strnlen(meta_path, sizeof(meta_path)) >= sizeof(meta_path)) {
        set_last_error("Metadata path too long");
        return IR_ERR_IO;
    }

    if (access(meta_path, F_OK) != 0) 
    {
        FILE *meta = fopen(meta_path, "w");
        if (!meta) {
            set_last_error("Cannot create remote metadata");
            return IR_ERR_IO;
        }

        fprintf(meta, "{\n  \"name\": \"%s\"\n}\n", safe_remote);
        fclose(meta);
    }

    set_last_error(NULL);
    return IR_OK;
}

ir_status_t ir_service_list_buttons(const char *remote_name, ir_button_list_t *out)
{
    char buttons_dir[PATH_MAX];
    buttons_collect_ctx_t ctx;

    if (!out)
        return IR_ERR_INVALID;

    memset(out, 0, sizeof(*out));

    if (!build_buttons_dir(remote_name, buttons_dir, sizeof(buttons_dir)))
        return IR_ERR_INVALID;

    out->items = (ir_button_info_t *)calloc(8, sizeof(ir_button_info_t));
    if (!out->items)
        return IR_ERR_IO;

    memset(&ctx, 0, sizeof(ctx));
    ctx.list = out;
    ctx.cap = 8;

    get_file_list(buttons_dir, collect_button_item, &ctx);
    if (ctx.has_error) {
        ir_service_free_buttons(out);
        return IR_ERR_IO;
    }

    if (out->count > 1)
        qsort(out->items, out->count, sizeof(ir_button_info_t), cmp_button_info);

    return IR_OK;
}

void ir_service_free_buttons(ir_button_list_t *list)
{
    if (!list)
        return;

    free(list->items);
    list->items = NULL;
    list->count = 0;
}

ir_status_t ir_service_learn_button(const char *remote_name, const char *button_name)
{
    char buttons_dir[PATH_MAX];
    char raw_path[PATH_MAX];
    ir_status_t rc = IR_ERR_IO;

    if (!g_ir.backend || !g_ir.backend->learn_button)
        return IR_ERR_CONFIG;

    if (!build_buttons_dir(remote_name, buttons_dir, sizeof(buttons_dir)) ||
        !build_button_raw_path(remote_name, button_name, raw_path, sizeof(raw_path))) {
        set_last_error("Invalid remote/button name");
        return IR_ERR_INVALID;
    }

    if (file_ensure_dir_recursive(buttons_dir) != 0) {
        set_last_error("Cannot create buttons folder");
        return IR_ERR_IO;
    }

    ir_trace("learn request remote='%s' button='%s' rx=%s out=%s timeout_ms=%d",
             remote_name, button_name, g_ir.rx_dev, raw_path, g_ir.learn_timeout_ms);

    for (int attempt = 1; attempt <= IR_LEARN_MAX_ATTEMPTS; attempt++) 
    {
        int token_count = 0;
        int last_sign = 0;
        bool is_valid;
        char invalid_path[PATH_MAX];

        ir_trace("learn attempt %d/%d", attempt, IR_LEARN_MAX_ATTEMPTS);
        rc = g_ir.backend->learn_button(g_ir.rx_dev, raw_path, g_ir.learn_timeout_ms);
        if (rc != IR_OK) {
            ir_trace("learn attempt %d failed rc=%d err='%s'", attempt, (int)rc, ir_service_last_error());
            continue;
        }

        is_valid = validate_raw_capture_file(raw_path, &token_count, &last_sign);
        ir_trace("learn attempt %d validation tokens=%d valid=%d", attempt, token_count, is_valid ? 1 : 0);
        if (is_valid) {
            set_last_error(NULL);
            return IR_OK;
        }

        if (append_synthetic_gap_for_odd_capture(raw_path, token_count, last_sign)) {
            int fixed_tokens = 0;
            int fixed_last_sign = 0;
            bool fixed_ok;
            fixed_ok = validate_raw_capture_file(raw_path, &fixed_tokens, &fixed_last_sign);
            ir_trace("learn attempt %d repaired-with-gap tokens=%d valid=%d",
                     attempt, fixed_tokens, fixed_ok ? 1 : 0);
            if (fixed_ok) {
                set_last_error(NULL);
                return IR_OK;
            }
        }

        if (snprintf(invalid_path, sizeof(invalid_path), "%s.invalid%d", raw_path, attempt) > 0 &&
            strnlen(invalid_path, sizeof(invalid_path)) < sizeof(invalid_path)) {
            if (rename(raw_path, invalid_path) == 0) {
                ir_trace("learn invalid capture saved attempt=%d path=%s", attempt, invalid_path);
            } else {
                ir_trace("learn invalid capture rename failed src=%s dst=%s errno=%d(%s)",
                         raw_path, invalid_path, errno, strerror(errno));
            }
        } else {
            ir_trace("learn invalid capture path too long base=%s", raw_path);
        }

        set_last_error("Captured signal looks invalid, retrying...");
    }

    set_last_error("No fue posible guardar la nueva señal tras 3 intentos.");
    return rc;
}

ir_status_t ir_service_send_button(const char *remote_name, const char *button_name)
{
    char raw_path[PATH_MAX];

    if (!g_ir.backend || !g_ir.backend->send_button)
        return IR_ERR_CONFIG;

    if (!build_button_raw_path(remote_name, button_name, raw_path, sizeof(raw_path))) {
        set_last_error("Invalid remote/button name");
        return IR_ERR_INVALID;
    }

    if (access(raw_path, R_OK) != 0) {
        set_last_error("Raw button file not found");
        return IR_ERR_IO;
    }

    return g_ir.backend->send_button(g_ir.tx_dev, raw_path);
}
