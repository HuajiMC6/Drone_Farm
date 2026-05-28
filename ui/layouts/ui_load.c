#include "audio.h"
#include "ui.h"
#include "ui_load_cb.h"

lv_obj_t *ui_load_screen_create(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *bg = lv_img_create(screen);
    lv_obj_set_size(bg, 1024, 600);
    lv_img_set_src(bg, img_load_bg);

    lv_obj_t *btn = lv_img_create(bg);
    lv_img_set_src(btn, img_load_btn);
    lv_obj_align_to(btn, bg, LV_ALIGN_BOTTOM_MID, 0, -75);
    lv_obj_set_style_radius(btn, 40, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, icon_btns_click_audio_play, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn, ui_load_btn_cb, LV_EVENT_CLICKED, NULL);

    return screen;
}
