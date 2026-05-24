#include "drivers.h"
#include "drone.h"
#include "farm.h"
#include "player.h"
#include "ui.h"

void heartbeat_timer_cb(lv_timer_t *timer);

static lv_timer_t *g_heartbeat_timer = NULL;

void game_start(void);
void game_pause(void);

int main() {
    sys_init();

    /* LED */
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_bit_reset(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    /* TRNG */
    rcu_periph_clock_enable(RCU_TRNG);
    trng_deinit();
    trng_mode_config(TRNG_MODSEL_NIST);
    trng_clockerror_detection_enable();
    trng_enable();

    /* LVGL Init */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    /* Random Init */
    while (trng_flag_get(TRNG_FLAG_DRDY) == RESET);
    srand(trng_get_true_random_data());

    /* Farm Instance Initialization */
    farm_init();
    player_init();
    drone_init();

    /* Speaker Init (must be before ui_init; ui_init may start BGM) */
    speaker_init();

    /* UI Initializaiton */
    ui_init();
    ui_update_timer_init();

    /* Heartbeat Timer Init */
    g_heartbeat_timer = lv_timer_create(heartbeat_timer_cb, 1000, NULL);
    // 默认先暂停，等进入主页面后再恢复
    game_pause();

    /* Joystick Init */
    joystick_init();

    while (1) {
        delay_us(2000);
        lv_timer_handler();

        // 扬声器播放状态更新 (DMA 缓冲区填充)
        speaker_update();

        // UI层处理游戏逻辑事件
        ui_event_handler(event_get());
    }
}

void heartbeat_timer_cb(lv_timer_t *timer) {
    farm_grow();

    // 定时保存游戏状态
    farm_save();
    player_save();
    drone_save();
}

void game_start(void) {
    // 恢复心跳计时器，让游戏逻辑开始推进
    if (g_heartbeat_timer) {
        lv_timer_resume(g_heartbeat_timer);
    }
}

void game_pause(void) {
    // 暂停心跳计时器，LOAD 界面期间不推进游戏逻辑
    if (g_heartbeat_timer) {
        lv_timer_pause(g_heartbeat_timer);
    }
}

void debug_heartbear_timer_set_period(uint32_t period_ms) {
    if (g_heartbeat_timer) {
        lv_timer_set_period(g_heartbeat_timer, period_ms);
    }
}