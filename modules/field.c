#include "field.h"
#include "enum.h"
#include "event.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static int ready_time(crop_type_t type);
static double calculate_possibility(double a, double b, double c, double t, double decline);
static double get_damage_possibility(crop_damage_t damage, double growing_percent, double tolerance);
static crop_damage_t max_possibility(field_t *field);
static void stage_update(field_t *field);
static void load_output_update(field_t *field);
static void load_ready_time_update(field_t *field);
static void load_tolerance_update(field_t *field);

// 初始化
field_t *field_init(int x, int y)
{
    field_t *field = calloc(1, sizeof(field_t)); // 所有字节置0
    field->x = x;
    field->y = y;
    // 不该置0的
    field->crop_type = CROP_TYPE_NONE; // 无作物
    field->damage = CROP_DAMAGE_NONE;  // 无虫害
    field->factor = 1.0;               // 默认因子为1
    return field;
}

// 打表
static int ready_time(crop_type_t type)
{
    switch (type)
    {
    case CROP_TYPE_WHEAT:
        return 100; // 小麦 100 天成熟
    case CROP_TYPE_RICE:
        return 120; // 水稻 120 天
    case CROP_TYPE_CORN:
        return 90; // 玉米 90 天
    default:
        return 0; // NONE类型，返回0
    }
}

// 二次函数
static double calculate_possibility(double a, double b, double c, double t, double decline)
{
    double percent = a * (t - b) * (t - b) + c - decline;
    if (percent > 1)
        return 1;
    else if (percent < 0)
        return 0;
    else
        return percent;
}

// 计算概率
static double get_damage_possibility(crop_damage_t damage, double growing_percent, double tolerance)
{
    switch (damage)
    {
    case CROP_DAMAGE_APHID:
        // 蚜虫：早期高发，峰值20%，最大概率0.95
        return calculate_possibility(-6, 0.2, 0.95, growing_percent, tolerance);
    case CROP_DAMAGE_MITE:
        // 螨虫：中期高发，峰值50%，最大概率0.98
        return calculate_possibility(-5, 0.5, 0.98, growing_percent, tolerance);
    case CROP_DAMAGE_LEAFROLLER:
        // 卷叶虫：中晚期高发，峰值70%，最大概率0.96
        return calculate_possibility(-4, 0.7, 0.96, growing_percent, tolerance);
    case CROP_DAMAGE_LOCUST:
        // 蝗虫：晚期高发，峰值85%，最大概率0.90
        return calculate_possibility(-3, 0.85, 0.90, growing_percent, tolerance);
    default:
        return 0.0;
    }
}

// 找最大可能病
static crop_damage_t max_possibility(field_t *field)
{
    double max = 0;
    crop_damage_t result = CROP_DAMAGE_APHID;
    for (crop_damage_t i = CROP_DAMAGE_APHID; i < CROP_DAMAGE_NONE; i++)
    {
        double current = get_damage_possibility(i, field->growing_percent, field->tolerance);
        if (max < current)
        {
            max = current;
            result = i;
        }
    }
    return result;
}

// 种植
bool field_plant(field_t *field, crop_type_t type)
{
    if (type == CROP_TYPE_NONE)
    { // 清空田地//只有重置会用到
        field->crop_type = type;
        field->growing_time = 0;
        field->growing_percent = 0;
        field->stage = CROP_STAGE_NONE;
        field->damage = CROP_DAMAGE_NONE;
        field->is_damaged = false;
        field->is_detected = false;
        field->base_output = 0;
        field->factor = 1;
        field->extra_factor = 0.0;
        field->tolerance = 0.0;

        event_send(EVENT_FIELD_UPDATE, field);

        return true;
    }
    else
    {
        if (field->crop_type != CROP_TYPE_NONE)
            return false;
        field->crop_type = type;
        field->ready_time = ready_time(field->crop_type);
        field->growing_time = 0;
        field->growing_percent = 0;
        field->stage = CROP_STAGE_SEED;
        field->damage = max_possibility(field);
        field->is_damaged = false;
        field->is_detected = false;
        field->base_output = 100;
        field->factor = 1;
        field->extra_factor = 0;
        field->tolerance = 0;
        load_output_update(field);
        load_ready_time_update(field);
        load_tolerance_update(field);

        event_send(EVENT_FIELD_UPDATE, field);

        return true;
    }
}

