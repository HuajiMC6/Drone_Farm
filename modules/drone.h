#ifndef DRONE_H
#define DRONE_H
#include "data.h"
#include "farm.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int x;
    int y;
} pos_t;

typedef struct {
    int speed_level; // 0 1 2 3
    int speed;
    int algorithm_level; // 0全部遍历,1贪心,2贪心+2‑opt优化 //算法只规划路径，药不管
    // 存储无人机检测到的田地虫害类型，不对应真实虫害情况，因为无人机只有检测后才知道，故该数据是落后于真实情况的
    crop_damage_t one_zero_matrix[GAME_GRID_SIZE][GAME_GRID_SIZE];
    int pesticide_storage[CROP_PESTICIDE_NONE]; // 0 1 2 3与枚举类型对应
    int storage_capacity;
    int storage_level; // 0 1 2 3
    pos_t current_pos;
    drone_state_t drone_state;
} drone_t;

drone_t *drone_get_instance();
void drone_init();
void drone_state_switch(drone_state_t drone_state);

crop_damage_t drone_detect_damage();
void drone_get_detected_pest_counts(uint8_t counts[CROP_DAMAGE_NONE]);

bool drone_algorithm_update();
bool drone_speed_update();
bool drone_storage_update();

pos_t *drone_auto_path(int *out_len);

bool drone_ensure_pesticide(pos_t pos);
bool drone_add_pesticide(crop_pesticide_t pesticide, int n);
bool drone_remove_pesticide(crop_pesticide_t pesticide, int n);

// joystick_move
void drone_move(pos_t vector);

bool drone_save();
bool drone_load();
bool drone_delete();

#endif