#include "ui_common.h"

lv_obj_t *ui_div_create(lv_obj_t *parent) {
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(div, 0, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);

    return div;
}
