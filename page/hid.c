#include "hid.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>

#include "components/ui_theme.h"
#include "components/nav.h"
#include "utils/file.h"
#include "config.h"


#define ZV_HID_DEFAULT_SCRIPTS_DIR  "/opt/zerovolts/hid-scripts"
#define ZV_HID_DEFAULT_SELECTED_PATH "/var/lib/zerovolts/hid-selected.txt"

typedef struct {
    lv_obj_t *page;
    lv_obj_t *root;

    lv_obj_t *list;
    lv_obj_t *status;
    lv_obj_t *toggle;
    lv_obj_t *selected_lbl;

    char scripts_dir[512];
    char selected_path[512];

    char selected_script[512];
    bool hid_enabled;
} hid_ui_t;

static hid_ui_t g_hid;


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


static void set_status(const char *msg, lv_color_t color)
{
    if(!g_hid.status) return;
    lv_label_set_text(g_hid.status, msg);
    lv_obj_set_style_text_color(g_hid.status, color, 0);
}

static void hid_set_selected_label(void)
{
    if(!g_hid.selected_lbl) 
        return;

    if(g_hid.selected_script[0]) 
    {    
        const char *base = strrchr(g_hid.selected_script, '/');
        base = base ? base + 1 : g_hid.selected_script;
        lv_label_set_text_fmt(g_hid.selected_lbl, "Selected: %s", base);
    } else {
        lv_label_set_text(g_hid.selected_lbl, "Selected: (none)");
    }
}

static void clear_list(void)
{
    if(!g_hid.list) return;
    lv_obj_clean(g_hid.list);
}

/* ===================== EVENTS ===================== */

static void hid_script_item_clicked(lv_event_t *e)
{
    const char *path = (const char *)lv_event_get_user_data(e);
    if(!path) 
        return;

    snprintf(g_hid.selected_script, sizeof(g_hid.selected_script), "%s", path);
    hid_set_selected_label();
    
    //set_status("Script selected. Enable HID when ready.", ZV_COLOR_TEXT_MAIN);
}


static void hid_btn_delete_cb(lv_event_t *e)
{
    void *ud = lv_event_get_user_data(e);
    if(ud) free(ud);
}

static void add_file_to_list(const file_desc *description, void *list)
{
    lv_obj_t *btn = lv_list_add_button(g_hid.list, NULL, description->file_name);
    lv_obj_set_style_bg_color(btn, ZV_COLOR_BG_PANEL, 0);

    lv_obj_set_style_text_color(btn, ZV_COLOR_TEXT_MAIN, 0);
    zv_nav_add(btn);

    // path debe vivir más que la función => strdup
    char *heap_path = strdup(description->file_path);
    lv_obj_add_event_cb(btn, hid_script_item_clicked, LV_EVENT_CLICKED, heap_path);
    lv_obj_add_event_cb(btn, hid_btn_delete_cb, LV_EVENT_DELETE, heap_path);
}


static void refresh_list_impl(void)
{
    clear_list();
    get_file_list(g_hid.scripts_dir, add_file_to_list, g_hid.list);
}

static void hid_refresh_btn_cb(lv_event_t *e)
{
    (void)e;
    refresh_list_impl();
}