// 阶段判断与更新
static void stage_update(field_t *field)
{ // 也就1刻的事儿，不管1e-6了
    double percent = field->growing_percent;
    if (percent < 0.1)
        field->stage = CROP_STAGE_SEED;
    else if (percent < 0.3)
        field->stage = CROP_STAGE_YOUNG;
    else if (percent < 0.7)
        field->stage = CROP_STAGE_GROW;
    else if (percent < 0.9)
        field->stage = CROP_STAGE_BLOOM;
    else if (percent < 1.0)
        field->stage = CROP_STAGE_RIPE;
    else
        field->stage = CROP_STAGE_READY;
}

// 生长
void field_grow(field_t *field)
{
    if (field->crop_type == CROP_TYPE_NONE || field->stage == CROP_STAGE_READY)
    {
        field->damage = CROP_DAMAGE_NONE;
        field->is_detected = false;
        return;
    }
    if (field->factor < 0.5)
    {
        field_plant(field, CROP_TYPE_NONE); // 植物死亡
        return;
    }

    if (field->damage == CROP_DAMAGE_NONE)
        field->is_damaged = false;
    else
        field->is_damaged = true;

    // 生长时间++
    field->growing_time++;
    field->growing_percent = (double)field->growing_time / field->ready_time;

    // 产量因子变化
    if (field->is_damaged)
        field->factor -= 0.01;
    else if (field->factor + 0.005 > 1)
        field->factor = 1;
    else
        field->factor += 0.005;

    // 生长阶段变化
    crop_stage_t pre = field->stage;
    stage_update(field);
    if (!field->is_damaged && pre != field->stage)
    { // 没患病且生长阶段更新，刷新患病概率
        field->damage = max_possibility(field);
    }

    // 随机数结算患病情况
    // srand(time(NULL));这句话一定要在main.c里面用
    if (!field->is_damaged)
    {
        int random = rand() % 100;
        if (100 * get_damage_possibility(field->damage, field->growing_percent, field->tolerance) > random)
        {
            field->is_damaged = true;
            field->is_detected = false;
        }
    }
}

// 收获
int field_harvest(field_t *field)
{
    if (field->crop_type == CROP_TYPE_NONE || field->stage != CROP_STAGE_READY)
        return 0;
    int output = field->base_output * (field->factor + field->extra_factor);
    field_plant(field, CROP_TYPE_NONE); // 移除植物
    return output;
}

// 喷农药
void field_use_pesticide(field_t *field)
{
    if (field->crop_type == CROP_TYPE_NONE || field->stage == CROP_STAGE_READY)
        return;
    field->is_damaged = false;
    field->is_detected = false;
    field->damage = max_possibility(field); // 打农药，刷新患病概率

    event_send(EVENT_FIELD_UPDATE, field);
}

// 产量等级数据获取
static void load_output_update(field_t *field)
{
    int level = field->output_level;
    if (level == 0)
        field->extra_factor = 0;
    else if (level == 1)
        field->extra_factor = 0.2;
    else if (level == 2)
        field->extra_factor = 0.4;
    else if (level == 3)
        field->extra_factor = 0.5;
}

// 产速等级数据获取
static void load_ready_time_update(field_t *field)
{
    int level = field->ready_time_level;
    if (level == 0)
        field->ready_time = ready_time(field->crop_type);
    else if (level == 1)
        field->ready_time = 0.9 * ready_time(field->crop_type);
    else if (level == 2)
        field->ready_time = 0.8 * ready_time(field->crop_type);
    else if (level == 3)
        field->ready_time = 0.7 * ready_time(field->crop_type);
}

// 耐虫性等级数据获取
static void load_tolerance_update(field_t *field)
{
    int level = field->tolerance_level;
    if (level == 0)
        field->tolerance = 0;
    else if (level == 1)
        field->tolerance = 0.05;
    else if (level == 2)
        field->tolerance = 0.1;
    else if (level == 3)
        field->tolerance = 0.15;
}

// 产量升级
bool field_output_update(field_t *field)
{
    if (field->output_level >= 3)
        return false;
    field->output_level++;
    event_send(EVENT_FIELD_UPDATE, field);
    return true;
}

// 产速升级
bool field_ready_time_update(field_t *field)
{
    if (field->ready_time_level >= 3)
        return false;
    field->ready_time_level++;
    event_send(EVENT_FIELD_UPDATE, field);
    return true;
}

// 耐虫性升级
bool field_tolerance_update(field_t *field)
{
    if (field->tolerance_level >= 3)
        return false;
    field->tolerance_level++;
    event_send(EVENT_FIELD_UPDATE, field);
    return true;
}

crop_damage_t field_get_damage(field_t *field)
{
    field->is_detected = true;
    return field->damage;
}