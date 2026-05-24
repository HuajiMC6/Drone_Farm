#ifndef __UI_SHOP_CB_H
#define __UI_SHOP_CB_H

#include "lvgl.h"

typedef enum {
    SHOP_KIND_SEED,
    SHOP_KIND_PESTICIDE,
} shop_item_kind_t;

typedef struct {
    shop_item_kind_t kind;
    uint8_t id;
} shop_item_desc_t;

void ui_shop_item_click_cb(lv_event_t *e);
void ui_shop_qty_minus_click_cb(lv_event_t *e);
void ui_shop_qty_plus_click_cb(lv_event_t *e);
void ui_shop_buy_click_cb(lv_event_t *e);

/* UI 暴露给回调层的最小购买接口 */
bool ui_shop_get_selected_purchase(shop_item_desc_t *desc, int *qty);
void ui_shop_after_buy_success(shop_item_kind_t kind);

void ui_shop_item_click_handle(const shop_item_desc_t *desc, lv_obj_t *target);
void ui_shop_qty_minus_click_handle(void);
void ui_shop_qty_plus_click_handle(void);

#endif
