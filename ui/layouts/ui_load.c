#include "ui.h"
#include "ui_load_cb.h"

lv_obj_t *ui_load_screen_create(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // DEBUG
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "LOAD PAGE");
    lv_obj_center(label);

    lv_obj_t *btn = lv_btn_create(screen);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align_to(btn, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Enter Game");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(btn, ui_load_test_cb, LV_EVENT_CLICKED, NULL);

    return screen;
}
