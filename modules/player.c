#include "player.h"
#include "data.h"
#include "event.h"
#include "ff.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static player_t s_player_storage;
static player_t *s_player = NULL;

static void player_set_coins(int coins);
static void player_set_experience(int experience);
static void player_level_update();

// 玩家模块初始化：恢复存档或创建新档
void player_init() {
    if (s_player != NULL) {
        return;
    }

    s_player = &s_player_storage;
    // 先绑定静态玩家对象，再尝试从 SD 卡恢复存档
    if (player_load()) {
        return;
    }

    // 没有存档时使用默认新档数据
    memset(s_player, 0, sizeof(*s_player));
    s_player->level = 0;
    s_player->level_stage = 0;
    s_player->experience = 0;
    s_player->coins = player_starting_coins; // 500大洋启动资金
    for (int i = 0; i < CROP_TYPE_NONE; i++) s_player->seed_bag[i] = s_player->harvest_bag[i] = 0;
}

// 获取玩家单例实例，首次调用自动初始化
player_t *player_get_instance() {
    if (s_player == NULL) {
        player_init();
    }
    return s_player;
}

// 购买种子（扣除金币，加入种子背包）
bool player_buy_seed(crop_type_t seed_type, int n) {
    int total_price = n * seed_price[seed_type] * level_discount[s_player->level_stage];
    if (s_player->coins >= total_price) {
        s_player->seed_bag[seed_type] += n;
        player_set_coins(s_player->coins - total_price);
        event_send(EVENT_ON_PLAYER_SEED_CHANGE, s_player);
        return true;
    }
    return false;
}

// 购买农药（扣除金币，加入农药背包）
bool player_buy_pesticide(crop_pesticide_t pesticide_type, int n) {
    int total_price = n * pesticide_price[pesticide_type] * level_discount[s_player->level_stage];
    if (s_player->coins >= total_price) {
        s_player->pesticide_bag[pesticide_type] += n;
        player_set_coins(s_player->coins - total_price);
        event_send(EVENT_ON_PLAYER_PESTICIDE_CHANGE, s_player);
        return true;
    }
    return false;
}

// 在地块上种植：消耗一枚种子并触发种植
bool player_plant(field_t *field, crop_type_t crop_type) {
    if (s_player->seed_bag[crop_type] > 0) {
        if (field_plant(field, crop_type)) {
            s_player->seed_bag[crop_type]--;
            event_send(EVENT_ON_PLAYER_SEED_CHANGE, s_player);
            player_set_experience(s_player->experience + plant_exp_earn);
            return true;
        }
    }
    return false;
}

// 收获单个地块的成熟作物，产物加入收获背包
bool player_harvest(field_t *field, int *p_output) {
    if (field->stage == CROP_STAGE_READY) {
        crop_type_t crop_type = field->crop_type;
        int output = field_harvest(field);
        s_player->harvest_bag[crop_type] += output;
        player_set_experience(s_player->experience + output * harvest_exp_earn[crop_type]);
        event_send(EVENT_ON_PLAYER_HARVEST_BAG_CHANGE, s_player);

        if (p_output) {
            *p_output = output;
        }

        return true;
    }
    return false;
}

// 一键收获全农场所有成熟作物
bool player_harvest_all(int outputs[CROP_TYPE_NONE]) {
    farm_t *farm = farm_get_instance();
    int count = 0;
    for (int i = 0; i < farm->current_size; i++)
        for (int j = 0; j < farm->current_size; j++) {
            field_t *field = farm->fields[i][j];
            crop_type_t crop_type = field->crop_type; // if已经将croptype变成了NONE，得先读取
            int output = 0;
            if (player_harvest(field, &output))
                outputs[crop_type] += output, count++;
        }
    return count != 0; // 仅代表是否收获了作物不代表出不出错
}

// 出售收获背包中的作物换取金币
bool player_sold(crop_type_t crop_type, int n, int *total_earning) {
    if (s_player->harvest_bag[crop_type] >= n) {
        *total_earning = n * harvest_price[crop_type];
        player_set_coins(s_player->coins + *total_earning);
        s_player->harvest_bag[crop_type] -= n;
        event_send(EVENT_ON_PLAYER_HARVEST_BAG_CHANGE, s_player);
        return true;
    }
    return false;
}

// 增加使用农药获得的经验值
void player_use_pesticide_exp() {
    player_set_experience(s_player->experience + use_pesticide_exp_earn);
}

