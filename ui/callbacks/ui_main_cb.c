#include "ui_main_cb.h"

#include "player.h"
#include "ui_common.h"
#include "ui_farm.h"
#include "ui_message.h"

void debug_heartbeat_timer_set_period(uint32_t period_ms);

void ui_reset(void);

/* ── 悬浮按钮 ── */

void ui_main_floating_button_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_window_toggle_desc_t *desc = lv_event_get_user_data(e);
    ui_window_toggle(desc);
}

/* ── 屏幕点击：关闭弹窗 / 清除田地选中 ── */

void ui_main_screen_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (target == obj || lv_obj_get_parent(target) == obj) {
        return;
    }

    lv_obj_t *current_window = ui_window_get_current();
    if (current_window) {
        ui_window_hide_current();
    } else {
        ui_farm_clear_field_selection(obj);
    }
}

/* ── 农场升级按钮 ── */

void ui_main_farm_upgrade_btn_click_cb(lv_event_t *e) {
    bool result = player_buy_farm_size_update();
    if (result) {
        ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
        ui_window_hide_current();
    } else {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
    }
}
