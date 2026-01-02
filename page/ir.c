#include "ir.h"
#include "components/ui_theme.h"

lv_obj_t *ir_page_create(lv_obj_t *menu, const zv_config *cfg)
{
    lv_obj_t *page = lv_menu_page_create(menu, "IR");
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_OFF); 

    lv_obj_t *root = lv_obj_create(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(root, 8, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // label for switch
    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, "texto de prueba");
    lv_obj_set_style_text_color(label, ZV_COLOR_TEXT_MAIN, 0);

    return page;
}