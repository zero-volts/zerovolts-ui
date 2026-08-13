#include "sub_ghz_capture.h"

#include <stdlib.h>
#include <string.h>

#include "components/ui_theme.h"
#include "components/component_helper.h"
#include "page/sub_ghz/sub_ghz_controller.h"

static void subghz_capture_handler(lv_event_t *e)
{
    subghz_start_capture();
}

lv_obj_t *subghz_capture_page_create(lv_obj_t *menu)
{
    lv_obj_t *page = lv_menu_page_create(menu, "Signal Capture");


    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *btn = create_btn(root, "Start Capture", LV_PCT(48), 100);
    lv_obj_add_event_cb(btn, subghz_capture_handler, LV_EVENT_CLICKED, NULL); 
        
    return page;
}