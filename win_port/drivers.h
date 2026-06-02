/**
 * @file    win_port/drivers.h
 * @brief   替代原版 Drivers/drivers.h, 移除 GD32H7xx HAL 依赖
 *
 * 原版 drivers.h 包含了 LCD_tli.h, exmc_sdram.h 等硬件相关头文件.
 * Windows 版本只保留便携组件的声明.
 */

#ifndef WIN_PORT_DRIVERS_H
#define WIN_PORT_DRIVERS_H

#include <stdio.h>
#include <string.h>

/* ---- 保留的便携组件 ---- */
#include "ff.h"             /* FATFS 文件系统 */
#include "joystick.h"       /* 摇杆接口 (由 joystick_win.c 实现) */
#include "lvgl.h"           /* LVGL 图形库 (包含 lvgl/lvgl.h) */
#include "speaker.h"        /* 音频接口 (由 audio_sdl.c 实现) */
#include "touch.h"          /* 触摸数据结构 (仅类型定义) */

/*
 * 以下头文件在 Windows 版不需要:
 *   LCD_tli.h     → display_sdl.c
 *   exmc_sdram.h  → Windows 堆 (malloc)
 *   sdram_malloc.h → Windows 堆 (见下方宏)
 */

/* ---- SDRAM 分配器 → Windows malloc/free ---- */
#include <stdlib.h>
#define sdram_malloc(size)  malloc(size)
#define sdram_free(ptr)     free(ptr)

/* ---- 兼容类型 (原 drivers.h 中定义的) ---- */
#include <stdint.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t *src;
    uint16_t *des;
} graphic_dma_struct;

/* ---- 全局变量 (原 drivers.h 中声明的) ---- */
/* touch_iic.c */
#include "touch.h"
extern lcd_touch_point_t tp[5];
extern graphic_dma_struct gdma;
extern volatile uint16_t gdma_lines;

/* ---- 函数声明 (由 win_port 各模块实现) ---- */
void systick_config(void);
void delay_us(uint32_t nus);

/*
 * graphic_dma_copy — 原版使用 DMA 从 LVGL 缓冲传送到 LCD.
 * Windows 版不需要 (SDL 纹理替代), 但为兼容保留空实现宏.
 */
#define graphic_dma_copy(x1, x2, y1, y2, src)  ((void)0)

void sys_init(void);

#endif /* WIN_PORT_DRIVERS_H */
