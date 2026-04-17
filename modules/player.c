#include "player.h"
#include "enum.h"
#include "event.h"
#include <stdbool.h>
#include <stdlib.h>

static player_t *s_player = NULL;

static void player_set_coins(int coins);
static void player_set_experience(int experience);
static void player_level_update();

void player_init() {
    if (s_player == NULL) {
        s_player = (player_t *)malloc(sizeof(player_t));
        s_player->level = 0;
        s_player->level_stage = 0;
        s_player->experience = 0;
        s_player->coins = 0;
        for (int i = 0; i < CROP_TYPE_NONE; i++)
            s_player->seed_bag[i] = s_player->harvest_bag[i] = 0;
    }
}

player_t *player_get_instance() {
    return s_player;
}

bool player_buy_seed(crop_type_t seed_type, int n) {
    int total_price = n * seed_price[seed_type] * level_discount[s_player->level_stage];
    if (s_player->coins >= total_price) {
        s_player->seed_bag[seed_type] += n;
        player_set_coins(s_player->coins - total_price);
        return true;
    }
    return false;
}

bool player_buy_pesticide(crop_pesticide_t pesticide_type, int n) {
    int total_price = n * pesticide_price[pesticide_type] * level_discount[s_player->level_stage];
    if (s_player->coins >= total_price) {
        if (drone_add_pesticide(pesticide_type, n)) {
            player_set_coins(s_player->coins - total_price);
            return true;
        }
    }
    return false;
}

bool player_plant(field_t *field, crop_type_t crop_type) {
    if (s_player->seed_bag[crop_type] > 0) {
        if (field_plant(field, crop_type)) {
            s_player->seed_bag[crop_type]--;
            player_set_experience(s_player->experience + plant_exp_earn);
            return true;
        }
    }
    return false;
}

bool player_harvest(field_t *field) {
    if (field->stage == CROP_STAGE_READY) {
        crop_type_t crop_type = field->crop_type;
        int output = field_harvest(field);
        s_player->harvest_bag[crop_type] += output;
        player_set_experience(s_player->experience + output * harvest_exp_earn[crop_type]);
        return true;
    }
    return false;
}

bool player_sold(crop_type_t crop_type, int n) {
    if (s_player->harvest_bag[crop_type] >= n) {
        player_set_coins(s_player->coins + n * harvest_price[crop_type]);
        s_player->harvest_bag[crop_type] -= n;
        return true;
    }
    return false;
}

void player_use_pesticide_exp() {
    player_set_experience(s_player->experience + use_pesticide_exp_earn);
}

// 购买升级
// drone update
bool player_buy_drone_speed_update() {
    drone_t *drone = drone_get_instance();
    if (drone->speed_level >= 3)
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

bool player_buy_drone_storage_update() {
    drone_t *drone = drone_get_instance();
    if (drone->storage_level >= 3)
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

bool player_buy_drone_algorithm_update() {
    drone_t *drone = drone_get_instance();
    if (drone->algorithm_level >= 2)
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
bool player_buy_farm_size_update() {
    farm_t *farm = farm_get_instance();
    if (farm->size_level >= 3)
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
bool player_buy_field_output_upgrade(pos_t pos) {
    farm_t *farm = farm_get_instance();
    field_t *field = farm->fields[pos.x][pos.y];
    if (field->output_level >= 3)
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

bool player_buy_field_ready_time_upgrade(pos_t pos) {
    farm_t *farm = farm_get_instance();
    field_t *field = farm->fields[pos.x][pos.y];
    if (field->ready_time_level >= 3)
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

bool player_buy_field_tolerance_upgrade(pos_t pos) {
    farm_t *farm = farm_get_instance();
    field_t *field = farm->fields[pos.x][pos.y];
    if (field->tolerance_level >= 3)
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

static void player_set_coins(int coins) {
    s_player->coins = coins;
    event_send(EVENT_ON_PLAYER_COIN_CHANGE, s_player);
}

static void player_set_experience(int experience) {
    s_player->experience = experience;
    player_level_update();
    event_send(EVENT_ON_PLAYER_EXPERIENCE_CHANGE, s_player);
}

static void player_level_update() { // 每次碰到与经验相关操作都调用
    while (s_player->level < 40 && s_player->experience >= experience_level[s_player->level]) {
        s_player->level++;
        event_send(EVENT_ON_PLAYER_LEVEL_UPGRADE, s_player);
    }
    if (s_player->level < 10)
        s_player->level_stage = 0;
    else if (s_player->level < 15)
        s_player->level_stage = 1;
    else if (s_player->level < 20)
        s_player->level_stage = 2;
    else if (s_player->level < 25)
        s_player->level_stage = 3;
    else if (s_player->level < 30)
        s_player->level_stage = 4;
    else if (s_player->level < 40)
        s_player->level_stage = 5;
    else if (s_player->level >= 40)
        s_player->level_stage = 6;
};

int harvest_exp_earn[CROP_TYPE_NONE] = {2, 3, 4};

int plant_exp_earn = 2;
int use_pesticide_exp_earn = 5;

int experience_level[40] = { // 1-40级总经验-等级表
    10,  25,  40,  55,  75,  95,  120, 150, 180,  210,  240,  270,  300,  330,  360,  390,  420,  450,  480,  550,
    600, 650, 700, 750, 800, 850, 900, 950, 1000, 1060, 1130, 1210, 1300, 1400, 1510, 1630, 1760, 1900, 2050, 2210};