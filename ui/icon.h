#ifndef __ICON_H__
#define __ICON_H__

#include "data.h"
#include "lvgl.h"

const void *icon_get_crop(crop_type_t type, crop_stage_t stage);
const void *icon_get_crop_item(crop_type_t type);
const void *icon_get_pest(crop_damage_t pest);
const void *icon_get_pesticide(crop_pesticide_t pesticide);

lv_img_dsc_t *icon_sd_load(const char *path, lv_img_cf_t cf, uint16_t w, uint16_t h);
void icon_sd_free(lv_img_dsc_t *dsc);

LV_IMG_DECLARE(icon_field_bg);
LV_IMG_DECLARE(icon_farm_bg);
LV_IMG_DECLARE(icon_gold_bar_bg);
LV_IMG_DECLARE(icon_exp_bar_bg);

LV_IMG_DECLARE(icon_plant_btn);
LV_IMG_DECLARE(icon_shop_btn);
LV_IMG_DECLARE(icon_storage_btn);
LV_IMG_DECLARE(icon_setting_btn);

LV_IMG_DECLARE(icon_crop_corn);
LV_IMG_DECLARE(icon_crop_corn_seed);
LV_IMG_DECLARE(icon_crop_corn_young);
LV_IMG_DECLARE(icon_crop_corn_grow);
LV_IMG_DECLARE(icon_crop_corn_bloom);
LV_IMG_DECLARE(icon_crop_corn_ripe);

LV_IMG_DECLARE(icon_crop_wheat);
LV_IMG_DECLARE(icon_crop_wheat_seed);
LV_IMG_DECLARE(icon_crop_wheat_young);
LV_IMG_DECLARE(icon_crop_wheat_grow);
LV_IMG_DECLARE(icon_crop_wheat_bloom);
LV_IMG_DECLARE(icon_crop_wheat_ripe);
LV_IMG_DECLARE(icon_crop_corn_ripe);

LV_IMG_DECLARE(icon_crop_rice);
LV_IMG_DECLARE(icon_crop_rice_seed);
LV_IMG_DECLARE(icon_crop_rice_young);
LV_IMG_DECLARE(icon_crop_rice_grow);
LV_IMG_DECLARE(icon_crop_rice_bloom);
LV_IMG_DECLARE(icon_crop_rice_ripe);

LV_IMG_DECLARE(icon_drone_0);
LV_IMG_DECLARE(icon_drone_1);

LV_IMG_DECLARE(icon_pest_unknown);
LV_IMG_DECLARE(icon_pest_aphid);
LV_IMG_DECLARE(icon_pest_mite);
LV_IMG_DECLARE(icon_pest_leafroller);
LV_IMG_DECLARE(icon_pest_locust);

LV_IMG_DECLARE(icon_pesticide_aphid);
LV_IMG_DECLARE(icon_pesticide_mite);
LV_IMG_DECLARE(icon_pesticide_leafroller);
LV_IMG_DECLARE(icon_pesticide_locust);

extern lv_img_dsc_t *img_prop_scarecrow;
extern lv_img_dsc_t *img_load_bg;
extern lv_img_dsc_t *img_load_btn;
extern lv_img_dsc_t *img_load_btn_pressed;

#endif
