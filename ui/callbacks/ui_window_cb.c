#include "ui_window_cb.h"

void ui_window_mask_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_window_mask_click_handle();
}

void ui_window_lifecycle_delete_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    ui_window_lifecycle_delete_handle(target);
}
