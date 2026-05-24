#include "ui_storage_cb.h"

#include "player.h"

void ui_storage_item_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    ui_storage_crop_desc_t *desc = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (!desc || !target) {
        return;
    }

    ui_storage_item_click_handle(desc, target);
}

void ui_storage_qty_minus_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_storage_qty_minus_click_handle();
}

void ui_storage_qty_plus_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_storage_qty_plus_click_handle();
}

void ui_storage_sell_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    ui_storage_crop_desc_t desc;
    int qty = 0;
    if (!ui_storage_get_selected_sell(&desc, &qty)) {
        return;
    }

    if (player_sold(desc.type, qty)) {
        ui_storage_after_sell_success();
    }
}
