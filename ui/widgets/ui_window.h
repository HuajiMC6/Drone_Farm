#ifndef __UI_WINDOW_H
#define __UI_WINDOW_H

#include "lvgl.h"

typedef lv_obj_t *(*ui_window_factory_t)(void);

typedef struct {
    ui_window_factory_t create;
    lv_obj_t **window_ref;
} ui_window_toggle_desc_t;

lv_obj_t *ui_window_create(const char *title, lv_obj_t *body, bool enable_mask);
void ui_window_show(lv_obj_t *window);
void ui_window_hide(lv_obj_t *window);
void ui_window_hide_current(void);
lv_obj_t *ui_window_get_current(void);
bool ui_window_is_visible(lv_obj_t *window);
void ui_window_set_display_relative(lv_obj_t *window);
void ui_window_follow_scroll(lv_obj_t *window, lv_obj_t *target);
void ui_window_disable_keep_alive(lv_obj_t *window);
void ui_window_toggle(ui_window_toggle_desc_t *desc);

#endif
