#include "ui_storage_cb.h"

#include "player.h"
#include "ui_message.h"

/* UI层接口 */
bool ui_storage_get_selected_sell(ui_storage_crop_desc_t *desc, int *qty);
void ui_storage_after_sell_success(void);

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

    int total_earning;
    bool result = player_sold(desc.type, qty, &total_earning);
    if (result) {
        ui_storage_after_sell_success();

        char message[64];
        snprintf(message, sizeof(message), "Sold %s x %d for %d coins!", crop_type_name(desc.type), qty, total_earning);
        ui_message_show(message, UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    }
}
