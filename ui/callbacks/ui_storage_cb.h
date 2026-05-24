#ifndef __UI_STORAGE_CB_H
#define __UI_STORAGE_CB_H

#include "data.h"
#include "lvgl.h"

typedef struct {
    crop_type_t type;
} ui_storage_crop_desc_t;

void ui_storage_item_click_cb(lv_event_t *e);
void ui_storage_qty_minus_click_cb(lv_event_t *e);
void ui_storage_qty_plus_click_cb(lv_event_t *e);
void ui_storage_sell_click_cb(lv_event_t *e);

void ui_storage_item_click_handle(const ui_storage_crop_desc_t *desc, lv_obj_t *target);
void ui_storage_qty_minus_click_handle(void);
void ui_storage_qty_plus_click_handle(void);

#endif
