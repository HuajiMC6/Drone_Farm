#include "ui_storage_cb.h"

#include "player.h"
#include "ui_message.h"
#include "ui_storage.h"

void ui_storage_item_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    crop_type_t *type = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (!type || !target) {
        return;
    }

    ui_storage_item_click_handle(*type, target);
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

    crop_type_t type;
    int qty = 0;
    if (!ui_storage_get_selected_sell(&type, &qty)) {
        return;
    }

    int total_earning;
    bool result = player_sold(type, qty, &total_earning);
    if (result) {
        ui_storage_after_sell_success();

        char message[64];
        snprintf(message, sizeof(message), "Sold %s x %d for %d coins!", crop_type_name(type), qty, total_earning);
        ui_message_show(message, UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    }
}
