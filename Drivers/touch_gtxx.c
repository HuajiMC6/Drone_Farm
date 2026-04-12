#include "touch.h"
#include "touch_iic.h"
#include <string.h>


/* RGB LCD模块触摸的PID */
#define ATK_RGBLCD_TOUCH_PID                "911"
#define ATK_RGBLCD_TOUCH_PID1               "9147"
#define ATK_RGBLCD_TOUCH_PID2               "9271"
#define ATK_RGBLCD_TOUCH_PID3               "1158"

/* RGB LCD模块最大触摸点数量 */
#define ATK_RGBLCD_TOUCH_TP_MAX             5

/* RGB LCD模块触摸部分寄存器定义 */
#define ATK_RGBLCD_TOUCH_REG_CTRL           0x8040  /* 控制寄存器 */
#define ATK_RGBLCD_TOUCH_REG_PID            0x8140  /* PID寄存器 */
#define ATK_RGBLCD_TOUCH_REG_TPINFO         0x814E  /* 触摸状态寄存器 */
#define ATK_RGBLCD_TOUCH_REG_TP1            0x8150  /* 触摸点1数据寄存器 */
#define ATK_RGBLCD_TOUCH_REG_TP2            0x8158  /* 触摸点2数据寄存器 */
#define ATK_RGBLCD_TOUCH_REG_TP3            0x8160  /* 触摸点3数据寄存器 */
#define ATK_RGBLCD_TOUCH_REG_TP4            0x8168  /* 触摸点4数据寄存器 */
#define ATK_RGBLCD_TOUCH_REG_TP5            0x8170  /* 触摸点5数据寄存器 */

/* 触摸状态寄存器掩码 */
#define ATK_RGBLCD_TOUCH_TPINFO_MASK_STA    0x80
#define ATK_RGBLCD_TOUCH_TPINFO_MASK_CNT    0x0F

/* RGB LCD模块触摸点数据寄存器 */
static const uint16_t g_atk_rgblcd_touch_tp_reg[ATK_RGBLCD_TOUCH_TP_MAX] = {
    ATK_RGBLCD_TOUCH_REG_TP1,
    ATK_RGBLCD_TOUCH_REG_TP2,
    ATK_RGBLCD_TOUCH_REG_TP3,
    ATK_RGBLCD_TOUCH_REG_TP4,
    ATK_RGBLCD_TOUCH_REG_TP5,
};

/* RGB LCD模块触摸状态结构体 */
static struct
{
    atk_rgblcd_touch_iic_addr_t iic_addr;
} g_atk_rgblcd_touch_sta;

static void delay_ms(uint32_t t)
{
	uint32_t i;
	for(i=0; i<t; i++)
	{
		__IO uint32_t timecount = 300000;
		while(0 != timecount--);
	}
}


static void atk_rgblcd_touch_hw_init(void)
{
	rcu_periph_clock_enable(RCU_GPIOD);

	//INT E3
    gpio_mode_set(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_11);
	//RST G3
    //gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_3);
    //gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
	//gpio_bit_set(GPIOD, GPIO_PIN_3);
}

/**
static void atk_rgblcd_touch_hw_reset(atk_rgblcd_touch_iic_addr_t addr)
{
	//INT E3
    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_3);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
	//gpio_bit_set(GPIOE, GPIO_PIN_3);
    
    switch (addr)
    {
        case ATK_RGBLCD_TOUCH_IIC_ADDR_14:
        {
            ATK_RGBLCD_TOUCH_TCS(0);
            ATK_RGBLCD_TOUCH_PEN(0);
            ATK_RGBLCD_TOUCH_PEN(1);
            delay_ms(1);
            ATK_RGBLCD_TOUCH_TCS(1);
            delay_ms(10);
            break;
        }
        case ATK_RGBLCD_TOUCH_IIC_ADDR_5D:
        {
            ATK_RGBLCD_TOUCH_TCS(0);
            ATK_RGBLCD_TOUCH_PEN(0);
            delay_ms(1);
            ATK_RGBLCD_TOUCH_TCS(1);
            delay_ms(10);
            break;
        }
        default:
        {
            break;
        }
    }
    
	//INT E3
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    
    g_atk_rgblcd_touch_sta.iic_addr = addr;
    delay_ms(100);
}*/

