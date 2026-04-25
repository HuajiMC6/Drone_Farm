#ifndef __UI_WINDOW_H
#define __UI_WINDOW_H

#include "lvgl.h"

lv_obj_t *ui_window_create(lv_obj_t *parent, const char *title, lv_obj_t *body);
void ui_window_show(lv_obj_t *window);
void ui_window_hide(lv_obj_t *window);
void ui_window_hide_current(void);
lv_obj_t *ui_window_get_current(void);
bool ui_window_is_visible(lv_obj_t *window);

#endif
