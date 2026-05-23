#include "ui.h"
#include "audio.h"

lv_obj_t *ui_load_screen_create(void);
lv_obj_t *ui_main_screen_create(void);
void ui_main_handle_event(event_t *event);
void ui_main_update_timer_init(void);
// 游戏逻辑推进开关
void game_start(void);
void game_pause(void);

static lv_obj_t *g_screen_load = NULL;
static lv_obj_t *g_screen_main = NULL;

// UI入口
void ui_init(void) {
    icon_init(); // 初始化图标资源

    g_screen_load = ui_load_screen_create();
    g_screen_main = ui_main_screen_create();

    ui_screen_switch(UI_SCREEN_LOAD); // 进入加载页面

    bgm_music_play(); // 进入游戏时播放bgm
}

// 切换界面
void ui_screen_switch(ui_screen_type_t screen) {
    switch (screen) {
        case UI_SCREEN_LOAD:
            if (g_screen_load) {
                // 进入加载界面时暂停游戏逻辑推进
                game_pause();
                lv_scr_load(g_screen_load);
            }
            break;
        case UI_SCREEN_MAIN:
            if (g_screen_main) {
                // 进入主界面时恢复游戏逻辑推进
                game_start();
                lv_scr_load(g_screen_main);
            }
            break;
        default:
            if (g_screen_main) {
                // 默认也按主界面处理，避免逻辑停在暂停状态
                game_start();
                lv_scr_load(g_screen_main);
            }
            break;
    }
}

// 事件分发入口
void ui_event_handler(event_t *event) {
    if (!event) {
        return;
    }

    ui_main_handle_event(event);
}

// UI更新定时器统一初始化
void ui_update_timer_init(void) {
    ui_main_update_timer_init();
}
