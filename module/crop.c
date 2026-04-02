#include "crop.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

//初始化
field_t* field_init(){
    field_t* field=calloc(1, sizeof(field_t));  // 所有字节置0
    //不该置0的
    field->base_output=100;
    field->crop_type=CROP_TYPE_NONE;//无作物
    field->damage=CROP_DAMAGE_NONE;  // 无虫害
    field->factor=1.0;      // 默认因子为1
    return field;
}

//打表
static int ready_time(crop_type_t type){
    switch (type) {
        case CROP_TYPE_WHEAT:
            return 100;   // 小麦 100 天成熟
        case CROP_TYPE_RICE:
            return 120;   // 水稻 120 天
        case CROP_TYPE_CORN:
            return 90;    // 玉米 90 天
        default:
            return 0;     // NONE类型，返回0
    }
}

//二次函数
static double calculate_possibility(double a,double b,double c,double t,double decline){
    double percent=a*(t-b)*(t-b)+c-decline;
    if(percent>1) return 1;
    else if(percent<0) return 0;
    else return percent;
}

//计算概率
static double get_damage_possibility(crop_damage_t damage,double growing_percent,double tolerance) {
    switch (damage) {
        case CROP_DAMAGE_APHID:
            // 蚜虫：早期高发，峰值20%，最大概率0.95
            return calculate_possibility(-6,0.2,0.95,growing_percent,tolerance);
        case CROP_DAMAGE_MITE:
            // 螨虫：中期高发，峰值50%，最大概率0.98
            return calculate_possibility(-5,0.5,0.98,growing_percent,tolerance);
        case CROP_DAMAGE_LEAFROLLER:
            // 卷叶虫：中晚期高发，峰值70%，最大概率0.96
            return calculate_possibility(-4,0.7,0.96,growing_percent,tolerance);
        case CROP_DAMAGE_LOCUST:
            // 蝗虫：晚期高发，峰值85%，最大概率0.90
            return calculate_possibility(-3,0.85,0.90,growing_percent,tolerance);
        default:
            return 0.0;
    }
}

//找最大可能病
static crop_damage_t max_possibility(field_t* crop){
    double max=0;
    crop_damage_t result=CROP_DAMAGE_APHID;
    for(crop_damage_t i=CROP_DAMAGE_APHID;i<CROP_DAMAGE_NONE;i++){
        double current=get_damage_possibility(i,crop->growing_percent,crop->tolerance);
        if(max<current){
            max=current;
            result=i;
        }
    }
    return result;
}

//种植
void crop_plant(field_t *crop,crop_type_t type){
    if(type==CROP_TYPE_NONE) { // 清空田地
        crop->crop_type=type;
        crop->growing_time=0;
        crop->growing_percent=0;
        crop->stage=SEED;
        crop->damage=CROP_DAMAGE_NONE;
        crop->is_damaged=false;
        crop->base_output=0;
        crop->factor=1;
        crop->extra_factor=0.0;
        crop->tolerance=0.0;
        return;
    }
    crop->crop_type=type;
    crop->ready_time=ready_time(crop->crop_type);
    crop->growing_time=0;
    crop->growing_percent=0;
    crop->stage=SEED;
    crop->damage=max_possibility(crop);
    crop->is_damaged=false;
    crop->base_output=100;
    crop->factor=1;
    crop->extra_factor=0;
    crop->tolerance=0;
    load_output_update(crop);
    load_ready_time_update(crop);
    load_tolerance_update(crop);
}

//阶段判断与更新
static void stage_update(field_t* crop){//也就1刻的事儿，不管1e-6了
    double percent=crop->growing_percent;
    if(percent<0.1) crop->stage=SEED;
    else if(percent<0.3) crop->stage=YOUNG;
    else if(percent<0.7) crop->stage=GROW;
    else if(percent<0.9) crop->stage=BLOOM;
    else if(percent<1.0) crop->stage=RIPE;
    else crop->stage=READY;
}

//生长
void crop_grow(field_t* crop){
    if(crop->crop_type==CROP_TYPE_NONE||crop->stage==READY) return;
    if(crop->factor<0.5){
        crop_plant(crop,CROP_TYPE_NONE);//植物死亡
        return;
    }

    //生长时间++
    crop->growing_time++;
    crop->growing_percent=(double)crop->growing_time/crop->ready_time;

    //产量因子变化
    if(crop->is_damaged) crop->factor-=0.01;
    else if(crop->factor+0.005>1) crop->factor=1;
    else crop->factor+=0.005;

    //生长阶段变化
    crop_stage_t pre=crop->stage;
    stage_update(crop);
    if(!crop->is_damaged&&pre!=crop->stage){//没患病且生长阶段更新，刷新患病概率
        crop->damage=max_possibility(crop);
    }

    //随机数结算患病情况
    //srand(time(NULL));这句话一定要在main.c里面用
    if(!crop->is_damaged){
        int random=rand()%100;
        if(100*get_damage_possibility(crop->damage,crop->growing_percent,crop->tolerance)>random) crop->is_damaged=true;
    }
}

//收获
int crop_harvest(field_t* crop){
    if(crop->crop_type==CROP_TYPE_NONE||crop->stage!=READY) return 0;
    int output=crop->base_output*(crop->factor+crop->extra_factor);
    crop_plant(crop,CROP_TYPE_NONE);//移除植物
    return output;
}

//喷农药
void use_pesticide(field_t* crop){
    if(crop->crop_type==CROP_TYPE_NONE||crop->stage==READY) return;
    crop->is_damaged=false;
    crop->damage=max_possibility(crop);//打农药，刷新患病概率
}

//产量等级数据获取
static void load_output_update(field_t* field){
    int level=field->output_level;
    if(level==0) field->extra_factor=0;
    else if(level==1) field->extra_factor=0.2;
    else if(level==2) field->extra_factor=0.4;
    else if(level==3) field->extra_factor=0.5;
}

//产速等级数据获取
static void load_ready_time_update(field_t* field){
    int level=field->ready_time_level;
    if(level==0) field->ready_time=ready_time(field->crop_type);
    else if(level==1) field->ready_time=0.9*ready_time(field->crop_type);
    else if(level==2) field->ready_time=0.8*ready_time(field->crop_type);
    else if(level==3) field->ready_time=0.7*ready_time(field->crop_type);
}

//耐虫性等级数据获取
static void load_tolerance_update(field_t* field){
    int level=field->tolerance_level;
    if(level==0) field->tolerance=0;
    else if(level==1) field->tolerance=0.05;
    else if(level==2) field->tolerance=0.1;
    else if(level==3) field->tolerance=0.15;
}

//产量升级
void output_update(field_t* field){
    if(field->output_level>=3) return;
    field->output_level++;
}

//产速升级
void ready_time_update(field_t* field){
    if(field->ready_time_level>=3) return;
    field->ready_time_level++;
}

//耐虫性升级
void tolerance_update(field_t* field){
    if(field->tolerance_level>=3) return;
    field->tolerance_level++;
}

//监测是否患病及其类型
crop_damage_t get_damage(field_t* crop){
    return crop->damage;
}