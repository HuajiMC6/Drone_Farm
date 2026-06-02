#include "input_sdl.h"
#include "compat.h"
#include "joystick_win.h"

#include <SDL2/SDL.h>
#include <stdio.h>

/* ================================================================
 *  内部状态
 * ================================================================ */
/* 全局输入设备指针 (ui.c 引用) */
lv_indev_t *indev_touchpad = NULL;
static lv_coord_t g_last_x = 0;
static lv_coord_t g_last_y = 0;
static int g_pressed = 0;

/* ================================================================
 *  LVGL 输入回调
 * ================================================================ */

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    (void)indev_drv;

    if (g_pressed) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = g_last_x;
        data->point.y = g_last_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
        data->point.x = g_last_x;
        data->point.y = g_last_y;
    }
}

/* ================================================================
 *  公开接口
 * ================================================================ */

void lv_port_indev_init(void) {
    /* 注册 LVGL 触摸输入设备 */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);

    printf("[INFO] SDL input initialized (mouse as touch)\n");
}

int lv_port_indev_handle_sdl_events(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {

            case SDL_QUIT:
                return 1; /* 通知主循环退出 */

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    g_pressed = 1;
                    g_last_x = event.button.x;
                    g_last_y = event.button.y;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    g_pressed = 0;
                }
                break;

            case SDL_MOUSEMOTION:
                if (g_pressed) {
                    g_last_x = event.motion.x;
                    g_last_y = event.motion.y;
                }
                break;

            /* 键盘事件传递给 joystick_win 处理 */
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                int pressed = (event.type == SDL_KEYDOWN);
                joystick_win_handle_key(event.key.keysym.scancode, pressed);
                break;
            }

            default:
                break;
        }
    }

    return 0; /* 没有退出事件 */
}
