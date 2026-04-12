#ifndef DRIVERS_H
#define DRIVERS_H

#include <string.h>
#include <stdio.h>

#include "LCD_tli.h"
#include "exmc_sdram.h"
#include "ff.h"
#include "touch.h"
#include "lvgl.h"
#include "sdram_malloc.h"

typedef struct
{
	uint16_t width;
	uint16_t height;
 	uint16_t * src;
    uint16_t * des;
} graphic_dma_struct;

//extern volatile bool touch_status;
extern lcd_touch_point_t tp[5];
extern graphic_dma_struct gdma;
extern __IO uint16_t gdma_lines;

void systick_config(void);
void delay_us(uint32_t nus);
void graphic_dma_copy(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *src);
void sys_init();

uint32_t get_tick(void);

#endif
