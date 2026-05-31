#ifndef __UI_FARM_CB_H
#define __UI_FARM_CB_H

#include "data.h"
#include "lvgl.h"

/* 拖拽种植描述符 */
typedef struct {
    crop_type_t type;
    const void *img;
    lv_obj_t *fields;
} ui_drag_to_plant_desc_t;

/* 地块交互 */
void ui_farm_field_block_click_cb(lv_event_t *e);
void ui_farm_field_block_long_press_cb(lv_event_t *e);

/* 地块升级 */
void ui_farm_output_upgrade_click_cb(lv_event_t *e);
void ui_farm_ready_time_upgrade_click_cb(lv_event_t *e);
void ui_farm_tolerance_upgrade_click_cb(lv_event_t *e);

/* 拖拽种植 */
void ui_farm_seed_drag_event_cb(lv_event_t *e);

/* 作物进度条绘制 */
void ui_farm_crop_bar_draw_part_end_cb(lv_event_t *e);

/* 一键收获 */
void ui_farm_harvest_all_click_cb(lv_event_t *e);

#endif
