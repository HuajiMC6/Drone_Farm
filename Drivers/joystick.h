#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include "gd32h7xx.h"
#include <stdint.h>

// 初始化
void joystick_init(void);

// 读取数据（非阻塞）
int8_t joystick_get_dir_x(void); // -100~100
int8_t joystick_get_dir_y(void); // -100~100
uint16_t joystick_get_raw_x(void);
uint16_t joystick_get_raw_y(void);
uint8_t joystick_get_switch(void); // 1=按下
uint8_t joystick_adc_ok(void);
void joystick_calibrate_center(void);

#endif