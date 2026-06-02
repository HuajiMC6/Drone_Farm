#ifndef WIN_PORT_INPUT_SDL_H
#define WIN_PORT_INPUT_SDL_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LVGL 输入驱动 (SDL2 鼠标)
 *
 * 替代嵌入式版的 lv_port_indev_init().
 * 鼠标左键模拟触摸按下, 鼠标移动模拟触摸位置.
 */
void lv_port_indev_init(void);

/**
 * @brief 处理 SDL 事件 (供主循环调用)
 *
 * 返回 1 表示收到退出事件, 主循环应退出.
 */
int lv_port_indev_handle_sdl_events(void);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_INPUT_SDL_H */