/**
 * @brief       写RGB LCD模块触摸寄存器
 * @param       reg: 待写寄存器地址
 *              buf: 待写入的数据
 *              len: 待写入数据的长度
 * @retval      ATK_RGBLCD_TOUCH_EOK  : 写ATK-RGBLCD模块触摸寄存器成功
 *              ATK_RGBLCD_TOUCH_ERROR: 写ATK-RGBLCD模块触摸寄存器失败
 */
static uint8_t atk_rgblcd_touch_write_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t buf_index;
    uint8_t ret;
    
    atk_rgblcd_touch_iic_start();
    atk_rgblcd_touch_iic_send_byte((g_atk_rgblcd_touch_sta.iic_addr << 1) | ATK_RGBLCD_TOUCH_IIC_WRITE);
    atk_rgblcd_touch_iic_wait_ack();
    atk_rgblcd_touch_iic_send_byte((uint8_t)(reg >> 8) & 0xFF);
    atk_rgblcd_touch_iic_wait_ack();
    atk_rgblcd_touch_iic_send_byte((uint8_t)reg & 0xFF);
    atk_rgblcd_touch_iic_wait_ack();
    
    for (buf_index=0; buf_index<len; buf_index++)
    {
        atk_rgblcd_touch_iic_send_byte(buf[buf_index]);
        ret = atk_rgblcd_touch_iic_wait_ack();
        if (ret != 0)
        {
            break;
        }
    }
    
    atk_rgblcd_touch_iic_stop();
    
    if (ret != 0)
    {
        return ATK_RGBLCD_TOUCH_ERROR;
    }
    
    return ATK_RGBLCD_TOUCH_EOK;
}

/**
 * @brief       读RGB LCD模块触摸寄存器
 * @param       reg: 待读寄存器地址
 *              buf: 读取的数据
 *              len: 待读取数据的长度
 * @retval      无
 */
static void atk_rgblcd_touch_read_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t buf_index;
    
    atk_rgblcd_touch_iic_start();
    atk_rgblcd_touch_iic_send_byte((g_atk_rgblcd_touch_sta.iic_addr << 1) | ATK_RGBLCD_TOUCH_IIC_WRITE);
    atk_rgblcd_touch_iic_wait_ack();
    atk_rgblcd_touch_iic_send_byte((uint8_t)(reg >> 8) & 0xFF);
    atk_rgblcd_touch_iic_wait_ack();
    atk_rgblcd_touch_iic_send_byte((uint8_t)reg & 0xFF);
    atk_rgblcd_touch_iic_wait_ack();
    atk_rgblcd_touch_iic_start();
    atk_rgblcd_touch_iic_send_byte((g_atk_rgblcd_touch_sta.iic_addr << 1) | ATK_RGBLCD_TOUCH_IIC_READ);
    atk_rgblcd_touch_iic_wait_ack();
    
    for (buf_index=0; buf_index<len - 1; buf_index++)
    {
        buf[buf_index] = atk_rgblcd_touch_iic_recv_byte(1);
    }
    
    buf[buf_index] = atk_rgblcd_touch_iic_recv_byte(0);
    
    atk_rgblcd_touch_iic_stop();
}

/**
 * @brief       RGB LCD模块触摸软件复位
 * @param       无
 * @retval      无
 */
static void atk_rgblcd_touch_sw_reset(void)
{
    uint8_t dat;
    
    dat = 0x02;
    atk_rgblcd_touch_write_reg(ATK_RGBLCD_TOUCH_REG_CTRL, &dat, 1);
    delay_ms(10);
    
    dat = 0x00;
    atk_rgblcd_touch_write_reg(ATK_RGBLCD_TOUCH_REG_CTRL, &dat, 1);
}

/**
 * @brief       获取RGB LCD模块触摸的PID
 * @param       pid: 获取到的PID（ASCII）
 * @retval      无
 */
static void atk_rgblcd_touch_get_pid(char *pid)
{
    atk_rgblcd_touch_read_reg(ATK_RGBLCD_TOUCH_REG_PID, (uint8_t *)pid, 4);
    pid[4] = '\0';
}

/**
 * @brief       RGB LCD模块触摸初始化
 * @param       无
 * @retval      ATK_RGBLCD_TOUCH_EOK  : RGB LCD模块触摸初始化成功
 *              ATK_RGBLCD_TOUCH_ERROR: RGB LCD模块触摸初始化失败
 */
