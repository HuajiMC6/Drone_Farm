#ifndef __UI_GRID_LIST_H
#define __UI_GRID_LIST_H

#include "lv_port_disp_template.h"

// 网格列表配置结构体，包含布局参数和样式
typedef struct {
    lv_coord_t item_w;    // 单个格子的宽度
    lv_coord_t item_h;    // 单个格子的高度
    uint16_t col_count;   // 网格列数
    uint16_t row_count;   // 网格行数
    lv_coord_t pad_col;   // 列间距
    lv_coord_t pad_row;   // 行间距
    lv_coord_t pad_all;   // 四周内边距
    bool item_scrollable; // 格子是否允许内部滚动

    lv_color_t item_bg_color;         // 格子默认背景色
    lv_color_t item_bg_color_pressed; // 格子按下时背景色
    lv_color_t item_border_color;     // 格子边框颜色
    lv_coord_t item_border_width;     // 格子边框宽度
} ui_grid_list_cfg_t;

// 网格列表对象，包含配置和状态信息
typedef struct {
    lv_obj_t *obj;          // 容器对象
    ui_grid_list_cfg_t cfg; // 当前使用的配置结构体
    lv_coord_t *col_dsc;    // 列描述数组
    lv_coord_t *row_dsc;    // 行描述数组
    uint16_t item_count;    // 当前格子数量
    uint16_t next_index;    // 下一个插入索引
} ui_grid_list_t;

/* 初始化默认样式，偏种子表风格 */
void ui_grid_list_cfg_init(ui_grid_list_cfg_t *cfg);

/* 快速构造：init + 填入宽高行列 */
ui_grid_list_cfg_t ui_grid_list_cfg_make(lv_coord_t item_w, lv_coord_t item_h, uint16_t cols, uint16_t rows);

/* 设置三个间距 */
void ui_grid_list_cfg_set_pad(ui_grid_list_cfg_t *cfg, lv_coord_t pad_col, lv_coord_t pad_row, lv_coord_t pad_all);

/* 创建一个可复用的网格列表容器 */
ui_grid_list_t *ui_grid_list_create(lv_obj_t *parent, const ui_grid_list_cfg_t *cfg);

/* 按指定行列添加一个格子 */
lv_obj_t *ui_grid_list_add_item_at(ui_grid_list_t *list, uint16_t col, uint16_t row);

/* 按顺序添加到下一个空格子 */
lv_obj_t *ui_grid_list_add_item(ui_grid_list_t *list);

/* 清空所有子项，并重置添加状态 */
bool ui_grid_list_clear(ui_grid_list_t *list);

/* 先清空，再按新配置重建布局 */
bool ui_grid_list_reset(ui_grid_list_t *list, const ui_grid_list_cfg_t *cfg);

/* 给单个格子绑事件回调 */
bool ui_grid_list_bind_item_event(lv_obj_t *item, lv_event_cb_t cb, lv_event_code_t code, void *user_data);

/* 给整个列表容器绑事件回调 */
bool ui_grid_list_bind_list_event(ui_grid_list_t *list, lv_event_cb_t cb, lv_event_code_t code, void *user_data);

/* 获取底层 LVGL 对象 */
lv_obj_t *ui_grid_list_get_obj(ui_grid_list_t *list);

/* 便捷接口：创建带图标和点击事件的格子 */
lv_obj_t *ui_grid_list_add_icon_item(ui_grid_list_t *list, const void *icon_src, lv_event_cb_t cb, void *user_data);

#endif
