#ifndef __UI_H
#define __UI_H

#include "event.h"
#include "farm.h"
#include "icon.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

// 界面类型
typedef enum {
    UI_SCREEN_LOAD, // 初始加载界面
    UI_SCREEN_MAIN  // 游戏主界面
} ui_screen_type_t;

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

extern lv_obj_t *g_current_window;

// UI层初始化
void ui_init(void);
// 界面切换
void ui_screen_switch(ui_screen_type_t screen);
// UI数据刷新定时器初始化
void ui_update_timer_init();
// 逻辑层事件处理
void ui_event_handler(event_t *event);

/* 工具函数 */
lv_obj_t *ui_div_create(lv_obj_t *parent);
lv_obj_t *ui_window_create(const char *title, lv_obj_t *body, const void *btn1_text, void (*btn1_cb)(lv_event_t *),
                           const void *btn2_text, void (*btn2_cb)(lv_event_t *), void *user_data);

#endif
