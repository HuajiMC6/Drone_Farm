#ifndef DRONE_H
#define DRONE_H
#include "enum.h"
#include "farm.h"

typedef struct{
    int x;
    int y;
}pos_t;

typedef struct{
    int speed_level;
    int speed;
    int algorithm_level;//0全部遍历,1贪心,2贪心+2‑opt优化 //算法只规划路径，药不管
    int one_zero_matrix[10][10];
    int pesticide_storage[4];//0 1 2 3与枚举类型对应
}drone_t;

drone_t *get_drone_instance();
void drone_init();

crop_damage_t get_damage_information(pos_t pos);

void algorithm_update();
void speed_update();

pos_t* auto_path(int *out_len);

void ensure_pesticide(pos_t pos);
void add_pesticide(crop_pesticide_t pesticide);

#endif