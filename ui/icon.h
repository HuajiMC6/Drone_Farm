#ifndef __ICON_H
#define __ICON_H

#include "lvgl.h"
#include "enum.h"

// 虫害图标类型
typedef enum {
	ICON_PEST_UNKNOWN     // 未知（未勘察）
} icon_pest_type_t;

const void *icon_get_crop(crop_type_t type, crop_stage_t stage);

LV_IMG_DECLARE(icon_field_bg);
LV_IMG_DECLARE(icon_farm_bg);
LV_IMG_DECLARE(icon_gold_bar_bg);
LV_IMG_DECLARE(icon_plant_btn);
LV_IMG_DECLARE(icon_shop_btn);
LV_IMG_DECLARE(icon_storage_btn);
LV_IMG_DECLARE(icon_setting_btn);
LV_IMG_DECLARE(icon_crop_corn);
LV_IMG_DECLARE(icon_crop_wheat_young);
LV_IMG_DECLARE(icon_crop_wheat_ripe);
LV_IMG_DECLARE(icon_crop_corn_ripe);

#endif
