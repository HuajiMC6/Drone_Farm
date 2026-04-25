#ifndef __UI_EVENT_H
#define __UI_EVENT_H

#include "enum.h"
#include "farm.h"
#include "lvgl.h"

typedef struct {
    crop_type_t type;
    const void *img;
    lv_obj_t *fields;
} ui_drag_to_plant_desc_t;

typedef lv_obj_t *(*ui_window_factory_t)(void);

typedef struct {
    ui_window_factory_t create;
    lv_obj_t **window_ref;
} ui_window_toggle_desc_t;

void farm_block_click_cb(lv_event_t *e);
void screen_main_click_cb(lv_event_t *e);
void main_floating_btn_click_cb(lv_event_t *e);
void drag_to_plant_cb(lv_event_t *e);
void crop_growing_bar_event(lv_event_t *e);
void drone_click_cb(lv_event_t *e);

#endif