// 购买升级
// drone update
// 购买无人机速度升级
bool player_buy_drone_speed_update() {
    drone_t *drone = drone_get_instance();
    if (drone->speed_level >= DRONE_SPEED_LEVEL_MAX)
        return false;
    int price = drone_speed_update_price[drone->speed_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (drone_speed_update()) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// 购买无人机容量升级
bool player_buy_drone_storage_update() {
    drone_t *drone = drone_get_instance();
    if (drone->storage_level >= DRONE_STORAGE_LEVEL_MAX)
        return false;
    int price = drone_storage_update_price[drone->storage_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (drone_storage_update()) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// 购买无人机算法升级
bool player_buy_drone_algorithm_update() {
    drone_t *drone = drone_get_instance();
    if (drone->algorithm_level >= DRONE_ALGORITHM_LEVEL_MAX)
        return false;
    int price = drone_algorithm_update_price[drone->algorithm_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (drone_algorithm_update()) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// farm update
// 购买农场大小升级
bool player_buy_farm_size_update() {
    farm_t *farm = farm_get_instance();
    if (farm->size_level >= FARM_SIZE_LEVEL_MAX)
        return false;
    int price = farm_size_update_price[farm->size_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (farm_size_update()) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// field update
// 购买地块产量升级
bool player_buy_field_output_upgrade(field_t *field) {
    if (field->output_level >= FIELD_UPGRADE_LEVEL_MAX)
        return false;
    int price = field_output_upgrade_price[field->output_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (field_output_upgrade(field)) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// 购买地块成熟速度升级
bool player_buy_field_ready_time_upgrade(field_t *field) {
    if (field->ready_time_level >= FIELD_UPGRADE_LEVEL_MAX)
        return false;
    int price = field_ready_time_upgrade_price[field->ready_time_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (field_ready_time_upgrade(field)) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// 购买地块耐虫性升级
bool player_buy_field_tolerance_upgrade(field_t *field) {
    if (field->tolerance_level >= FIELD_UPGRADE_LEVEL_MAX)
        return false;
    int price = field_tolerance_upgrade_price[field->tolerance_level] * level_discount[s_player->level_stage];
    if (s_player->coins >= price) {
        if (field_tolerance_upgrade(field)) {
            player_set_coins(s_player->coins - price);
            return true;
        }
    }
    return false;
}

// 设置金币并发送变更事件
static void player_set_coins(int coins) {
    s_player->coins = coins;
    event_send(EVENT_ON_PLAYER_COIN_CHANGE, s_player);
}

// 设置经验值并触发等级更新
static void player_set_experience(int experience) {
    s_player->experience = experience;
    player_level_update();
    event_send(EVENT_ON_PLAYER_EXPERIENCE_CHANGE, s_player);
}

// 获取玩家展示等级（从1开始）
int player_get_level() {
    return s_player->level + 1; // level从0开始计数，但对玩家展示时从1开始
}

// 获取玩家当前经验值
int player_get_experience() {
    return s_player->experience;
}

// 获取下一级所需经验值（满级返回-1）
int player_get_next_level_experience() {
    if (s_player->level >= PLAYER_EXPERIENCE_LEVELS - 1) {
        return -1; // 已满级
    }
    return experience_level[s_player->level];
}

// 获取当前等级起始经验值
int player_get_this_level_experience() {
    if (s_player->level == 0) {
        return 0;
    }
    return experience_level[s_player->level - 1];
}

static void player_level_update() { // 每次碰到与经验相关操作都调用
    while (s_player->level < PLAYER_EXPERIENCE_LEVELS && s_player->experience >= experience_level[s_player->level]) {
        s_player->level++;
        event_send(EVENT_ON_PLAYER_LEVEL_UPGRADE, s_player);
    }
    s_player->level_stage = PLAYER_LEVEL_STAGE_THRESHOLD_COUNT;
    for (int i = 0; i < PLAYER_LEVEL_STAGE_THRESHOLD_COUNT; i++) {
        if (s_player->level < player_level_stage_thresholds[i]) {
            s_player->level_stage = i;
            break;
        }
    }
};

// 将玩家状态保存到 SD 卡
bool player_save() {
    if (!s_player)
        return false;

    FIL fil;
    UINT bw;
    if (f_open(&fil, "0:/player_save.dat", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;

    if (f_write(&fil, s_player, sizeof(player_t), &bw) != FR_OK || bw != sizeof(player_t)) {
        f_close(&fil);
        return false;
    }

    f_close(&fil);
    return true;
}

// 从 SD 卡恢复玩家状态
bool player_load() {
    FIL fil;
    UINT br;
    if (f_open(&fil, "0:/player_save.dat", FA_READ) != FR_OK)
        return false;

    // 直接读回静态玩家存储区，避免再次进入 player_get_instance()
    if (!s_player) {
        s_player = &s_player_storage;
    }

    if (f_read(&fil, s_player, sizeof(player_t), &br) != FR_OK || br != sizeof(player_t)) {
        f_close(&fil);
        return false;
    }

    f_close(&fil);
    return true;
}

// 删除玩家存档文件
bool player_delete() {
    // 删除玩家存档文件；文件不存在时也视为已经删除成功
    FRESULT res = f_unlink("0:/player_save.dat");
    return res == FR_OK || res == FR_NO_FILE;
}
