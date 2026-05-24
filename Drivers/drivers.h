#ifndef DRIVERS_H
#define DRIVERS_H

#include <stdio.h>
#include <string.h>

#include "LCD_tli.h"
#include "exmc_sdram.h"
#include "ff.h"
#include "joystick.h"
#include "lvgl.h"
#include "sdram_malloc.h"
#include "speaker.h"

#include "touch.h"

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t *src;
    uint16_t *des;
} graphic_dma_struct;

// extern volatile bool touch_status;
extern lcd_touch_point_t tp[5];
extern graphic_dma_struct gdma;
extern __IO uint16_t gdma_lines;

void systick_config(void);
void delay_us(uint32_t nus);
void graphic_dma_copy(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *src);
void sys_init();

#endif
