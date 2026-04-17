#include "icon.h"

const void *icon_get_crop(crop_type_t type, crop_stage_t stage) {
    static const void *const map[CROP_TYPE_NONE][CROP_STAGE_NONE] = {
        [CROP_TYPE_WHEAT][CROP_STAGE_SEED] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_YOUNG] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_GROW] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_BLOOM] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_RIPE] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_READY] = &icon_crop_wheat_ripe,
    };

    return map[type][stage];
}
