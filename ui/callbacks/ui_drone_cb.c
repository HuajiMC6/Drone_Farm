#include "ui_drone_cb.h"

#include "drone.h"
#include "ui_common.h"

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
    } else {
        drone_state_switch(desc->target_state);
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
