#ifndef __UI_COMMON_H
#define __UI_COMMON_H

#include "data.h"
#include "farm.h"
#include "lvgl.h"

// 这个东西先这样吧，刚开始没设计好，后面需要重构成ctx
typedef struct {
    lv_obj_t *obj;
    lv_obj_t *crop_img;
    lv_obj_t *pest_img;
    lv_obj_t *growing_bar;
    lv_obj_t *death_bar;
    field_t *field;
    bool is_planted;
    uint8_t x;
    uint8_t y;
    bool has_pest;
    bool is_detected;
} farm_block_t;

/* ──────── 工具函数 ──────── */
lv_obj_t *ui_div_create(lv_obj_t *parent);

/* 透明弹性容器：无底色 + 无边框 + 零内边距 + 禁用滚动 */
lv_obj_t *ui_transparent_cont_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);

/* 样式卡片：浅米色底 + 灰棕边框 + 圆角 10 + pad 8 + 禁用滚动 */
lv_obj_t *ui_card_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);

/* 快速创建带颜色标签 */
lv_obj_t *ui_label_colored(lv_obj_t *parent, const char *text, lv_color_t color);

/* 带弹性布局的卡片：ui_card_create + flex COLUMN + pad_row 6 + scrollbar OFF */
lv_obj_t *ui_card_create_with_flex(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);

/* 按钮工厂：尺寸 + 底色 + 边框色 + 圆角 8 + 居中文本 + 事件 */
lv_obj_t *ui_btn_factory(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const char *text, lv_color_t bg,
                         lv_color_t border, lv_event_cb_t cb, void *user_data);

/* 仅 UI 内部模块使用 */
extern uint8_t ui_drone_pest_count[CROP_DAMAGE_NONE];
lv_obj_t *ui_drone_window_create(void);
void ui_drone_window_refresh(void);
void ui_drone_hud_create(lv_obj_t *parent);
void ui_drone_hud_set_visible(bool visible);
void ui_shop_refresh(void);
void ui_field_upgrade_window_switch(farm_block_t *block);

#endif
