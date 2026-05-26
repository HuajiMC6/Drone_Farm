#include "icon.h"

#include "drivers.h"
#include <stdlib.h>

extern int read_file_to_array(const char *filename, uint8_t *buffer, uint32_t max_size);

lv_img_dsc_t *img_prop_scarecrow;

// 外部图标加载
void icon_init(void) {
    img_prop_scarecrow = icon_sd_load("0:/images/img_prop_scarecrow.bin", LV_IMG_CF_TRUE_COLOR_ALPHA, 100, 122);
}

/*
 * 从SD卡读取图片二进制文件，返回 lv_img_dsc_t 描述符
 * 参数:
 *   path - SD卡文件路径（如 "0:images/logo.bin"）
 *   cf   - LVGL颜色格式（如 LV_IMG_CF_TRUE_COLOR）
 *   w, h - 图片宽高（像素）
 * 返回: 成功返回堆分配的 lv_img_dsc_t*，失败返回 NULL
 *       使用完毕后需调用 icon_sd_free() 释放
 */
lv_img_dsc_t *icon_sd_load(const char *path, lv_img_cf_t cf, uint16_t w, uint16_t h) {
    if (!path || w == 0 || h == 0) {
        return NULL;
    }

    /* 根据颜色格式计算期望的像素数据大小 */
    uint32_t px_size;
    switch (cf) {
        case LV_IMG_CF_TRUE_COLOR_ALPHA:
            px_size = 3;
            break;
        case LV_IMG_CF_TRUE_COLOR:
            px_size = 2;
            break;
        default:
            px_size = 2;
            break;
    }

    uint32_t expected_size = (uint32_t)w * h * px_size + 4;

    /* 用 sdram_malloc 分配像素数据到 SDRAM */
    uint8_t *px_data = sdram_malloc(expected_size);
    if (!px_data) {
        return NULL;
    }

    /* 使用 read_file_to_array 从 SD 卡读取文件到缓存 */
    int bytes_read = read_file_to_array(path, px_data, expected_size);
    if (bytes_read < 0 || (uint32_t)bytes_read != expected_size) {
        sdram_free(px_data);
        return NULL;
    }

    /* 分配描述符结构体 */
    lv_img_dsc_t *dsc = malloc(sizeof(lv_img_dsc_t));
    if (!dsc) {
        sdram_free(px_data);
        return NULL;
    }

    /* 填充 LVGL 图片描述符 */
    dsc->header.cf = cf;
    dsc->header.always_zero = 0;
    dsc->header.w = w;
    dsc->header.h = h;
    dsc->data_size = expected_size;
    dsc->data = px_data + 4; // 跳过前4字节的文件头

    return dsc;
}

// 释放由 icon_sd_load 分配的图片描述符及像素数据
void icon_sd_free(lv_img_dsc_t *dsc) {
    if (dsc) {
        if (dsc->data) {
            sdram_free((void *)dsc->data);
        }
        free(dsc);
    }
}

// 根据作物类型和生长阶段返回对应图标
const void *icon_get_crop(crop_type_t type, crop_stage_t stage) {
    static const void *const map[CROP_TYPE_NONE][CROP_STAGE_NONE] = {
        [CROP_TYPE_WHEAT][CROP_STAGE_SEED] = &icon_crop_wheat_seed,
        [CROP_TYPE_WHEAT][CROP_STAGE_YOUNG] = &icon_crop_wheat_young,
        [CROP_TYPE_WHEAT][CROP_STAGE_GROW] = &icon_crop_wheat_grow,
        [CROP_TYPE_WHEAT][CROP_STAGE_BLOOM] = &icon_crop_wheat_bloom,
        [CROP_TYPE_WHEAT][CROP_STAGE_RIPE] = &icon_crop_wheat_ripe,
        [CROP_TYPE_WHEAT][CROP_STAGE_READY] = &icon_crop_wheat_ripe,

        [CROP_TYPE_RICE][CROP_STAGE_SEED] = &icon_crop_rice_seed,
        [CROP_TYPE_RICE][CROP_STAGE_YOUNG] = &icon_crop_rice_young,
        [CROP_TYPE_RICE][CROP_STAGE_GROW] = &icon_crop_rice_grow,
        [CROP_TYPE_RICE][CROP_STAGE_BLOOM] = &icon_crop_rice_bloom,
        [CROP_TYPE_RICE][CROP_STAGE_RIPE] = &icon_crop_rice_ripe,
        [CROP_TYPE_RICE][CROP_STAGE_READY] = &icon_crop_rice_ripe,

        [CROP_TYPE_CORN][CROP_STAGE_SEED] = &icon_crop_corn_seed,
        [CROP_TYPE_CORN][CROP_STAGE_YOUNG] = &icon_crop_corn_young,
        [CROP_TYPE_CORN][CROP_STAGE_GROW] = &icon_crop_corn_grow,
        [CROP_TYPE_CORN][CROP_STAGE_BLOOM] = &icon_crop_corn_bloom,
        [CROP_TYPE_CORN][CROP_STAGE_RIPE] = &icon_crop_corn_ripe,
        [CROP_TYPE_CORN][CROP_STAGE_READY] = &icon_crop_corn_ripe,
    };

    return map[type][stage];
}

// 获取作物物品栏贴图
const void *icon_get_crop_item(crop_type_t type) {
    static const void *const map[CROP_TYPE_NONE] = {
        [CROP_TYPE_WHEAT] = &icon_crop_wheat,
        [CROP_TYPE_RICE] = &icon_crop_rice,
        [CROP_TYPE_CORN] = &icon_crop_corn,
    };

    return map[type];
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
