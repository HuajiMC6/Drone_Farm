#include "joystick_win.h"
#include <SDL2/SDL.h>
#include <string.h>

/* ================================================================
 *  内部状态 — 追踪多个按键同时按下
 * ================================================================ */

static int g_key_up = 0;    /* W / ↑ */
static int g_key_down = 0;  /* S / ↓ */
static int g_key_left = 0;  /* A / ← */
static int g_key_right = 0; /* D / → */
static int g_key_sw = 0;    /* 空格 */

/* ================================================================
 *  公开接口
 * ================================================================ */

void joystick_init(void) {
    g_key_up = 0;
    g_key_down = 0;
    g_key_left = 0;
    g_key_right = 0;
    g_key_sw = 0;
}

int8_t joystick_get_dir_x(void) {
    if (g_key_left && !g_key_right)
        return -100;
    if (g_key_right && !g_key_left)
        return 100;
    return 0;
}

int8_t joystick_get_dir_y(void) {
    /* 方向映射:
     *   屏幕坐标 Y↓=正, 无人机向上飞需要 Y 减小
     *   W/↑ → 无人机向上 → -100
     *   S/↓ → 无人机向下 → +100 */
    if (g_key_up && !g_key_down)
        return -100;
    if (g_key_down && !g_key_up)
        return 100;
    return 0;
}

uint16_t joystick_get_raw_x(void) {
    int8_t v = joystick_get_dir_x();
    return (uint16_t)(((int)v + 100) * 65535 / 200);
}

uint16_t joystick_get_raw_y(void) {
    int8_t v = joystick_get_dir_y();
    return (uint16_t)(((int)v + 100) * 65535 / 200);
}

uint8_t joystick_get_switch(void) {
    return g_key_sw ? 1 : 0;
}

uint8_t joystick_adc_ok(void) {
    return 1;
}

void joystick_calibrate_center(void) {
    /* no-op */
}

/* ================================================================
 *  键盘事件处理 (由 input_sdl 模块调用)
 *
 *  追踪多个按键同时按下的状态, 释放一个键不会清掉另一个键的值.
 * ================================================================ */

void joystick_win_handle_key(int scancode, int pressed) {
    switch (scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            g_key_up = pressed;
            break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            g_key_down = pressed;
            break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            g_key_left = pressed;
            break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            g_key_right = pressed;
            break;
        case SDL_SCANCODE_SPACE:
            g_key_sw = pressed;
            break;
        default:
            break;
    }
}
