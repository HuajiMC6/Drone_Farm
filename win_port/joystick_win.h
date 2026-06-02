#ifndef WIN_PORT_JOYSTICK_WIN_H
#define WIN_PORT_JOYSTICK_WIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Windows 键盘模拟摇杆
 *
 * 实现与 Drivers/joystick.h 相同的 API.
 *
 * 按键映射:
 *   W / ↑       → Y轴正向 (上)
 *   S / ↓       → Y轴负向 (下)
 *   A / ←       → X轴负向 (左)
 *   D / →       → X轴正向 (右)
 *   空格         → switch 按下
 */

void     joystick_init(void);
int8_t   joystick_get_dir_x(void);   /* -100 ~ 100 */
int8_t   joystick_get_dir_y(void);   /* -100 ~ 100 */
uint16_t joystick_get_raw_x(void);
uint16_t joystick_get_raw_y(void);
uint8_t  joystick_get_switch(void);  /* 1=按下 */
uint8_t  joystick_adc_ok(void);
void     joystick_calibrate_center(void);

/**
 * @brief 处理 SDL 键盘事件 (由 input_sdl 调用的内部接口)
 *
 * @param scancode  SDL 扫描码
 * @param pressed   1=按下, 0=释放
 */
void joystick_win_handle_key(int scancode, int pressed);

#ifdef __cplusplus
}
#endif

#endif /* WIN_PORT_JOYSTICK_WIN_H */
