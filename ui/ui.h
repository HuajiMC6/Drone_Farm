#ifndef __UI_H
#define __UI_H

#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

// 界面类型
typedef enum {
	UI_SCREEN_LOAD,     // 初始加载界面
	UI_SCREEN_MAIN      // 游戏主界面
} ui_screen_type_t;

// UI层初始化
void ui_init(void);
// 界面切换
void ui_screen_switch(ui_screen_type_t screen);

#endif