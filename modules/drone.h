#ifndef DRONE_H
#define DRONE_H
#include "enum.h"

typedef struct{
    int x;
    int y;
}pos_t;

typedef struct{
    pos_t pos;
    int battery;
    int speed_level;
    int algorithm_level;//0全部遍历,1贪心,2
};

#endif