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

typedef struct {
    lv_obj_t *obj;
    lv_obj_t *speed_label;
    lv_obj_t *algorithm_label;
    lv_obj_t *storage_label;
    lv_obj_t *result_labels[CROP_DAMAGE_NONE];
    lv_obj_t *detect_btn;
    lv_obj_t *spray_btn;
    lv_obj_t *pesticide_num_labels[CROP_PESTICIDE_NONE];
    lv_obj_t *state_label;
} drone_window_ctx_t;
extern drone_window_ctx_t g_drone_window_ctx;

/* 工具函数 */
lv_obj_t *ui_div_create(lv_obj_t *parent);

#endif