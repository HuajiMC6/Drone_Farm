#include "data.h"

/* —————————————— 配置数据 —————————————— */

/* 作物成熟时间表（按 crop_type_t 顺序），单位：游戏时间单位 */
const int crop_ready_time[CROP_TYPE_NONE] = {250, 300, 225};

/* 成长阶段阈值（0.0 - 1.0），用于判断阶段边界 */
const double crop_stage_thresholds[CROP_STAGE_THRESHOLD_COUNT] = {0.1, 0.3, 0.7, 0.9, 1.0};

/* 虫害影响参数表：每个虫害对应一组 a,b,c 参数，用于计算作物受损程度 */
const crop_damage_profile_t crop_damage_profiles[CROP_DAMAGE_NONE] = {
    {-6, 0.2, 0.95},
    {-5, 0.5, 0.98},
    {-4, 0.7, 0.96},
    {-3, 0.85, 0.90},
};

/* 地块随等级增加的产量/时间/容忍度相关的提升表 */
const double field_output_upgrade_bonus[FIELD_LEVEL_COUNT] = {0.0, 0.2, 0.4, 0.5};
const double field_ready_time_upgrade_ratio[FIELD_LEVEL_COUNT] = {1.0, 0.9, 0.8, 0.7};
const double field_tolerance_upgrade_value[FIELD_LEVEL_COUNT] = {0.0, 0.1, 0.25, 0.4};

/* 等级区间对应的收益折扣系数（按等级段索引） */
const double level_discount[7] = {
    1,    // 0-9
    0.95, // 10-14
    0.9,  // 15-19
    0.85, // 20-24
    0.8,  // 25-29
    0.75, // 30-39
    0.7   // 40+
};

/* 经验值配置：收获、种植、使用农药获得的经验；以及升级经验表 */
const int harvest_exp_earn[CROP_TYPE_NONE] = {2, 3, 4};
const int plant_exp_earn = 2;         /* 每次种植奖励经验 */
const int use_pesticide_exp_earn = 5; /* 使用农药奖励经验 */
const int experience_level[PLAYER_EXPERIENCE_LEVELS] = {
    10,  25,  40,  55,  75,  95,  120, 150, 180,  210,  240,  270,  300,  330,  360,  390,  420,  450,  480,  550,
    600, 650, 700, 750, 800, 850, 900, 950, 1000, 1060, 1130, 1210, 1300, 1400, 1510, 1630, 1760, 1900, 2050, 2210};

/* 玩家等级段阈值 */
const int player_level_stage_thresholds[PLAYER_LEVEL_STAGE_THRESHOLD_COUNT] = {10, 15, 20, 25, 30, 40};

/* 玩家/无人机/农场基础数值 */
const int player_starting_coins = 500;                                         /* 玩家初始金钱 */
const int drone_speed_values[DRONE_LEVEL_COUNT] = {10, 20, 30, 50};            /* 无人机不同等级的速度 */
const int drone_storage_capacity_values[DRONE_LEVEL_COUNT] = {10, 20, 30, 50}; /* 无人机存储容量 */
const int farm_size_by_level[FARM_LEVEL_COUNT] = {4, 5, 6, 7};                 /* 农场不同等级的地块数 */

/* 地块基础产量与损伤模型参数 */
const int field_base_output = 100;                   /* 基础产量 */
const double field_factor_threshold = 0.3;           /* 产量系数阈值 */
const double field_damage_decline_rate = 0.005;      /* 损伤随时间自然下降速率 */
const double field_damage_recovery_rate = 0.005;     /* 损伤恢复速率 */
const int field_damage_probability_scale = 100;      /* 概率缩放基数 */
const int field_damage_probability_multiplier = 150; /* 概率乘数 */
const double field_default_factor = 1.0;             /* 默认产量系数 */
const double field_default_tolerance = 0.0;          /* 默认容忍度 */

/* —————————————— 价格配置 —————————————— */

/* 价格表：种子、农药、收获收益（按枚举顺序） */
const int seed_price[CROP_TYPE_NONE] = {10, 15, 20};
const int pesticide_price[CROP_PESTICIDE_NONE] = {5, 15, 20, 15};
const int harvest_price[CROP_TYPE_NONE] = {40, 60, 80};

/* 升级价格：无人机速度/存储/算法、农场大小、地块各项升级 */
const int drone_speed_update_price[3] = {1000, 2500, 5000};
const int drone_storage_update_price[3] = {1000, 2500, 5000};
const int drone_algorithm_update_price[2] = {10000, 50000};
const int farm_size_update_price[3] = {5000, 10000, 20000};
const int field_output_upgrade_price[3] = {2500, 5000, 10000};
const int field_ready_time_upgrade_price[3] = {2500, 5000, 10000};
const int field_tolerance_upgrade_price[3] = {10000, 20000, 50000};

/* —————————————— 名称映射 —————————————— */

/* 作物类型名称 */
static const char *const crop_type_names[CROP_TYPE_NONE] = {
    [CROP_TYPE_WHEAT] = "Wheat",
    [CROP_TYPE_RICE] = "Rice",
    [CROP_TYPE_CORN] = "Corn",
};

/* 虫害名称 */
static const char *const crop_pest_names[CROP_DAMAGE_NONE] = {
    [CROP_DAMAGE_APHID] = "Aphid",
    [CROP_DAMAGE_MITE] = "Mite",
    [CROP_DAMAGE_LEAFROLLER] = "Leafroller",
    [CROP_DAMAGE_LOCUST] = "Locust",
};

/* 农药名称 */
static const char *const crop_pesticide_names[CROP_PESTICIDE_NONE] = {
    [CROP_PESTICIDE_APHICIDE] = "Aphicide",
    [CROP_PESTICIDE_ACARICIDE] = "Acaricide",
    [CROP_PESTICIDE_LEAFROLLERICIDE] = "Leafrollericide",
    [CROP_PESTICIDE_LOCUSTICIDE] = "Locusticide",
};

/* 作物生长阶段名称 */
static const char *const crop_stage_names[CROP_STAGE_NONE] = {
    [CROP_STAGE_SEED] = "Seed",   [CROP_STAGE_YOUNG] = "Young", [CROP_STAGE_GROW] = "Grow",
    [CROP_STAGE_BLOOM] = "Bloom", [CROP_STAGE_RIPE] = "Ripe",   [CROP_STAGE_READY] = "Ready",
};

/* 无人机状态名称 */
static const char *const drone_state_names[DRONE_STATE_NONE] = {
    [DRONE_STATE_FREE] = "Free",
    [DRONE_STATE_DETECTING] = "Detecting",
    [DRONE_STATE_AUTO] = "Spraying",
};

/* —————————————— 名称获取函数 —————————————— */

const char *crop_type_name(crop_type_t type) {
    if (type >= CROP_TYPE_NONE)
        return "Unknown";
    return crop_type_names[type];
}

const char *crop_pest_name(crop_damage_t pest) {
    if (pest >= CROP_DAMAGE_NONE)
        return "Unknown";
    return crop_pest_names[pest];
}

const char *crop_pesticide_name(crop_pesticide_t pesticide) {
    if (pesticide >= CROP_PESTICIDE_NONE)
        return "Unknown";
    return crop_pesticide_names[pesticide];
}

const char *crop_stage_name(crop_stage_t stage) {
    if (stage >= CROP_STAGE_NONE)
        return "Unknown";
    return crop_stage_names[stage];
}

const char *drone_state_name(drone_state_t state) {
    if (state >= DRONE_STATE_NONE)
        return "Unknown";
    return drone_state_names[state];
}
