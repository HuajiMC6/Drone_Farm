#ifndef __UI_MAIN_CB_H
#define __UI_MAIN_CB_H

#include "enum.h"
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

void ui_main_field_block_click_cb(lv_event_t *e);
void ui_main_screen_click_cb(lv_event_t *e);
void ui_main_floating_button_click_cb(lv_event_t *e);
void ui_main_seed_drag_event_cb(lv_event_t *e);
void ui_main_drone_click_cb(lv_event_t *e);
void ui_main_crop_growing_bar_draw_part_end_cb(lv_event_t *e);

#endif
