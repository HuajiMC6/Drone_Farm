#ifndef FIELD_H
#define FIELD_H
#include <stdbool.h>
#include "enum.h"

typedef struct{//田地
    int output_level;//产量等级
    int ready_time_level;//产速等级
    int tolerance_level;//耐虫性等级

    crop_type_t crop_type;//作物种类
    int ready_time;//总需时间
    int growing_time;//生长时间
    double growing_percent;//生长进度
    crop_stage_t stage;//生长阶段

    crop_damage_t damage;//虫害种类
    bool is_damaged;//是否染虫害

    int base_output;//基础作物产量
    double factor;//影响因子
    double extra_factor;//额外影响因子
    
    double tolerance;//抗虫害
}field_t;

// 田地管理
field_t* field_init();
void field_plant(field_t* field, crop_type_t type);
void field_grow(field_t* field);
int field_harvest(field_t* field);
void use_pesticide(field_t* field);
crop_damage_t get_damage(field_t* field);

// 升级接口(升级下一季生效)
void output_update(field_t* field);
void ready_time_update(field_t* field);
void tolerance_update(field_t* field);
#endif