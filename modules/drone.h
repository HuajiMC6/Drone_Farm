#ifndef DRONE_H
#define DRONE_H
#include "enum.h"
#include "farm.h"
#include "player.h"
#include <stdbool.h>

typedef struct{
    int x;
    int y;
}pos_t;

typedef struct{
    int speed_level;//0 1 2 3
    int speed;
    int algorithm_level;//0全部遍历,1贪心,2贪心+2‑opt优化 //算法只规划路径，药不管
    int one_zero_matrix[10][10];
    int pesticide_storage[CROP_PESTICIDE_NONE];//0 1 2 3与枚举类型对应
    int storage_capacity;
    int storage_level;//0 1 2 3
    pos_t current_pos;
}drone_t;

drone_t *drone_get_instance();
void drone_init();

crop_damage_t get_damage_information();

bool drone_algorithm_update();
bool drone_speed_update();
bool drone_storage_update();

pos_t* auto_path(int *out_len);

bool drone_ensure_pesticide(pos_t pos);
bool drone_add_pesticide(crop_pesticide_t pesticide,int n);

//joystick_move
void drone_move(pos_t vector);

#endif