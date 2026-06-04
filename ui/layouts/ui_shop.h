#ifndef __UI_SHOP_H
#define __UI_SHOP_H

#include "event.h"
#include "ui_common.h"

void ui_shop_refresh(void);
lv_obj_t *ui_shop_window_create(void);
void ui_shop_handle_event(event_t *event);

#endif
