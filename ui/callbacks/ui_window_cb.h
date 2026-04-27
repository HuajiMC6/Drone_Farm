#ifndef __UI_WINDOW_CB_H
#define __UI_WINDOW_CB_H

#include "lvgl.h"

void ui_window_mask_click_cb(lv_event_t *e);
void ui_window_lifecycle_delete_cb(lv_event_t *e);

void ui_window_mask_click_handle(void);
void ui_window_lifecycle_delete_handle(lv_obj_t *target);

#endif
