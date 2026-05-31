#include "field.h"
#include "data.h"
#include "event.h"
#include <stdbool.h>
#include <stdlib.h>

static int ready_time(crop_type_t type);
static double calculate_possibility(double a, double b, double c, double t, double tolerance);
static double get_damage_possibility(crop_damage_t damage, double growing_percent, double tolerance);
static crop_damage_t max_possibility(field_t *field);
static void stage_upgrade(field_t *field);
static void load_output_upgrade(field_t *field);
static void load_ready_time_upgrade(field_t *field);
static void load_tolerance_upgrade(field_t *field);

// 初始化
// 创建并初始化一个地块对象（堆分配，零初始化）
field_t *field_init(int x, int y) {
    field_t *field = calloc(1, sizeof(field_t)); // 所有字节置0
    field->x = x;
    field->y = y;
    // 不该置0的
    field->crop_type = CROP_TYPE_NONE;    // 无作物
    field->damage = CROP_DAMAGE_NONE;     // 无虫害
    field->factor = field_default_factor; // 默认因子为1
    return field;
}

// 清空
// 清空地块：移除作物，重置所有状态字段
bool field_remove(field_t *field) {
    field->crop_type = CROP_TYPE_NONE;
    field->growing_time = 0;
    field->growing_percent = 0;
    field->stage = CROP_STAGE_NONE;
    field->damage = CROP_DAMAGE_NONE;
    field->is_detected = false;
    field->base_output = 0;
    field->factor = field_default_factor;
    field->tolerance = field_default_tolerance;

    event_send(EVENT_ON_FIELD_CLEARED, field);

    return true;
}

// 种植
// 在地块上种植指定作物
bool field_plant(field_t *field, crop_type_t type) {
    if (field->crop_type != CROP_TYPE_NONE)
        return false;
    field->crop_type = type;
    field->ready_time = ready_time(field->crop_type);
    field->growing_time = 0;
    field->growing_percent = 0;
    field->stage = CROP_STAGE_SEED;
    field->damage = CROP_DAMAGE_NONE; // 种植时无虫害
    field->is_detected = false;
    field->base_output = field_base_output;
    field->factor = field_default_factor;
    field->tolerance = field_default_tolerance;
    load_output_upgrade(field);
    load_ready_time_upgrade(field);
    load_tolerance_upgrade(field);

    event_send(EVENT_ON_FIELD_PLANTED, field);

    return true;
}

// 生长
// 驱动地块作物生长一个单位时间
void field_grow(field_t *field) {
    if (field->crop_type == CROP_TYPE_NONE || field->stage == CROP_STAGE_READY) {
        field->damage = CROP_DAMAGE_NONE;
        return;
    }

    if (field->factor < field_factor_threshold) {
        field_remove(field); // 植物死亡
        return;
    }

    // 生长时间++
    field->growing_time++;
    field->growing_percent = (double)field->growing_time / field->ready_time;

    // 产量因子变化
    if (field_is_damaged(field))
        field->factor -= field_damage_decline_rate * (1.0 - field->tolerance); // 慢点死
    else
        field->factor += field_damage_recovery_rate;

    // 生长阶段变化
    crop_stage_t pre = field->stage;
    stage_upgrade(field);
    if (pre != field->stage) {
        event_send(EVENT_ON_CROP_STAGE_CHANGE, field);
    }

    // 当前无虫害时检查是否发生感染
    if (!field_is_damaged(field)) {
        // 计算当前生长阶段最可能的虫害类型
        crop_damage_t potential = max_possibility(field);
        int random = rand() % field_damage_probability_scale;
        int prob = field_damage_probability_scale *
                   get_damage_possibility(potential, field->growing_percent, field->tolerance);
        if (prob > random * field_damage_probability_multiplier) {
            field->damage = potential; // 感染该虫害
            field->is_detected = false;

            event_send(EVENT_ON_PEST_SUFFERING, field);
        }
    }
}

// 收获
// 收获成熟作物并返回产量，同时清空地块
int field_harvest(field_t *field) {
    if (field->crop_type == CROP_TYPE_NONE || field->stage != CROP_STAGE_READY)
        return 0;
    int output = field->base_output * field->factor;
    field_remove(field); // 移除植物
    event_send(EVENT_ON_FIELD_HARVESTED, field);
    return output;
}

// 喷农药
// 对地块使用农药，清除当前虫害
void field_use_pesticide(field_t *field) {
    if (field->crop_type == CROP_TYPE_NONE || field->stage == CROP_STAGE_READY)
        return;
    field->is_detected = false;
    field->damage = CROP_DAMAGE_NONE; // 清除虫害

    event_send(EVENT_ON_PEST_CLEARED, field);
}

