#ifndef __UI_STORAGE_H
#define __UI_STORAGE_H

#include "event.h"
#include "ui_common.h"

lv_obj_t *ui_storage_window_create(void);
void ui_storage_window_refresh(void);
bool ui_storage_get_selected_sell(crop_type_t *type, int *qty);
void ui_storage_after_sell_success(void);
void ui_storage_window_select(crop_type_t type, lv_obj_t *target);
void ui_storage_handle_event(event_t *event);

#endif
