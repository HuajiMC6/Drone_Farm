#ifndef __UI_DRONE_H
#define __UI_DRONE_H

#include "event.h"
#include "ui_common.h"
#include "ui_window.h"

void ui_drone_module_create(lv_obj_t *parent, lv_obj_t *screen);
void ui_drone_handle_event(event_t *event);
void ui_drone_update_100ms(void);
void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb);

/* 无人机窗口与 HUD */
extern uint8_t ui_drone_pest_count[CROP_DAMAGE_NONE];
extern ui_window_toggle_desc_t g_drone_window_toggle;
lv_obj_t *ui_drone_window_create(void);
void ui_drone_window_refresh(void);
void ui_drone_hud_create(lv_obj_t *parent);
void ui_drone_hud_set_visible(bool visible);

#endif
