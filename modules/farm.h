#ifndef FARM_H
#define FARM_H
#include "enum.h"
#include "field.h"

typedef struct{//农场结构体
    field_t *fields[10][10];
    int current_size;//当前n数
    int size_level;//0 1 2 3
}farm_t;

int farm_init();
farm_t *farm_get_instance();
bool farm_size_update();
void farm_grow();
int get_farm_size();

#endif