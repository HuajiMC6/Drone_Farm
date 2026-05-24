#include "ui_common.h"

lv_obj_t *ui_div_create(lv_obj_t *parent) {
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(div, 0, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_pad_all(div, 0, 0);

    return div;
}

lv_obj_t *ui_transparent_cont_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

lv_obj_t *ui_card_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

lv_obj_t *ui_label_colored(lv_obj_t *parent, const char *text, lv_color_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

lv_obj_t *ui_btn_factory(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const char *text, lv_color_t bg,
                         lv_color_t border, lv_event_cb_t cb, void *user_data) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_border_color(btn, border, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    if (cb) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    }
    return btn;
}

lv_obj_t *ui_card_create_with_flex(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *card = ui_card_create(parent, w, h);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    return card;
}
