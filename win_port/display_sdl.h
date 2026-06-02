#ifndef WIN_PORT_DISPLAY_SDL_H
#define WIN_PORT_DISPLAY_SDL_H

#include "lvgl/lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LVGL 显示驱动 (SDL2 后端)
 *
 * 替代嵌入式版的 lv_port_disp_init().
 * 创建 SDL 窗口 1024×600, 使用 RGB565 像素格式.
 */
void lv_port_disp_init(void);

/**
 * @brief 通知 LVGL 一帧刷新完成 (由 SDL 渲染循环调用)
 *
 * 嵌入式版由 DMA 中断调用, Windows 版改为 SDL 渲染后主动调用.
 */
void lv_port_disp_flush_ready(void);

/**
 * @brief 获取 SDL 渲染器和窗口纹理 (供 input/audio 模块使用)
 *
 * @return SDL_Renderer* 指针 (实际类型为 void* 避免头文件依赖)
 */
void *lv_port_disp_get_renderer(void);
void *lv_port_disp_get_window(void);

/**
 * @brief 将 LVGL 渲染结果提交到屏幕 (供主循环调用)
 */
void lv_port_disp_render(void);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_DISPLAY_SDL_H */
