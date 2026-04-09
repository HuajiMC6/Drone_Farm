#ifndef __UI_H
#define __UI_H

#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "farm.h"
#include "icon.h"

// 界面类型
typedef enum {
	UI_SCREEN_LOAD,     // 初始加载界面
	UI_SCREEN_MAIN      // 游戏主界面
} ui_screen_type_t;


typedef struct {
	lv_obj_t *obj;
	lv_obj_t *window;
	field_t *field;
	bool is_planted;
	uint8_t x;
	uint8_t y;
	icon_pest_type_t pest_type;
} farm_block_t;


// UI层初始化
void ui_init(void);
// 界面切换
void ui_screen_switch(ui_screen_type_t screen);
// UI数据刷新定时器初始化
void ui_update_timer_init();

#endif
