#include "icon.h"

// 根据作物类型和生长阶段返回对应图标
const void *icon_get_crop(crop_type_t type, crop_stage_t stage) {
    static const void *const map[CROP_TYPE_NONE][CROP_STAGE_NONE] = {
        [CROP_TYPE_WHEAT][CROP_STAGE_SEED] = &icon_crop_wheat_seed,
        [CROP_TYPE_WHEAT][CROP_STAGE_YOUNG] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_GROW] = &icon_crop_wheat_grow,
        [CROP_TYPE_WHEAT][CROP_STAGE_BLOOM] = &icon_crop_wheat_bloom,
        [CROP_TYPE_WHEAT][CROP_STAGE_RIPE] = &icon_crop_wheat_ripe,
        [CROP_TYPE_WHEAT][CROP_STAGE_READY] = &icon_crop_wheat_ripe,
    };

    return map[type][stage];
}

// 根据虫害类型返回对应图标
const void *icon_get_pest(crop_damage_t pest) {
    static const void *const map[CROP_DAMAGE_NONE] = {
        [CROP_DAMAGE_APHID] = &icon_pest_aphid,
        [CROP_DAMAGE_MITE] = &icon_pest_mite,
        [CROP_DAMAGE_LEAFROLLER] = &icon_pest_leafroller,
        [CROP_DAMAGE_LOCUST] = &icon_pest_locust,
    };

    return map[pest];
}

// 根据农药类型返回对应图标
const void *icon_get_pesticide(crop_pesticide_t pesticide) {
    static const void *const map[CROP_PESTICIDE_NONE] = {
        [CROP_PESTICIDE_APHICIDE] = &icon_pesticide_aphid,
        [CROP_PESTICIDE_ACARICIDE] = &icon_pesticide_mite,
        [CROP_PESTICIDE_LEAFROLLERICIDE] = &icon_pesticide_leafroller,
        [CROP_PESTICIDE_LOCUSTICIDE] = &icon_pesticide_locust,
    };

    return map[pesticide];
}