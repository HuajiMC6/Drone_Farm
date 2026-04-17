#include "drivers.h"
#include "drone.h"
#include "farm.h"
#include "joystick.h"
#include "player.h"
#include "ui.h"

void heartbeat_timer_cb(lv_timer_t *timer);

int main() {
    sys_init();

    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_bit_reset(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    rcu_periph_clock_enable(RCU_TRNG);
    trng_deinit();
    trng_mode_config(TRNG_MODSEL_NIST);
    trng_clockerror_detection_enable();
    trng_enable();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* Farm Instance Initialization */
    farm_init();
    player_init();
    drone_init();
    /* UI Initializaiton */
    ui_init();
    ui_update_timer_init();
    /* Heartbeat Timer Init */
    lv_timer_create(heartbeat_timer_cb, 1000, NULL);
    /* Random Init */
    while (trng_flag_get(TRNG_FLAG_DRDY) == RESET)
        ;
    srand(trng_get_true_random_data());
    /* Joystick Init */
    joystick_init();

    while (1) {
        delay_us(2000);
        lv_timer_handler();

        ui_event_handler(event_get());
    }
}

void heartbeat_timer_cb(lv_timer_t *timer) {
    farm_grow();
}
