#include "hid.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>

#include "components/ui_theme.h"
#include "utils/file.h"
#include "config.h"


#define ZV_HID_DEFAULT_SCRIPTS_DIR  "/opt/zerovolts/hid-scripts"
#define ZV_HID_DEFAULT_SELECTED_PATH "/var/lib/zerovolts/hid-selected.txt"

typedef struct {
    hid_view *view;
    char *path;
} hid_item_ctx;

static int zv_hid_enable(void)
{
    system("sudo /home/zerovolts/git/zerovolts-ui/scripts/zv-hid-enable.sh");
    return 0;
}

static int zv_hid_disable(void)
{
    
    system("sudo /home/zerovolts/git/zerovolts-ui/scripts/zv-hid-disable.sh");
    return 0;
}

static int zv_hid_start_session(void)
{
    system("sudo systemctl start zv-hid-session.service");
    printf("iniciando servicio! \n");
    return 0;
}

static int zv_hid_stop_session(void)
{
    system("sudo systemctl stop zv-hid-session.service");
    printf("deteniendo servicio! \n");
    return 0;
}

/* ===================== PERSISTENCIA ===================== */


static void set_status(const char *msg, lv_color_t color, hid_view *self)
{
    if(!self->status) return;
    lv_label_set_text(self->status, msg);
    lv_obj_set_style_text_color(self->status, color, 0);
}

static void hid_set_selected_label(hid_view *self)
{
    if(!self->selected_lbl) 
        return;

    if(self->script.selected_script[0]) 
    {    
        const char *base = strrchr(self->script.selected_script, '/');
        base = base ? base + 1 : self->script.selected_script;
        lv_label_set_text_fmt(self->selected_lbl, "Selected: %s", base);
    } else {
        lv_label_set_text(self->selected_lbl, "Selected: (none)");
    }
}

static void clear_list(lv_obj_t *list)
{
    if(!list) 
        return;

    lv_obj_clean(list);
}

/* ===================== EVENTS ===================== */

static void hid_script_item_clicked(lv_event_t *e)
{
    hid_item_ctx *context = (hid_item_ctx *)lv_event_get_user_data(e);
    if(!context->path) 
        return;

    if (!context->view)
        return;

    snprintf(context->view->script.selected_script, sizeof(context->view->script.selected_script), "%s", context->path);
    hid_set_selected_label(context->view);
}


static void hid_btn_delete_cb(lv_event_t *e)
{
    hid_item_ctx *context = (hid_item_ctx *)lv_event_get_user_data(e);
    if (!context || !context->path)
        return;

    free(context->path);
    free(context);
}

static void add_file_to_list(const file_desc *description, void *self)
{
    if (!description || !description->is_file)
        return;

    hid_view *view = (hid_view *)self;

    lv_obj_t *btn = lv_list_add_button(view->list, NULL, description->file_name);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);

    lv_obj_set_style_text_color(btn, ZV_COLOR_TEXT_MAIN, 0);

    // path debe vivir más que la función => strdup
    char *heap_path = strdup(description->file_path);

    hid_item_ctx *ctx = (hid_item_ctx *)malloc(sizeof(*ctx));
    ctx->view = view;
    ctx->path = heap_path;

    lv_obj_add_event_cb(btn, hid_script_item_clicked, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(btn, hid_btn_delete_cb, LV_EVENT_DELETE, ctx);
}


static void refresh_list_impl(hid_view *self)
{
    clear_list(self->list);
    get_file_list(self->script.scripts_dir, add_file_to_list, self);
}

static void hid_refresh_btn_cb(lv_event_t *e)
{
    hid_view *self = (hid_view *)lv_event_get_user_data(e);
    refresh_list_impl(self);
}

static void hid_toggle_cb(lv_event_t *e)
{
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    hid_view *self = (hid_view *)lv_event_get_user_data(e);

    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (!on) 
    {
        zv_hid_stop_session();
        zv_hid_disable();

        config_set_hid_selected_script("");
        config_set_hid_enabled(false);

        self->script.hid_enabled = false;
        return;
    }

    if(!self->script.selected_script[0]) {
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        set_status("Select a script first.", lv_color_hex(0xFF5A5A), self);
        return;
    }

    if(zv_hid_enable() != 0) {
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        // set_status("Failed to enable HID.", lv_color_hex(0xFF5A5A));
        return;
    }

    if(zv_hid_start_session() != 0) {
        zv_hid_disable();
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        //set_status("Failed to start session.", lv_color_hex(0xFF5A5A));
        return;
    }

    config_set_hid_selected_script(self->script.selected_script);
    config_set_hid_enabled(true);
    self->script.hid_enabled = true;
}

