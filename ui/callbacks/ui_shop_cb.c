#include "ui_shop_cb.h"
#include "player.h"
#include "ui_message.h"

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

    shop_item_desc_t selected_desc;
    int qty = 1;
    if (!ui_shop_get_selected_purchase(&selected_desc, &qty)) {
        return;
    }

    bool buy_ok = false;
    switch (selected_desc.kind) {
        case SHOP_KIND_SEED:
            buy_ok = selected_desc.id < CROP_TYPE_NONE ? player_buy_seed((crop_type_t)selected_desc.id, qty) : false;
            break;
        case SHOP_KIND_PESTICIDE:
            buy_ok = selected_desc.id < CROP_PESTICIDE_NONE
                         ? player_buy_pesticide((crop_pesticide_t)selected_desc.id, qty)
                         : false;
            break;
        default:
            buy_ok = false;
            break;
    }

    if (buy_ok) {
        ui_shop_after_buy_success(selected_desc.kind);
        ui_message_show("Purchase successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    }
}
