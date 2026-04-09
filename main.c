#include "drivers.h"
#include "ui.h"
#include "farm.h"

#include <stdlib.h>
#include <time.h>

void heartbeat_timer_cb(lv_timer_t *timer);

int main()
{
    sys_init();

    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_bit_reset(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
	
	/* Farm instance initialization */
	farm_init();
	/* UI Initializaiton */
	ui_init();
	ui_update_timer_init();
	/* Heartbeat timer init */
	lv_timer_create(heartbeat_timer_cb, 1000, NULL);
	/* Random init */
	srand((unsigned int) SysTick->VAL);
	 
    while (1)
    {
        delay_us(2000);
        lv_timer_handler();
    }
}


void heartbeat_timer_cb(lv_timer_t *timer) {
	farm_grow(farm_get_instance());
}
