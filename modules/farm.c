#include "farm.h"
#include "enum.h"
#include "stdlib.h"
#include <stdbool.h>

static farm_t *s_farm = NULL;

void farm_init() {
    if (s_farm == NULL) {
        s_farm = (farm_t *)malloc(sizeof(farm_t));
        for (int i = 0; i < 10; i++)
            for (int j = 0; j < 10; j++)
                s_farm->fields[i][j] = field_init(i, j);
        s_farm->current_size = 5;
        s_farm->size_level = 0;
    }
}

farm_t *farm_get_instance() {
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