void load_script_paths(hid_view *self, const zv_config *cfg)
{
    memset(&self->script, 0, sizeof(self->script));

    const char *scripts_dir =
        (cfg && cfg->hid.list_path[0]) ? cfg->hid.list_path : ZV_HID_DEFAULT_SCRIPTS_DIR;

    const char *selected_path =
        (cfg && strlen(cfg->hid.selected_file) > 0) ? cfg->hid.selected_file : ZV_HID_DEFAULT_SELECTED_PATH;

    snprintf(self->script.scripts_dir, sizeof(self->script.scripts_dir), "%s", scripts_dir);
    snprintf(self->script.selected_path, sizeof(self->script.selected_path), "%s", selected_path);
}

hid_view *hid_page_create(hid_view *self, lv_obj_t *menu, const zv_config *cfg)
{
    base_view *view = zv_view_create(&self->base, menu, "HID / BadUSB");
    if (!view)
        return NULL;

    lv_obj_set_style_pad_all(self->base.root, 8, 0);
    self->base.set_flex_layout(&self->base, LV_FLEX_FLOW_COLUMN, 5,0);
    
   load_script_paths(self, cfg);
    
    // HID enabled container
    lv_obj_t *enable_container = lv_obj_create(self->base.root);
    lv_obj_set_size(enable_container, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(enable_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(enable_container, 0, 0);
    lv_obj_set_style_pad_all(enable_container, 0, 0);
    lv_obj_set_layout(enable_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(enable_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(enable_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // label for switch
    self->status = lv_label_create(enable_container);
    lv_label_set_text(self->status, "Enable HID");
    lv_obj_set_style_text_color(self->status, ZV_COLOR_TEXT_MAIN, 0);

    self->toggle = lv_switch_create(enable_container);
    lv_obj_add_event_cb(self->toggle, hid_toggle_cb, LV_EVENT_VALUE_CHANGED, self);

    // center_container
    lv_obj_t *center_container = lv_obj_create(self->base.root);
    lv_obj_set_size(center_container, LV_PCT(100), 45);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(center_container, 0, 0);
    lv_obj_set_style_pad_all(center_container, 0, 0);
    lv_obj_set_layout(center_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(center_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(center_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    self->selected_lbl = lv_label_create(center_container);
    lv_obj_set_style_text_color(self->selected_lbl, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *refresh_btn = lv_btn_create(center_container);
    lv_obj_set_size(refresh_btn, 45, 35);
    lv_obj_set_style_bg_color(refresh_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(refresh_btn, 2, 0);
    lv_obj_set_style_border_color(refresh_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(refresh_btn, 10, 0);

    lv_obj_add_event_cb(refresh_btn, hid_refresh_btn_cb, LV_EVENT_CLICKED, self);
    lv_obj_set_flex_flow(refresh_btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(refresh_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(refresh_btn, 6, 0);

    lv_obj_t *ic = lv_label_create(refresh_btn);
    lv_label_set_text(ic, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(ic, ZV_COLOR_TEXT_MAIN, 0);

    // Lista (scrollable)
    self->list = lv_list_create(self->base.root);
    lv_obj_set_width(self->list, LV_PCT(100));

    lv_obj_set_style_bg_color(self->list, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_radius(self->list, 5, 0);
    lv_obj_set_style_pad_all(self->list, 5, 0);
    lv_obj_set_style_pad_row(self->list, 10, 0);

    lv_obj_set_scroll_dir(self->list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(self->list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(self->list, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(self->list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    
    //g_hid.selected_script = selected_path;
    
    hid_set_selected_label(self);

    // Refrescar scripts al crear
    //TODO: deberia llamarse desde afuera, no en el momento de creacion
    refresh_list_impl(self);

    lv_obj_set_state(self->toggle, LV_STATE_CHECKED, cfg->hid.is_enabled);
    self->script.hid_enabled = cfg->hid.is_enabled;

    return self;
}

lv_obj_t *hid_view_create(lv_obj_t *menu, const zv_config *cfg)
{
    hid_view *view = (hid_view *)malloc(sizeof(*view));
    if (!view)
        return NULL;

    hid_view *self = hid_page_create(view, menu, cfg);
    if (!self)
    {
        free(view);
        return NULL;
    }

    return self->base.page;
}

void hid_refresh_scripts(hid_view *self)
{
    refresh_list_impl(self);
}

const char *hid_get_selected_script(hid_view *self)
{
    return self->script.selected_script;
}
