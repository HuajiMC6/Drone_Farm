#ifndef __UI_MAIN_CB_H
#define __UI_MAIN_CB_H

#include "lvgl.h"

typedef lv_obj_t *(*ui_window_factory_t)(void);

typedef struct {
    ui_window_factory_t create;
    lv_obj_t **window_ref;
} ui_window_toggle_desc_t;

void ui_main_floating_button_click_cb(lv_event_t *e);
void ui_main_drone_click_cb(lv_event_t *e);
void ui_main_screen_click_cb(lv_event_t *e);
void ui_main_farm_upgrade_btn_click_cb(lv_event_t *e);

#endif
