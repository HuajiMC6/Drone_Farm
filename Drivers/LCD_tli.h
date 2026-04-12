#ifndef LCD_TLI_H
#define LCD_TLI_H

#include "gd32h7xx.h"

#define GMEM_BASE				( 0xC0000000 + 32*1024*1024 - 1024*600*2 )


void lcd_config(void);



#endif
