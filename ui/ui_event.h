#ifndef __UI_EVENT_H
#define __UI_EVENT_H

#include "lvgl.h"
#include "enum.h"
#include "farm.h"

typedef struct {
	crop_type_t type;
	const void *img;
	lv_obj_t *fields;
} ui_drag_to_plant_desc_t;

void farm_block_click_cb(lv_event_t *e);
void screen_main_click_cb(lv_event_t *e);
void plant_btn_click_cb(lv_event_t *e);
void drag_to_plant_cb(lv_event_t *e);
void crop_growing_bar_event(lv_event_t *e);
void window_delete_cb(lv_event_t *e);
void drone_click_cb(lv_event_t *e);

#endif