// 产量升级
// 升级地块产量等级
bool field_output_upgrade(field_t *field) {
    if (field->output_level >= 3)
        return false;
    field->output_level++;
    load_output_upgrade(field);
    event_send(EVENT_ON_FIELD_UPGRADE, field);
    return true;
}

// 产速升级
// 升级地块成熟速度等级
bool field_ready_time_upgrade(field_t *field) {
    if (field->ready_time_level >= 3)
        return false;
    field->ready_time_level++;
    event_send(EVENT_ON_FIELD_UPGRADE, field);
    return true;
}

// 耐虫性升级
// 升级地块耐虫性等级
bool field_tolerance_upgrade(field_t *field) {
    if (field->tolerance_level >= 3)
        return false;
    field->tolerance_level++;
    event_send(EVENT_ON_FIELD_UPGRADE, field);
    return true;
}

// 获取地块当前虫害类型
crop_damage_t field_get_damage(field_t *field) {
    return field->damage;
}

// 标记地块已被无人机检测
void field_detect(field_t *field) {
    field->is_detected = true;
    event_send(EVENT_ON_PEST_DETECTED, field);
}

// 判断田地是否患病
// 判断地块是否受到虫害
bool field_is_damaged(const field_t *field) {
    // 通过虫害类型判断是否患病
    return field->damage != CROP_DAMAGE_NONE;
}

// 获取死亡进度百分比
// 获取作物死亡进度百分比（0~100）
int field_get_death_percentage(field_t *field) {
    if (field->crop_type == CROP_TYPE_NONE)
        return 0;
    if (field->factor < field_factor_threshold)
        return field_damage_probability_scale;
    // 线性映射，因子从1降到阈值时死亡率从0升到100
    return (int)((1.0 - field->factor) / (1.0 - field_factor_threshold) * field_damage_probability_scale);
}

// 阶段判断与更新
// 根据生长百分比更新作物阶段
static void stage_upgrade(field_t *field) {
    double percent = field->growing_percent;
    for (crop_stage_t stage = CROP_STAGE_SEED; stage < CROP_STAGE_READY; stage++) {
        if (percent < crop_stage_thresholds[stage]) {
            field->stage = stage;
            return;
        }
    }
    field->stage = CROP_STAGE_READY;
}

// 打表
// 查表获取作物成熟所需时间
static int ready_time(crop_type_t type) {
    if (type >= CROP_TYPE_NONE)
        return 0;
    return crop_ready_time[type];
}

// 二次函数
// 二次函数计算虫害感染概率基准值
static double calculate_possibility(double a, double b, double c, double t, double tolerance) {
    double percent = a * (t - b) * (t - b) + c - tolerance;
    if (percent > 1)
        return 1;
    else if (percent < 0)
        return 0;
    else
        return percent;
}

// 计算概率
// 计算指定虫害在当前生长阶段的感染概率
static double get_damage_possibility(crop_damage_t damage, double growing_percent, double tolerance) {
    if (damage >= CROP_DAMAGE_NONE)
        return 0.0;
    crop_damage_profile_t profile = crop_damage_profiles[damage];
    double base_percentage = calculate_possibility(profile.a, profile.b, profile.c, growing_percent, tolerance);
    return base_percentage * (1.0 - tolerance);
}

// 找最大可能病
// 返回当前最可能感染的虫害类型
static crop_damage_t max_possibility(field_t *field) {
    double max = 0;
    crop_damage_t result = 0;
    for (crop_damage_t i = 0; i < CROP_DAMAGE_NONE; i++) {
        double current = get_damage_possibility(i, field->growing_percent, field->tolerance);
        if (max < current) {
            max = current;
            result = i;
        }
    }
    return result;
}

// 产量等级数据获取
// 根据产量升级等级更新地块产量系数
static void load_output_upgrade(field_t *field) {
    int level = field->output_level;
    double new_extra = field_output_upgrade_bonus[level];
    int prev_level = (level > 0) ? (level - 1) : 0;
    double old_extra = field_output_upgrade_bonus[prev_level];

    // 将等级带来的额外加成合并到 factor（增加或减少差值）
    field->factor += (new_extra - old_extra);
}

// 产速等级数据获取
// 根据产速升级等级更新地块成熟时间
static void load_ready_time_upgrade(field_t *field) {
    int level = field->ready_time_level;
    field->ready_time = field_ready_time_upgrade_ratio[level] * ready_time(field->crop_type);
}

// 耐虫性等级数据获取
// 根据耐虫性升级等级更新地块耐受值
static void load_tolerance_upgrade(field_t *field) {
    int level = field->tolerance_level;
    field->tolerance = field_tolerance_upgrade_value[level];
}
