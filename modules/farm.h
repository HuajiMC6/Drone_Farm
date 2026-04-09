#ifndef FARM_H
#define FARM_H
#include "enum.h"
#include "field.h"

typedef struct{//农场结构体
    field_t *fields[10][10];
    int current_size;//当前n数
    int size_level;
}farm_t;

int farm_init();
farm_t *farm_get_instance();
void size_update(farm_t* farm);
void farm_grow(farm_t* farm);
int get_farm_size(farm_t* farm);

#endif