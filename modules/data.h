#ifndef DATA_H
#define DATA_H

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

typedef struct {
    double a;
    double b;
    double c;
} crop_damage_profile_t;

enum {
    GAME_GRID_SIZE = 10,
    GAME_CELL_SIZE = 100,
    PLAYER_EXPERIENCE_LEVELS = 40,
    PLAYER_LEVEL_STAGE_THRESHOLD_COUNT = 6,
    FIELD_LEVEL_COUNT = 4,
    DRONE_LEVEL_COUNT = 4,
    FARM_LEVEL_COUNT = 4,
    CROP_STAGE_THRESHOLD_COUNT = CROP_STAGE_READY,
    DRONE_SPEED_LEVEL_MAX = 3,
    DRONE_STORAGE_LEVEL_MAX = 3,
    DRONE_ALGORITHM_LEVEL_MAX = 2,
    FARM_SIZE_LEVEL_MAX = 3,
    FIELD_UPGRADE_LEVEL_MAX = 3,
};

extern const int crop_ready_time[CROP_TYPE_NONE];
extern const double crop_stage_thresholds[CROP_STAGE_THRESHOLD_COUNT];
extern const crop_damage_profile_t crop_damage_profiles[CROP_DAMAGE_NONE];
extern const double field_output_upgrade_bonus[FIELD_LEVEL_COUNT];
extern const double field_ready_time_upgrade_ratio[FIELD_LEVEL_COUNT];
extern const double field_tolerance_upgrade_value[FIELD_LEVEL_COUNT];
extern const double level_discount[7];
extern const int seed_price[CROP_TYPE_NONE];
extern const int pesticide_price[CROP_PESTICIDE_NONE];
extern const int harvest_price[CROP_TYPE_NONE];
extern const int drone_speed_update_price[3];
extern const int drone_storage_update_price[3];
extern const int drone_algorithm_update_price[2];
extern const int farm_size_update_price[3];
extern const int field_output_upgrade_price[3];
extern const int field_ready_time_upgrade_price[3];
extern const int field_tolerance_upgrade_price[3];
extern const int harvest_exp_earn[CROP_TYPE_NONE];
extern const int plant_exp_earn;
extern const int use_pesticide_exp_earn;
extern const int experience_level[PLAYER_EXPERIENCE_LEVELS];
extern const int player_level_stage_thresholds[PLAYER_LEVEL_STAGE_THRESHOLD_COUNT];
extern const int player_starting_coins;
extern const int drone_speed_values[DRONE_LEVEL_COUNT];
extern const int drone_storage_capacity_values[DRONE_LEVEL_COUNT];
extern const int farm_size_by_level[FARM_LEVEL_COUNT];
extern const int field_base_output;
extern const double field_factor_threshold;
extern const double field_damage_decline_rate;
extern const double field_damage_recovery_rate;
extern const int field_damage_probability_scale;
extern const int field_damage_probability_multiplier;
extern const double field_default_factor;
extern const double field_default_tolerance;

const char *crop_type_name(crop_type_t type);
const char *crop_pest_name(crop_damage_t pest);
const char *crop_pesticide_name(crop_pesticide_t pesticide);
const char *crop_stage_name(crop_stage_t stage);
const char *drone_state_name(drone_state_t state);

#endif
