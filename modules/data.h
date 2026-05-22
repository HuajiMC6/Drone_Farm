#ifndef DATA_H
#define DATA_H

/*
 * 模块说明：
 * 该模块集中保存游戏配置数据与枚举定义（例如作物种类、成长阶段、价格表、经验表等），
 * 所有值均为只读常量；在代码中通过索引与枚举值对应。
 */

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

typedef enum { // 无人机状态
    DRONE_STATE_FREE,
    DRONE_STATE_DETECTING,
    DRONE_STATE_AUTO,
    DRONE_STATE_NONE
} drone_state_t;

typedef struct {
    double a;
    double b;
    double c;
} crop_damage_profile_t;

enum {
    /* 全局常量：游戏网格、等级表长度与各类上限/计数 */
    GAME_GRID_SIZE = 7,                            /* 游戏格子行/列数（网格大小） */
    GAME_CELL_SIZE = 100,                          /* 单元格像素尺寸（UI 渲染） */
    PLAYER_EXPERIENCE_LEVELS = 40,                 /* 玩家经验/等级表长度 */
    PLAYER_LEVEL_STAGE_THRESHOLD_COUNT = 6,        /* 玩家等级阶段阈值数量 */
    FIELD_LEVEL_COUNT = 4,                         /* 地块可用等级数量 */
    DRONE_LEVEL_COUNT = 4,                         /* 无人机等级数量 */
    FARM_LEVEL_COUNT = 4,                          /* 农场等级数量 */
    CROP_STAGE_THRESHOLD_COUNT = CROP_STAGE_READY, /* 作物阶段阈值数量（等于 CROP_STAGE_READY） */
    DRONE_SPEED_LEVEL_MAX = 3,                     /* 无人机速度可达的等级上限（索引计数） */
    DRONE_STORAGE_LEVEL_MAX = 3,                   /* 无人机存储可达的等级上限 */
    DRONE_ALGORITHM_LEVEL_MAX = 2,                 /* 无人机算法升级上限 */
    FARM_SIZE_LEVEL_MAX = 3,                       /* 农场可升级大小级别上限 */
    FIELD_UPGRADE_LEVEL_MAX = 3,                   /* 地块可升级等级上限 */
};

/* ===== 作物相关配置 ===== */
/* 作物成熟所需时间表，按 `crop_type_t` 顺序，对应每种作物的成熟时间（单位：游戏时间单位） */
extern const int crop_ready_time[CROP_TYPE_NONE];
/* 作物各成长阶段的阈值（0.0 - 1.0），用于计算阶段进度 */
extern const double crop_stage_thresholds[CROP_STAGE_THRESHOLD_COUNT];
/* 作物受虫害影响时的参数曲线（结构体表示 a,b,c 三个参数） */
extern const crop_damage_profile_t crop_damage_profiles[CROP_DAMAGE_NONE];

/* ===== 地块/农场/无人机升级与价格相关 ===== */
/* 地块产量随等级增加的额外系数 */
extern const double field_output_upgrade_bonus[FIELD_LEVEL_COUNT];
/* 地块收获所需时间的倍率（等级对应的乘数） */
extern const double field_ready_time_upgrade_ratio[FIELD_LEVEL_COUNT];
/* 地块容忍度随等级提升的增加值 */
extern const double field_tolerance_upgrade_value[FIELD_LEVEL_COUNT];

/* 不同等级范围的收益折扣（索引为等级段） */
extern const double level_discount[7];

/* 种子/农药/收获等价格表，按枚举顺序排列 */
extern const int seed_price[CROP_TYPE_NONE];
extern const int pesticide_price[CROP_PESTICIDE_NONE];
extern const int harvest_price[CROP_TYPE_NONE];

/* 无人机、仓储、算法、农场与地块的升级价格表 */
extern const int drone_speed_update_price[3];
extern const int drone_storage_update_price[3];
extern const int drone_algorithm_update_price[2];
extern const int farm_size_update_price[3];
extern const int field_output_upgrade_price[3];
extern const int field_ready_time_upgrade_price[3];
extern const int field_tolerance_upgrade_price[3];

/* ===== 经验与等级相关 ===== */
/* 不同作物收获/种植/使用农药的经验值 */
extern const int harvest_exp_earn[CROP_TYPE_NONE];
extern const int plant_exp_earn;         /* 种植获得经验 */
extern const int use_pesticide_exp_earn; /* 使用农药获得经验 */
/* 升级所需的累计经验表（玩家等级上限 PLAYER_EXPERIENCE_LEVELS） */
extern const int experience_level[PLAYER_EXPERIENCE_LEVELS];
/* 玩家等级段对应的阶段阈值（用于界面或功能解锁） */
extern const int player_level_stage_thresholds[PLAYER_LEVEL_STAGE_THRESHOLD_COUNT];

/* ===== 玩家/无人机/农场/地块基础常量 ===== */
extern const int player_starting_coins;                            /* 玩家初始金钱 */
extern const int drone_speed_values[DRONE_LEVEL_COUNT];            /* 无人机速度值表 */
extern const int drone_storage_capacity_values[DRONE_LEVEL_COUNT]; /* 无人机仓储容量表 */
extern const int farm_size_by_level[FARM_LEVEL_COUNT];             /* 农场大小随等级变化 */

/* 地块基础与损伤模型参数 */
extern const int field_base_output;                   /* 地块基础产量 */
extern const double field_factor_threshold;           /* 产量系数阈值 */
extern const double field_damage_decline_rate;        /* 损伤下降速率（随时间减少的比率） */
extern const double field_damage_recovery_rate;       /* 损伤恢复速率 */
extern const int field_damage_probability_scale;      /* 概率缩放基数 */
extern const int field_damage_probability_multiplier; /* 概率乘数 */
extern const double field_default_factor;             /* 地块默认产量系数 */
extern const double field_default_tolerance;          /* 地块默认容忍度 */

const char *crop_type_name(crop_type_t type);
const char *crop_pest_name(crop_damage_t pest);
const char *crop_pesticide_name(crop_pesticide_t pesticide);
const char *crop_stage_name(crop_stage_t stage);
const char *drone_state_name(drone_state_t state);

#endif
