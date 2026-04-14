#include "enum.h"
#include "store.h"

// 等级折扣
double level_discount[7] = {
    1,    // 0-9
    0.95, // 10-14
    0.9,  // 15-19
    0.85, // 20-24
    0.8,  // 25-29
    0.75, // 30-39
    0.7   // 40+
};

int seed_price[CROP_TYPE_NONE] = {
    10,
    15,
    20};
int pesticide_price[CROP_PESTICIDE_NONE] = {
    5,
    15,
    20,
    15};

int harvest_price[CROP_TYPE_NONE] = {
    40,
    60,
    80};

// drone update
int drone_speed_update_price[3] = {1000, 2500, 5000};
int drone_storage_update_price[3] = {1000, 2500, 5000};
int drone_algorithm_update_price[2] = {10000, 50000};
// farm update
int farm_size_update_price[3] = {5000, 10000, 20000};
// field update
int field_output_upgrade_price[3] = {2500, 5000, 10000};
int field_ready_time_upgrade_price[3] = {2500, 5000, 10000};
int field_tolerance_upgrade_price[3] = {10000, 20000, 50000};