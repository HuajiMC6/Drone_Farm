#include "farm.h"
#include "data.h"
#include "event.h"
#include "ff.h"
#include "save_utils.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static farm_t s_farm_storage;
static farm_t *s_farm = NULL;

// 农场模块初始化：创建所有地块并恢复存档
void farm_init() {
    if (s_farm == NULL) {
        s_farm = &s_farm_storage;
        // 先建立静态农场对象，再尝试从 SD 卡恢复
        memset(s_farm, 0, sizeof(*s_farm));
        for (int i = 0; i < GAME_GRID_SIZE; i++)
            for (int j = 0; j < GAME_GRID_SIZE; j++) s_farm->fields[i][j] = field_init(i, j);
        s_farm->current_size = farm_size_by_level[0];
        s_farm->size_level = 0;
        if (farm_load()) {
            return;
        }
    }
}

// 获取农场单例实例，首次调用自动初始化
farm_t *farm_get_instance() {
    if (s_farm == NULL) {
        farm_init();
    }
    return s_farm;
}

// 升级农场大小（拓展地块数）
bool farm_size_update() {
    if (s_farm->size_level >= FARM_SIZE_LEVEL_MAX)
        return false;
    s_farm->size_level++;
    s_farm->current_size = farm_size_by_level[s_farm->size_level];
    // 农场大小变化后通知 UI 重新构建田地网格
    event_send(EVENT_ON_FARM_SIZE_UPGRADE, s_farm);
    return true;
}

// 驱动全农场所有已种植地块生长一个单位时间
void farm_grow() {
    for (int i = 0; i < s_farm->current_size; i++)
        for (int j = 0; j < s_farm->current_size; j++)
            if (s_farm->fields[i][j]->crop_type != CROP_TYPE_NONE)
                field_grow(s_farm->fields[i][j]);
}

// 获取当前农场尺寸（单边地块数）
int get_farm_size() {
    return s_farm->current_size;
}

// 将农场状态及所有地块数据保存到 SD 卡
bool farm_save() {
    if (!s_farm)
        return false;

    FIL fil;
    if (f_open(&fil, "0:/farm_save.dat", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;

    // 写入农场头部
    int size = s_farm->current_size;
    SAVE_CHECK(save_write_int(&fil, size));
    SAVE_CHECK(save_write_int(&fil, s_farm->size_level));

    // 逐个地块写入
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            field_t *f = s_farm->fields[i][j];
            if (!f)
                continue;

            SAVE_CHECK(save_write_int(&fil, f->x));
            SAVE_CHECK(save_write_int(&fil, f->y));
            SAVE_CHECK(save_write_int(&fil, f->output_level));
            SAVE_CHECK(save_write_int(&fil, f->ready_time_level));
            SAVE_CHECK(save_write_int(&fil, f->tolerance_level));
            SAVE_CHECK(save_write_int(&fil, (int)f->crop_type));
            SAVE_CHECK(save_write_int(&fil, f->ready_time));
            SAVE_CHECK(save_write_int(&fil, f->growing_time));
            SAVE_CHECK(save_write_double(&fil, f->growing_percent));
            SAVE_CHECK(save_write_int(&fil, (int)f->stage));
            SAVE_CHECK(save_write_int(&fil, (int)f->damage));
            SAVE_CHECK(save_write_int(&fil, f->is_detected ? 1 : 0));
            SAVE_CHECK(save_write_int(&fil, f->base_output));
            SAVE_CHECK(save_write_double(&fil, f->factor));
            SAVE_CHECK(save_write_double(&fil, f->tolerance));
        }

    f_close(&fil);
    return true;
}

// 从 SD 卡恢复农场状态及所有地块数据
bool farm_load() {
    FIL fil;
    if (f_open(&fil, "0:/farm_save.dat", FA_READ) != FR_OK)
        return false;

    // 直接读回静态农场存储区，避免递归调用 farm_get_instance()
    if (!s_farm) {
        s_farm = &s_farm_storage;
    }
    farm_t *farm = s_farm;

    // 读取农场头部
    int size;
    SAVE_CHECK(save_read_int(&fil, &size));
    SAVE_CHECK(save_read_int(&fil, &farm->size_level));
    farm->current_size = size;

    // 逐个地块读取
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            field_t *f = farm->fields[i][j];
            if (!f)
                continue;

            int tmp;

            SAVE_CHECK(save_read_int(&fil, &f->x));
            SAVE_CHECK(save_read_int(&fil, &f->y));
            SAVE_CHECK(save_read_int(&fil, &f->output_level));
            SAVE_CHECK(save_read_int(&fil, &f->ready_time_level));
            SAVE_CHECK(save_read_int(&fil, &f->tolerance_level));

            SAVE_CHECK(save_read_int(&fil, &tmp));
            f->crop_type = (crop_type_t)tmp;

            SAVE_CHECK(save_read_int(&fil, &f->ready_time));
            SAVE_CHECK(save_read_int(&fil, &f->growing_time));
            SAVE_CHECK(save_read_double(&fil, &f->growing_percent));

            SAVE_CHECK(save_read_int(&fil, &tmp));
            f->stage = (crop_stage_t)tmp;

            SAVE_CHECK(save_read_int(&fil, &tmp));
            f->damage = (crop_damage_t)tmp;

            SAVE_CHECK(save_read_int(&fil, &tmp));
            f->is_detected = tmp ? true : false;

            SAVE_CHECK(save_read_int(&fil, &f->base_output));
            SAVE_CHECK(save_read_double(&fil, &f->factor));
            SAVE_CHECK(save_read_double(&fil, &f->tolerance));
        }

    f_close(&fil);
    return true;
}

// 删除农场存档文件
bool farm_delete() {
    // 删除农场存档文件；文件不存在时也视为已经删除成功
    FRESULT res = f_unlink("0:/farm_save.dat");
    return res == FR_OK || res == FR_NO_FILE;
}
