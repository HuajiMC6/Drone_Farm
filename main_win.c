/**
 * @file    main_win.c
 * @brief   Windows 版主入口 — 替代嵌入式 main.c
 *
 * 与嵌入式版 main.c 的关键区别:
 *   1. 不初始化 MCU 硬件 (时钟/SDRAM/DMA/TRNG 等)
 *   2. 使用 SDL2 替代 TLI 显示 + I2C 触摸 + SAI2 音频
 *   3. 使用 Windows 文件 I/O 替代 SDIO/SD 卡
 *   4. 随机数使用 stdlib rand() (srand 用 time())
 *   5. 主循环使用 SDL 事件模型而非裸机轮询
 */

#include "win_port/audio_sdl.h"
#include "win_port/compat.h"
#include "win_port/display_sdl.h"
#include "win_port/input_sdl.h"
#include "win_port/joystick_win.h"
#include "win_port/resource_embed.h"
#include "win_port/system_win.h"

/* ---- 嵌入式游戏逻辑 (完全复用) ---- */
#include "drone.h"
#include "event.h"
#include "farm.h"
#include "lvgl.h"
#include "player.h"
#include "ui.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ================================================================
 *  心跳定时器 (LVGL timer, 1秒周期)
 *  与嵌入式 main.c 完全一致
 * ================================================================ */

static lv_timer_t *g_heartbeat_timer = NULL;

static void heartbeat_timer_cb(lv_timer_t *timer) {
    (void)timer;
    farm_grow();
    farm_save();
    player_save();
    drone_save();
}

static void heartbeat_timer_init(void) {
    if (!g_heartbeat_timer) {
        g_heartbeat_timer = lv_timer_create(heartbeat_timer_cb, 1000, NULL);
    }
}

void game_start(void) {
    if (g_heartbeat_timer)
        lv_timer_resume(g_heartbeat_timer);
}

void game_pause(void) {
    if (g_heartbeat_timer)
        lv_timer_pause(g_heartbeat_timer);
}

void debug_heartbeat_timer_set_period(uint32_t period_ms) {
    if (g_heartbeat_timer) {
        lv_timer_set_period(g_heartbeat_timer, period_ms);
    }
}

/* ================================================================
 *  LVGL tick 回调 (SDL timer)
 * ================================================================ */

static Uint32 lvgl_tick_callback(Uint32 interval, void *param) {
    (void)param;
    lv_tick_inc((uint32_t)interval);
    return interval;
}

/* ================================================================
 *  main()
 * ================================================================ */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    /* ---- 0. SDL 统一初始化 (必须在任何 SDL 调用前) ---- */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "[FATAL] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* ---- 1. 系统初始化 (FATFS 挂载) ---- */
    sys_init();

    /* ---- 1.5. 首次启动: 将嵌入式资源写入 SD 镜像 ---- */
    resource_install_all();

    /* ---- 2. 随机数种子 (替代 TRNG 硬件) ---- */
    srand((unsigned int)time(NULL));

    /* ---- 3. LVGL 核心初始化 ---- */
    lv_init();

    /* ---- 4. 显示驱动 (SDL2) ---- */
    lv_port_disp_init();

    /* ---- 5. 输入驱动 (SDL2 鼠标 → 触摸) ---- */
    lv_port_indev_init();

    /* ---- 6. 游戏逻辑初始化 ---- */
    farm_init();
    player_init();
    drone_init();

    /* ---- 7. 音频初始化 (SDL2) ---- */
    speaker_init();

    /* ---- 8. UI 初始化 (可能触发 BGM) ---- */
    ui_init();

    /* ---- 9. 心跳定时器 (1秒) ---- */
    heartbeat_timer_init();
    game_pause(); /* LOAD 界面暂停, 进入主界面后恢复 */

    /* ---- 10. 摇杆初始化 (键盘模拟) ---- */
    joystick_init();

    /* ---- 11. LVGL tick 定时器 (SDL timer) ---- */
    SDL_TimerID tick_timer = SDL_AddTimer(1, lvgl_tick_callback, NULL);

    /* ================================================================
     *  主循环
     * ================================================================ */
    printf("[INFO] Entering main loop...\n");

    int running = 1;
    while (running) {
        /* 处理 SDL 事件 */
        if (lv_port_indev_handle_sdl_events()) {
            running = 0;
            break;
        }

        /* 音频更新 (填充流缓冲) */
        speaker_update();

        /* LVGL 渲染一帧 */
        lv_timer_handler();

        /* 处理游戏事件队列 */
        event_t *e;
        while ((e = event_get()) != NULL) {
            ui_event_handler(e);
        }

        /* 提交 LVGL 渲染到屏幕 */
        lv_port_disp_render();

        /* 帧率限制 ~125 Hz (8ms) */
        SDL_Delay(2);

        /* 补充音频 (渲染后) */
        speaker_update();
    }

    /* ---- 清理 ---- */
    SDL_RemoveTimer(tick_timer);
    speaker_stop();
    SDL_Quit();

    printf("[INFO] Clean exit.\n");
    return 0;
}
