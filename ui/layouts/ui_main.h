#ifndef __UI_MAIN_H__
#define __UI_MAIN_H__

#include "lvgl.h"
#include "event.h"

lv_obj_t *ui_main_screen_create(void);
void ui_main_handle_event(event_t *event);
void ui_main_update_timer_init(void);
void ui_main_update_timer_start(void);
void ui_main_update_timer_pause(void);

#endif
