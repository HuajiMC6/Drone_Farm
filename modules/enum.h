#ifndef ENUM_H
#define ENUM_H

typedef enum { // 作物种类
    CROP_TYPE_WHEAT,
    CROP_TYPE_RICE,
    CROP_TYPE_CORN,
    CROP_TYPE_NONE // 没种
} crop_type_t;

typedef enum { // 虫害种类
    CROP_DAMAGE_APHID,
    CROP_DAMAGE_MITE,
    CROP_DAMAGE_LEAFROLLER,
    CROP_DAMAGE_LOCUST,
    CROP_DAMAGE_NONE // 便于遍历
} crop_damage_t;

typedef enum {                      // 农药种类
    CROP_PESTICIDE_APHICIDE,        // 杀蚜剂（对应蚜虫）
    CROP_PESTICIDE_ACARICIDE,       // 杀螨剂（对应螨虫）
    CROP_PESTICIDE_LEAFROLLERICIDE, // 杀卷叶蛾剂（对应卷叶蛾）
    CROP_PESTICIDE_LOCUSTICIDE,     // 杀蝗剂（对应蝗虫）
    CROP_PESTICIDE_NONE             // 无农药（对应无虫害）
} crop_pesticide_t;

typedef enum {        // 生长阶段
    CROP_STAGE_SEED,  // 0-10
    CROP_STAGE_YOUNG, // 10-30
    CROP_STAGE_GROW,  // 30-70
    CROP_STAGE_BLOOM, // 70-90
    CROP_STAGE_RIPE,  // 90-100
    CROP_STAGE_READY, // 100
    CROP_STAGE_NONE
} crop_stage_t;

typedef enum { //
    DRONE_STATE_FREE,
    DRONE_STATE_DETECTING,
    DRONE_STATE_AUTO,
    DRONE_STATE_NONE
} drone_state_t;

const char *crop_type_name(crop_type_t type);
const char *crop_pest_name(crop_damage_t pest);
const char *crop_pesticide_name(crop_pesticide_t pesticide);
const char *crop_stage_name(crop_stage_t stage);
const char *drone_state_name(drone_state_t state);

#endif