#include "ui_shop_cb.h"

void ui_shop_item_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    shop_item_desc_t *desc = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (!desc || !target) {
        return;
    }

    ui_shop_item_click_handle(desc, target);
}

void ui_shop_qty_minus_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_shop_qty_minus_click_handle();
}

void ui_shop_qty_plus_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_shop_qty_plus_click_handle();
}

void ui_shop_buy_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_shop_buy_click_handle();
}
