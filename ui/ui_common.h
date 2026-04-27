#ifndef __UI_COMMON_H
#define __UI_COMMON_H

#include "enum.h"
#include "farm.h"
#include "lvgl.h"

// 这个东西先这样吧，刚开始没设计好，后面需要重构成ctx
typedef struct {
    lv_obj_t *obj;
    lv_obj_t *crop_img;
    lv_obj_t *pest_img;
    lv_obj_t *growing_bar;
    field_t *field;
    bool is_planted;
    uint8_t x;
    uint8_t y;
    bool has_pest;
    bool is_detected;
} farm_block_t;

/* 工具函数 */
lv_obj_t *ui_div_create(lv_obj_t *parent);

/* 仅 UI 内部模块使用 */
extern uint8_t ui_drone_pest_count[CROP_DAMAGE_NONE];
lv_obj_t *ui_drone_window_create(void);
void ui_drone_window_refresh(void);
void ui_drone_hud_create(lv_obj_t *parent);
void ui_drone_hud_set_visible(bool visible);
void ui_shop_refresh(void);

#endif