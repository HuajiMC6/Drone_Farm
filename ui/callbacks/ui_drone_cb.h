#ifndef __UI_DRONE_CB_H
#define __UI_DRONE_CB_H

#include "enum.h"
#include "lvgl.h"

typedef struct {
    crop_pesticide_t pesticide;
    int delta;
} drone_pesticide_btn_desc_t;

typedef struct {
    drone_state_t target_state;
} drone_mode_btn_desc_t;

void ui_drone_mode_button_click_cb(lv_event_t *e);
void ui_drone_pesticide_button_click_cb(lv_event_t *e);

#endif
