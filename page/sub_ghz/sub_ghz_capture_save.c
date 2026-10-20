
#include "sub_ghz_capture_save.h"

#include "components/component_helper.h"
#include "components/text_input/ui_text_input.h"
#include "components/button/ui_button.h"
#include "utils/string_utils.h"
#include "types.h"
#include "page/navigation_page.h"

#include <string.h>

typedef struct {
    ui_button *save_btn;
    lv_obj_t *status_lbl;
    ui_button *back_btn;
    ui_text_input *file_name_txt;
    subghz_save_confirm_cb on_confirm;
} subghz_save_view_ctx;

static subghz_save_view_ctx local_ctx;

static void view_delete_cb(lv_event_t *e)
{
    (void)e;
    memset(&local_ctx, 0, sizeof(local_ctx));
}

static void subghz_save_back_handler(event_data_btn *event)
{
    zv_nav_back_cb(event->lv_event);
}

static void subghz_save_file_handler(event_data_btn *event)
{
    const char *file_name = ui_text_get_text(local_ctx.file_name_txt);
    if (zv_is_empty(file_name))
    {
       // learn_set_status("Select a remote first.");
        return;
    }

    if (event->user_data)
    {
        subghz_save_view_ctx *ctx = (subghz_save_view_ctx *)event->user_data;
        if (ctx->on_confirm)
        {
            bool success = ctx->on_confirm(file_name);
            if (success)
                zv_nav_pop();
        }       
    }
}

static void create_bottom_panel(lv_obj_t *parent, subghz_save_confirm_cb on_confirm)
{
    lv_obj_t *bottom_panel = helper_create_bottom_panel(parent);
 
    local_ctx.on_confirm = on_confirm;
   
    local_ctx.save_btn = ui_button_create(bottom_panel,  LV_PCT(48), 50, "Save");
    ui_button_set_on_click(local_ctx.save_btn, subghz_save_file_handler, &local_ctx);
    ui_button_set_action_style(local_ctx.save_btn, Success);

    local_ctx.back_btn = ui_button_create(bottom_panel, LV_PCT(48), 50, "Back");
    ui_button_set_on_click(local_ctx.back_btn, subghz_save_back_handler, NULL);
}

zv_view_t *subghz_capture_save_create(subghz_save_confirm_cb on_confirm)
{
    memset(&local_ctx, 0, sizeof(local_ctx));
    zv_view_t *view = zv_view_new("Name Capture");
    if (!view)
        return NULL;

    lv_obj_add_event_cb(view->main_container, view_delete_cb, LV_EVENT_DELETE, NULL);    

    lv_obj_t *root = view->content;
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    create_section_label(root, "Signal Name");

    // ui_text_input_options input_options = {
    //     .use_on_screen_keyboard = cfg ? cfg->ir.use_on_screen_keyboard : true,
    //     .one_line = true
    // };

    local_ctx.file_name_txt = ui_text_create(root, LV_PCT(100), 60, "eg: doorbell");
    create_bottom_panel(root, on_confirm);

    return view;
}