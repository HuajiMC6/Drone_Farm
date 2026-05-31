#ifndef __UI_SETTING_CB_H__
#define __UI_SETTING_CB_H__

#include "lvgl.h"

void ui_setting_reset_game_cb(lv_event_t *e);
void ui_setting_add_coins_cb(lv_event_t *e);
void ui_setting_add_level_cb(lv_event_t *e);
void ui_setting_game_speed_cb(lv_event_t *e);
void ui_setting_volume_slider_cb(lv_event_t *e);

#endif
