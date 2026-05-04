#ifndef FARM_H
#define FARM_H
#include "enum.h"
#include "field.h"

typedef struct { // 农场结构体
    field_t *fields[10][10];
    int current_size; // 当前n数
    int size_level;   // 0 1 2 3
} farm_t;

void farm_init();
farm_t *farm_get_instance();
bool farm_size_update();
void farm_grow();
int get_farm_size();

//包含指针会出问题，毕竟重启后分配区域会变，只能依次存储数值依次读取数值（纯赋值）
bool farm_save();
bool farm_load();

#endif