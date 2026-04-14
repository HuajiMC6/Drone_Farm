#ifndef STORE_H
#define STORE_H
#include "enum.h"

// 等级折扣
extern double level_discount[7];

extern int seed_price[CROP_TYPE_NONE];
extern int pesticide_price[CROP_PESTICIDE_NONE];
extern int harvest_price[CROP_TYPE_NONE];

// drone update
extern int drone_speed_update_price[3];
extern int drone_storage_update_price[3];
extern int drone_algorithm_update_price[2];
// farm update
extern int farm_size_update_price[3];
// field update
extern int field_output_upgrade_price[3];
extern int field_ready_time_upgrade_price[3];
extern int field_tolerance_upgrade_price[3];

#endif