static void hid_toggle_cb(lv_event_t *e)
{
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (!on) 
    {
        zv_hid_stop_session();
        zv_hid_disable();

        config_set_hid_selected_script("");
        config_set_hid_enabled(false);

        g_hid.hid_enabled = false;
        return;
    }

    
    if(!g_hid.selected_script[0]) {
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
        set_status("Select a script first.", lv_color_hex(0xFF5A5A));
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

    config_set_hid_selected_script(g_hid.selected_script);
    config_set_hid_enabled(true);
    g_hid.hid_enabled = true;
}

lv_obj_t *hid_page_create(lv_obj_t *menu, const zv_config *cfg)
{
    memset(&g_hid, 0, sizeof(g_hid));

    const char *scripts_dir =
        (cfg && cfg->hid.list_path[0]) ? cfg->hid.list_path : ZV_HID_DEFAULT_SCRIPTS_DIR;
    const char *selected_path =
        (cfg && strlen(cfg->hid.selected_file) > 0) ? cfg->hid.selected_file : ZV_HID_DEFAULT_SELECTED_PATH;

    snprintf(g_hid.scripts_dir, sizeof(g_hid.scripts_dir), "%s", scripts_dir);
    snprintf(g_hid.selected_path, sizeof(g_hid.selected_path), "%s", selected_path);
    
    //lv_label_set_text(g_hid.selected_lbl, selected_path);

    g_hid.page = lv_menu_page_create(menu, "HID / BadUSB");
    lv_obj_set_scrollbar_mode(g_hid.page, LV_SCROLLBAR_MODE_OFF); 

    g_hid.root = lv_obj_create(g_hid.page);
    lv_obj_set_size(g_hid.root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(g_hid.root, 8, 0);
    lv_obj_set_style_border_width(g_hid.root, 0, 0);
    lv_obj_set_style_radius(g_hid.root, 0, 0);
    lv_obj_set_style_bg_opa(g_hid.root, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(g_hid.root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(g_hid.root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_hid.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(g_hid.root, 5, 0);

    // HID enabled container
    lv_obj_t *enable_container = lv_obj_create(g_hid.root);
    lv_obj_set_size(enable_container, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(enable_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(enable_container, 0, 0);
    lv_obj_set_style_pad_all(enable_container, 0, 0);
    lv_obj_set_layout(enable_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(enable_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(enable_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // label for switch
    g_hid.status = lv_label_create(enable_container);
    lv_label_set_text(g_hid.status, "Enable HID");
    lv_obj_set_style_text_color(g_hid.status, ZV_COLOR_TEXT_MAIN, 0);

    g_hid.toggle = lv_switch_create(enable_container);
    lv_obj_add_event_cb(g_hid.toggle, hid_toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);
    zv_nav_add(g_hid.toggle);

    // center_container
    lv_obj_t *center_container = lv_obj_create(g_hid.root);
    lv_obj_set_size(center_container, LV_PCT(100), 45);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(center_container, 0, 0);
    lv_obj_set_style_pad_all(center_container, 0, 0);
    lv_obj_set_layout(center_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(center_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(center_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    g_hid.selected_lbl = lv_label_create(center_container);
    lv_obj_set_style_text_color(g_hid.selected_lbl, ZV_COLOR_TEXT_MAIN, 0);

    lv_obj_t *refresh_btn = lv_btn_create(center_container);
    lv_obj_set_size(refresh_btn, 45, 35);
    lv_obj_set_style_bg_color(refresh_btn, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(refresh_btn, 2, 0);
    lv_obj_set_style_border_color(refresh_btn, ZV_COLOR_BORDER, 0);
    lv_obj_set_style_radius(refresh_btn, 10, 0);

    lv_obj_add_event_cb(refresh_btn, hid_refresh_btn_cb, LV_EVENT_CLICKED, NULL);
    zv_nav_add(refresh_btn);
    lv_obj_set_flex_flow(refresh_btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(refresh_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(refresh_btn, 6, 0);

    lv_obj_t *ic = lv_label_create(refresh_btn);
    lv_label_set_text(ic, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(ic, ZV_COLOR_TEXT_MAIN, 0);

    // Lista (scrollable)
    g_hid.list = lv_list_create(g_hid.root);
    lv_obj_set_width(g_hid.list, LV_PCT(100));

    lv_obj_set_style_bg_color(g_hid.list, ZV_COLOR_BG_PANEL, 0);
    lv_obj_set_style_radius(g_hid.list, 5, 0);
    lv_obj_set_style_pad_all(g_hid.list, 5, 0);
    lv_obj_set_style_pad_row(g_hid.list, 10, 0);

    lv_obj_set_scroll_dir(g_hid.list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(g_hid.list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(g_hid.list, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(g_hid.list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    

    //g_hid.selected_script = selected_path;
    
    hid_set_selected_label();

    // Refrescar scripts al crear
    //TODO: deberia llamarse desde afuera, no en el momento de creacion
    refresh_list_impl();

    lv_obj_set_state(g_hid.toggle, LV_STATE_CHECKED, cfg->hid.is_enabled);
    g_hid.hid_enabled = cfg->hid.is_enabled;

    return g_hid.page;
}

void hid_refresh_scripts(void)
{
    refresh_list_impl();
}

const char *hid_get_selected_script(void)
{
    return g_hid.selected_script;
}
