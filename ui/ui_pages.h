#ifndef __UI_PAGES_H
#define __UI_PAGES_H

#include "ui.h"

lv_obj_t *ui_load_screen_create(void);
lv_obj_t *ui_main_screen_create(void);
void ui_main_handle_event(event_t *event);
void ui_main_update_timer_init(void);

#endif
