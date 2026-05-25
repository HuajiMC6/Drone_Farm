#ifndef __UI_DRONE_H
#define __UI_DRONE_H

#include "event.h"
#include "ui_common.h"

void ui_drone_module_create(lv_obj_t *parent, lv_obj_t *screen);
void ui_drone_handle_event(event_t *event);
void ui_drone_update_100ms(void);
void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb);

#endif
