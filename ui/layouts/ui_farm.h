#ifndef __UI_FARM_H
#define __UI_FARM_H

#include "event.h"
#include "ui_common.h"

void ui_farm_module_create(lv_obj_t *screen_main, lv_obj_t *main_layer);
void ui_farm_handle_event(event_t *event);
lv_obj_t *ui_farm_get_grid(void);
farm_block_t *ui_farm_get_block(int x, int y);
void ui_farm_refresh_all(void);
void ui_farm_clear_field_selection(lv_obj_t *parent);
void ui_field_upgrade_window_switch(farm_block_t *block);
void ui_field_upgrade_window_refresh(farm_block_t *block);

#endif
