#include "ui_setting_cb.h"
#include "audio.h"
#include "data.h"
#include "gd32h7xx.h"
#include "player.h"
#include "ui.h"
#include "ui_common.h"

// 重置游戏
void ui_setting_reset_game_cb(lv_event_t *e) {
    farm_delete();
    drone_delete();
    player_delete();

    // 重置后直接复位设备，重新初始化游戏状态
    NVIC_SystemReset();
}

// 增加金币（调试用）
void ui_setting_add_coins_cb(lv_event_t *e) {
    player_t *player = player_get_instance();
    if (player) {
        player->coins += 100000;
    }
}

// 增加等级（调试用）
void ui_setting_add_level_cb(lv_event_t *e) {
    player_t *player = player_get_instance();
    if (!player || player->level >= PLAYER_EXPERIENCE_LEVELS - 1)
        return;

    /* 经验拉到当前等级阈值，触发升级 */
    player->experience = experience_level[player->level];
    player->level++;

    /* 更新等级段 */
    player->level_stage = PLAYER_LEVEL_STAGE_THRESHOLD_COUNT;
    for (int i = 0; i < PLAYER_LEVEL_STAGE_THRESHOLD_COUNT; i++) {
        if (player->level < player_level_stage_thresholds[i]) {
            player->level_stage = i;
            break;
        }
    }
}

// 游戏速度调整
void ui_setting_game_speed_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **btns = (lv_obj_t **)lv_event_get_user_data(e);

    /* 通过 target 反查按钮索引 */
    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (btns[i] == target) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return;

    /* 倍率 → 心跳周期映射: 0.5x→2000ms, 1x→1000ms, 2x→500ms, 5x→200ms */
    static const uint32_t periods[] = {2000, 1000, 500, 200};
    debug_heartbeat_timer_set_period(periods[idx]);

    /* 更新按钮高亮 */
    for (int i = 0; i < 4; i++) {
        if (!btns[i])
            continue;
        if (i == idx) {
            lv_obj_add_style(btns[i], &ui_style_btn_yellow, 0);
        } else {
            lv_obj_remove_style(btns[i], &ui_style_btn_yellow, 0);
        }
    }
}

// 音量调整
void ui_setting_volume_slider_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_current_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);

    int value = lv_slider_get_value(slider);
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d%%", value);
    lv_label_set_text(label, buf);

    audio_set_volume(value);
}
