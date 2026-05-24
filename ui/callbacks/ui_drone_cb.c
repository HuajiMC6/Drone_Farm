#include "ui_drone_cb.h"

#include "drone.h"
#include "player.h"
#include "ui_common.h"
#include "ui_message.h"

static int ui_drone_pesticide_used_local(const drone_t *drone);

void ui_drone_mode_button_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    drone_mode_btn_desc_t *desc = lv_event_get_user_data(e);
    if (!desc) {
        return;
    }

    drone_t *drone = drone_get_instance();
    if (!drone) {
        return;
    }

    if (drone->drone_state == desc->target_state) {
        drone_state_switch(DRONE_STATE_FREE);
        ui_message_show("Recall the drone.", UI_MESSAGE_TYPE_INFO, UI_MESSAGE_TOAST);
    } else {
        drone_state_switch(desc->target_state);
        switch (desc->target_state) {
            case DRONE_STATE_DETECTING:
                ui_message_show("Drone: start DETECTING.", UI_MESSAGE_TYPE_INFO, UI_MESSAGE_TOAST);
                break;
            case DRONE_STATE_AUTO:
                ui_message_show("Drone: start SPRAYING.", UI_MESSAGE_TYPE_INFO, UI_MESSAGE_TOAST);
                break;
            default:
                break;
        }
    }

    ui_drone_window_refresh();
}

void ui_drone_pesticide_button_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    drone_pesticide_btn_desc_t *desc = lv_event_get_user_data(e);
    drone_t *drone = drone_get_instance();
    if (!desc || !drone) {
        return;
    }

    if (drone->drone_state != DRONE_STATE_FREE) {
        return;
    }

    if (desc->delta > 0) {
        if (!drone_add_pesticide(desc->pesticide, 1)) {
            return;
        }
    } else {
        if (!drone_remove_pesticide(desc->pesticide, 1)) {
            return;
        }
    }

    ui_drone_window_refresh();
}

void ui_drone_speed_upgrade_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    if (!player_buy_drone_speed_update()) {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
        return;
    }

    ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    ui_drone_window_refresh();
}

void ui_drone_storage_upgrade_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);

    if (!player_buy_drone_storage_update()) {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
        return;
    }

    ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    ui_drone_window_refresh();
}
