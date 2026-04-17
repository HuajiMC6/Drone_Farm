#ifndef PLAYER_H
#define PLAYER_H
#include "drone.h"
#include "enum.h"
#include "farm.h"
#include "store.h"

typedef struct {
    int experience;
    int level;
    int level_stage;
    int coins;
    crop_type_t seed_bag[CROP_TYPE_NONE];
    crop_type_t harvest_bag[CROP_TYPE_NONE];
} player_t;

extern int harvest_exp_earn[CROP_TYPE_NONE];
extern int plant_exp_earn;
extern int use_pesticide_exp_earn;
extern int experience_level[40];

player_t *player_get_instance();
void player_init();
bool player_buy_seed(crop_type_t seed_type, int n);
bool player_buy_pesticide(crop_pesticide_t pesticide_type, int n);
bool player_plant(field_t *field, crop_type_t crop_type);
bool player_harvest(field_t *field);
bool player_sold(crop_type_t crop_type, int n);
void player_use_pesticide_exp();

// drone
bool player_buy_drone_speed_update();
bool player_buy_drone_algorithm_update();
bool player_buy_drone_storage_update();

// farm
bool player_buy_farm_size_update();

// field
bool player_buy_field_output_update(pos_t pos);
bool player_buy_field_ready_time_update(pos_t pos);
bool player_buy_field_tolerance_update(pos_t pos);

#endif