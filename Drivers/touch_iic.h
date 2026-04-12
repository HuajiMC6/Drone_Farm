#ifndef TOUCH_IIC_H
#define TOUCH_IIC_H

#include "gd32h7xx.h"



/* IO操作 */
#define ATK_RGBLCD_TOUCH_IIC_SCL(x)                 do{ x ?                                                                                                         \
                                                        gpio_bit_set(GPIOD, GPIO_PIN_12) :    \
                                                        gpio_bit_reset(GPIOD, GPIO_PIN_12);   \
                                                    }while(0)

#define ATK_RGBLCD_TOUCH_IIC_SDA(x)                 do{ x ?                                                                                                         \
                                                        gpio_bit_set(GPIOD, GPIO_PIN_13) :    \
                                                        gpio_bit_reset(GPIOD, GPIO_PIN_13);   \
                                                    }while(0)

#define ATK_RGBLCD_TOUCH_IIC_READ_SDA()             gpio_input_bit_get(GPIOD, GPIO_PIN_13)

/* 控制IIC通讯是读操作还是写操作 */
#define ATK_RGBLCD_TOUCH_IIC_WRITE                  0
#define ATK_RGBLCD_TOUCH_IIC_READ                   1

/* 错误代码 */
#define ATK_RGBLCD_TOUCH_IIC_EOK                    0   /* 没有错误 */
#define ATK_RGBLCD_TOUCH_IIC_ERROR                  1   /* 错误 */

/* 操作函数 */
void atk_rgblcd_touch_iic_init(void);                   /* 初始化IIC接口 */
void atk_rgblcd_touch_iic_start(void);                  /* 产生IIC起始信号 */
void atk_rgblcd_touch_iic_stop(void);                   /* 产生IIC停止信号 */
uint8_t atk_rgblcd_touch_iic_wait_ack(void);            /* 等待IIC应答信号 */
void atk_rgblcd_touch_iic_ack(void);                    /* 产生ACK应答信号 */
void atk_rgblcd_touch_iic_nack(void);                   /* 不产生ACK应答信号 */
void atk_rgblcd_touch_iic_send_byte(uint8_t dat);       /* IIC发送一个字节 */
uint8_t atk_rgblcd_touch_iic_recv_byte(uint8_t ack);    /* IIC接收一个字节 */

#endif