uint8_t atk_rgblcd_touch_init(void)
{
    char pid[5];
    
    atk_rgblcd_touch_hw_init();
    //atk_rgblcd_touch_hw_reset(ATK_RGBLCD_TOUCH_IIC_ADDR_14);
	g_atk_rgblcd_touch_sta.iic_addr = ATK_RGBLCD_TOUCH_IIC_ADDR_5D;
    atk_rgblcd_touch_iic_init();
    atk_rgblcd_touch_get_pid(pid);
    if ((strcmp(pid, ATK_RGBLCD_TOUCH_PID) != 0) && (strcmp(pid, ATK_RGBLCD_TOUCH_PID1) != 0) && (strcmp(pid, ATK_RGBLCD_TOUCH_PID2) != 0) && (strcmp(pid, ATK_RGBLCD_TOUCH_PID3) != 0))
    {
        return ATK_RGBLCD_TOUCH_ERROR;
    }
    atk_rgblcd_touch_sw_reset();
    
    return ATK_RGBLCD_TOUCH_EOK;
}

/**
 * @brief       RGB LCD模块触摸扫描
 * @note        连续调用间隔需大于10ms
 * @param       point: 扫描到的触摸点信息
 *              cnt  : 需要扫描的触摸点数量（1~ATK_RGBLCD_TOUCH_TP_MAX）
 * @retval      0   : 没有扫描到触摸点
 *              其他: 实际获取到的触摸点信息数量
 */
uint8_t atk_rgblcd_touch_scan(lcd_touch_point_t *point, uint8_t cnt)
{
    uint8_t tp_info;
    uint8_t tp_cnt;
    uint8_t point_index;
    //atk_rgblcd_lcd_disp_dir_t dir;
    uint8_t tpn_info[6];
    lcd_touch_point_t point_raw;
    
    if ((cnt == 0) || (cnt > ATK_RGBLCD_TOUCH_TP_MAX))
    {
        return 0;
    }
    
    for (point_index=0; point_index<cnt; point_index++)
    {
        if (&point[point_index] == NULL)
        {
            return 0;
        }
    }
    
    atk_rgblcd_touch_read_reg(ATK_RGBLCD_TOUCH_REG_TPINFO, &tp_info, sizeof(tp_info));
    if ((tp_info & ATK_RGBLCD_TOUCH_TPINFO_MASK_STA) == ATK_RGBLCD_TOUCH_TPINFO_MASK_STA)
    {
        tp_cnt = tp_info & ATK_RGBLCD_TOUCH_TPINFO_MASK_CNT;
        tp_cnt = (cnt < tp_cnt) ? cnt : tp_cnt;
        
        for (point_index=0; point_index<tp_cnt; point_index++)
        {
            atk_rgblcd_touch_read_reg(g_atk_rgblcd_touch_tp_reg[point_index], tpn_info, sizeof(tpn_info));
            point_raw.x = (uint16_t)(tpn_info[1] << 8) | tpn_info[0];
            point_raw.y = (uint16_t)(tpn_info[3] << 8) | tpn_info[2];
            point_raw.size = (uint16_t)(tpn_info[5] << 8) | tpn_info[4];
            
            /*dir = atk_rgblcd_get_disp_dir();
            switch (dir)
            {
                case ATK_RGBLCD_LCD_DISP_DIR_0:
                {*/
                    point[point_index].x = point_raw.x;
                    point[point_index].y = point_raw.y;
                    /*break;
                }
                case ATK_RGBLCD_LCD_DISP_DIR_90:
                {
                    point[point_index].x = point_raw.y;
                    point[point_index].y = atk_rgblcd_get_lcd_height() - point_raw.x;
                    break;
                }
                case ATK_RGBLCD_LCD_DISP_DIR_180:
                {
                    point[point_index].x = atk_rgblcd_get_lcd_width() - point_raw.x;
                    point[point_index].y = atk_rgblcd_get_lcd_height() - point_raw.y;
                    break;
                }
                case ATK_RGBLCD_LCD_DISP_DIR_270:
                {
                    point[point_index].x = atk_rgblcd_get_lcd_width() - point_raw.y;
                    point[point_index].y = point_raw.x;
                    break;
                }
            }*/
            
            point[point_index].size = point_raw.size;
        }
        
        tp_info = 0;
        atk_rgblcd_touch_write_reg(ATK_RGBLCD_TOUCH_REG_TPINFO, &tp_info, sizeof(tp_info));
        
        return tp_cnt;
    }
    else
    {
        return 0;
    }
}

