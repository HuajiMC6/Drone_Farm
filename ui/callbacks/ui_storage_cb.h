#ifndef __UI_STORAGE_CB_H
#define __UI_STORAGE_CB_H

#include "data.h"
#include "lvgl.h"

void ui_storage_item_click_cb(lv_event_t *e);
void ui_storage_qty_minus_click_cb(lv_event_t *e);
void ui_storage_qty_plus_click_cb(lv_event_t *e);
void ui_storage_sell_click_cb(lv_event_t *e);

void ui_storage_qty_minus_click_handle(void);
void ui_storage_qty_plus_click_handle(void);

#endif
