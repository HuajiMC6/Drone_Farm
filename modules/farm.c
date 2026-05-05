#include "farm.h"
#include "enum.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "ff.h"

static farm_t s_farm_storage;
static farm_t *s_farm = NULL;

void farm_init() {
    if (s_farm == NULL) {
        s_farm = &s_farm_storage;
        memset(s_farm, 0, sizeof(*s_farm));
        for (int i = 0; i < 10; i++)
            for (int j = 0; j < 10; j++) s_farm->fields[i][j] = field_init(i, j);
        s_farm->current_size = 5;
        s_farm->size_level = 0;
    }
}

farm_t *farm_get_instance() {
    if (s_farm == NULL) {
        farm_init();
    }
    return s_farm;
}

bool farm_size_update() {
    if (s_farm->size_level >= 3)
        return false;
    s_farm->size_level++;
    if (s_farm->size_level == 0)
        s_farm->current_size = 5;
    else if (s_farm->size_level == 1)
        s_farm->current_size = 7;
    else if (s_farm->size_level == 2)
        s_farm->current_size = 9;
    else if (s_farm->size_level == 3)
        s_farm->current_size = 10;
    return true;
}

void farm_grow() {
    for (int i = 0; i < s_farm->current_size; i++)
        for (int j = 0; j < s_farm->current_size; j++)
            if (s_farm->fields[i][j]->crop_type != CROP_TYPE_NONE)
                field_grow(s_farm->fields[i][j]);
}

int get_farm_size() {
    return s_farm->current_size;
}

bool farm_save() {
    if (!s_farm) return false;

    FIL fil;
    UINT bw;
    if (f_open(&fil, "0:/farm_save.dat", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;

    int size = s_farm->current_size;

    // 写入农场头部
    if (f_write(&fil, &size, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
        f_close(&fil);
        return false;
    }
    if (f_write(&fil, &s_farm->size_level, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
        f_close(&fil);
        return false;
    }

    // 逐个地块写入
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            field_t *f = s_farm->fields[i][j];
            if (!f) continue;

            if (f_write(&fil, &f->x, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->y, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            if (f_write(&fil, &f->output_level, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->ready_time_level, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->tolerance_level, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            int crop = (int)f->crop_type;
            if (f_write(&fil, &crop, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            if (f_write(&fil, &f->ready_time, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->growing_time, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->growing_percent, sizeof(double), &bw) != FR_OK || bw != sizeof(double)) {
                f_close(&fil);
                return false;
            }

            int stage = (int)f->stage;
            if (f_write(&fil, &stage, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            int damage = (int)f->damage;
            if (f_write(&fil, &damage, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            int damaged  = f->is_damaged  ? 1 : 0;
            int detected = f->is_detected ? 1 : 0;
            if (f_write(&fil, &damaged, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &detected, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            if (f_write(&fil, &f->base_output, sizeof(int), &bw) != FR_OK || bw != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->factor, sizeof(double), &bw) != FR_OK || bw != sizeof(double)) {
                f_close(&fil);
                return false;
            }
            if (f_write(&fil, &f->extra_factor, sizeof(double), &bw) != FR_OK || bw != sizeof(double)) {
                f_close(&fil);
                return false;
            }

            if (f_write(&fil, &f->tolerance, sizeof(double), &bw) != FR_OK || bw != sizeof(double)) {
                f_close(&fil);
                return false;
            }
        }

    f_close(&fil);
    return true;
}

bool farm_load() {
    FIL fil;
    UINT br;
    if (f_open(&fil, "0:/farm_save.dat", FA_READ) != FR_OK)
        return false;

    farm_t *farm = farm_get_instance();

    int size;
    if (f_read(&fil, &size, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
        f_close(&fil);
        return false;
    }
    if (f_read(&fil, &farm->size_level, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
        f_close(&fil);
        return false;
    }
    farm->current_size = size;

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            field_t *f = farm->fields[i][j];
            if (!f) continue;

            if (f_read(&fil, &f->x, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->y, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            if (f_read(&fil, &f->output_level, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->ready_time_level, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->tolerance_level, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }

            int crop;
            if (f_read(&fil, &crop, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            f->crop_type = (crop_type_t)crop;

            if (f_read(&fil, &f->ready_time, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->growing_time, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->growing_percent, sizeof(double), &br) != FR_OK || br != sizeof(double)) {
                f_close(&fil);
                return false;
            }

            int stage;
            if (f_read(&fil, &stage, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            f->stage = (crop_stage_t)stage;

            int damage;
            if (f_read(&fil, &damage, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            f->damage = (crop_damage_t)damage;

            int damaged, detected;
            if (f_read(&fil, &damaged, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &detected, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            f->is_damaged  = damaged  ? true : false;
            f->is_detected = detected ? true : false;

            if (f_read(&fil, &f->base_output, sizeof(int), &br) != FR_OK || br != sizeof(int)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->factor, sizeof(double), &br) != FR_OK || br != sizeof(double)) {
                f_close(&fil);
                return false;
            }
            if (f_read(&fil, &f->extra_factor, sizeof(double), &br) != FR_OK || br != sizeof(double)) {
                f_close(&fil);
                return false;
            }

            if (f_read(&fil, &f->tolerance, sizeof(double), &br) != FR_OK || br != sizeof(double)) {
                f_close(&fil);
                return false;
            }
        }

    f_close(&fil);
    return true;
}