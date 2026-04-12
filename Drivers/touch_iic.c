

#include "touch_iic.h"



/**
 * @brief       IIC接口延时函数，用于控制IIC读写速度
 * @param       无
 * @retval      无
 */
static void atk_rgblcd_touch_iic_delay(void)
{
	__IO uint32_t timecount = 600;
	while(0 != timecount--);
}

/**
 * @brief       初始化IIC接口
 * @param       无
 * @retval      无
 */
void atk_rgblcd_touch_iic_init(void)
{
	
	rcu_periph_clock_enable(RCU_GPIOD);

	//SCL D12
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_12);
	gpio_bit_set(GPIOD, GPIO_PIN_12);
	//SDA D13
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_13);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, GPIO_PIN_13);
	gpio_bit_set(GPIOD, GPIO_PIN_13);
	

    
    atk_rgblcd_touch_iic_stop();
}

/**
 * @brief       产生IIC起始信号
 * @param       无
 * @retval      无
 */
void atk_rgblcd_touch_iic_start(void)
{
    ATK_RGBLCD_TOUCH_IIC_SDA(1);
    ATK_RGBLCD_TOUCH_IIC_SCL(1);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SDA(0);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(0);
    atk_rgblcd_touch_iic_delay();
}

/**
 * @brief       产生IIC停止信号
 * @param       无
 * @retval      无
 */
void atk_rgblcd_touch_iic_stop(void)
{
    ATK_RGBLCD_TOUCH_IIC_SDA(0);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(1);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SDA(1);
    atk_rgblcd_touch_iic_delay();
}

/**
 * @brief       等待IIC应答信号
 * @param       无
 * @retval      0: 应答信号接收成功
 *              1: 应答信号接收失败
 */
uint8_t atk_rgblcd_touch_iic_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;
    
    ATK_RGBLCD_TOUCH_IIC_SDA(1);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(1);
    atk_rgblcd_touch_iic_delay();
    
    while (ATK_RGBLCD_TOUCH_IIC_READ_SDA())
    {
        waittime++;
        
        if (waittime > 250)
        {
            atk_rgblcd_touch_iic_stop();
            rack = 1;
            break;
        }
    }
    
    ATK_RGBLCD_TOUCH_IIC_SCL(0);
    atk_rgblcd_touch_iic_delay();
    
    return rack;
}

/**
 * @brief       产生ACK应答信号
 * @param       无
 * @retval      无
 */
void atk_rgblcd_touch_iic_ack(void)
{
    ATK_RGBLCD_TOUCH_IIC_SDA(0);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(1);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(0);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SDA(1);
    atk_rgblcd_touch_iic_delay();
}

/**
 * @brief       不产生ACK应答信号
 * @param       无
 * @retval      无
 */
void atk_rgblcd_touch_iic_nack(void)
{
    ATK_RGBLCD_TOUCH_IIC_SDA(1);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(1);
    atk_rgblcd_touch_iic_delay();
    ATK_RGBLCD_TOUCH_IIC_SCL(0);
    atk_rgblcd_touch_iic_delay();
}

/**
 * @brief       IIC发送一个字节
 * @param       dat: 要发送的数据
 * @retval      无
 */
void atk_rgblcd_touch_iic_send_byte(uint8_t dat)
{
    uint8_t t;
    
    for (t=0; t<8; t++)
    {
        ATK_RGBLCD_TOUCH_IIC_SDA((dat & 0x80) >> 7);
        atk_rgblcd_touch_iic_delay();
        ATK_RGBLCD_TOUCH_IIC_SCL(1);
        atk_rgblcd_touch_iic_delay();
        ATK_RGBLCD_TOUCH_IIC_SCL(0);
        dat <<= 1;
    }
    ATK_RGBLCD_TOUCH_IIC_SDA(1);
}

/**
 * @brief       IIC接收一个字节
 * @param       ack: ack=1时，发送ack; ack=0时，发送nack
 * @retval      接收到的数据
 */
uint8_t atk_rgblcd_touch_iic_recv_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t dat = 0;
    
    for (i = 0; i < 8; i++ )
    {
        dat <<= 1;
        ATK_RGBLCD_TOUCH_IIC_SCL(1);
        atk_rgblcd_touch_iic_delay();
        
        if (ATK_RGBLCD_TOUCH_IIC_READ_SDA())
        {
            dat++;
        }
        
        ATK_RGBLCD_TOUCH_IIC_SCL(0);
        atk_rgblcd_touch_iic_delay();
    }
    
    if (ack == 0)
    {
        atk_rgblcd_touch_iic_nack();
    }
    else
    {
        atk_rgblcd_touch_iic_ack();
    }

    return dat;
}


