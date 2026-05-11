#include "drone.h"
#include "enum.h"
#include "event.h"
#include "ff.h"
#include "player.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static drone_t s_drone_storage;
static drone_t *s_drone = NULL;

drone_t *drone_get_instance() {
    if (s_drone == NULL) {
        drone_init();
    }
    return s_drone;
}

void drone_init() {
    if (s_drone == NULL) {
        s_drone = &s_drone_storage;
        // 先绑定静态无人机对象，再尝试从 SD 卡恢复
        memset(s_drone, 0, sizeof(*s_drone));
        s_drone->speed = 10;
        s_drone->algorithm_level = 0;
        s_drone->speed_level = 0;
        s_drone->storage_level = 0;
        s_drone->storage_capacity = 10;
        for (int i = 0; i < 10; i++)
            for (int j = 0; j < 10; j++) s_drone->one_zero_matrix[i][j] = CROP_DAMAGE_NONE;
        for (int i = 0; i < 4; i++) s_drone->pesticide_storage[i] = 0;
        s_drone->current_pos.x = 0, s_drone->current_pos.y = 0;
        s_drone->drone_state = DRONE_STATE_FREE;
        if (drone_load()) {
            return;
        }
    }
}

void drone_state_switch(drone_state_t drone_state) {
    s_drone->drone_state = drone_state;
    if (drone_state == DRONE_STATE_FREE) {
        s_drone->current_pos.x = 0;
        s_drone->current_pos.y = 0;
        event_send(EVENT_ON_DRONE_TO_FREE, NULL);
    } else {
        event_send(EVENT_ON_DRONE_TO_MOVING, NULL);
    }
}

crop_damage_t drone_detect_damage() {
    farm_t *farm = farm_get_instance();
    pos_t matrix_pos = {s_drone->current_pos.x / 100, s_drone->current_pos.y / 100};
    field_t *field = farm->fields[matrix_pos.x][matrix_pos.y];
    if (field->is_detected)
        return CROP_DAMAGE_NONE;
    else {
        field_detect(field); // 标记为已检测
        s_drone->one_zero_matrix[matrix_pos.x][matrix_pos.y] = field_get_damage(field);
        return field_get_damage(field);
    }
}

void drone_get_detected_pest_counts(uint8_t counts[CROP_DAMAGE_NONE]) {
    for (int i = 0; i < CROP_DAMAGE_NONE; i++) counts[i] = 0;
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++) {
            crop_damage_t damage = s_drone->one_zero_matrix[i][j];
            if (damage != CROP_DAMAGE_NONE) {
                counts[damage]++;
            }
        }
}

bool drone_algorithm_update() { // player接口
    if (s_drone->algorithm_level >= 2)
        return false;
    s_drone->algorithm_level++;
    return true;
}

bool drone_speed_update() { // player接口
    if (s_drone->speed_level >= 3)
        return false;
    s_drone->speed_level++;
    if (s_drone->speed_level == 0)
        s_drone->speed = 10;
    else if (s_drone->speed_level == 1)
        s_drone->speed = 20;
    else if (s_drone->speed_level == 2)
        s_drone->speed = 30;
    else if (s_drone->speed_level == 3)
        s_drone->speed = 50;
    return true;
}

bool drone_storage_update() { // player接口
    if (s_drone->storage_level >= 3)
        return false;
    s_drone->storage_level++;
    if (s_drone->storage_level == 0)
        s_drone->storage_capacity = 10;
    else if (s_drone->storage_level == 1)
        s_drone->storage_capacity = 20;
    else if (s_drone->storage_level == 2)
        s_drone->storage_capacity = 30;
    else if (s_drone->storage_level == 3)
        s_drone->storage_capacity = 50;
    return true;
}

static int manhattan_dist(pos_t a, pos_t b) { // 曼哈顿距离
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// 对路径执行 2-opt 优化（开放路径，起点固定）
static void two_opt_optimize(pos_t *path, int count) {
    if (count < 2)
        return;
    bool improved = true;
    while (improved) {
        improved = false;
        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count - 1; j++) {
                // 当前边：path[i]->path[i+1] 和 path[j]->path[j+1]
                // 新边：path[i]->path[j] 和 path[i+1]->path[j+1]
                int old_dist = manhattan_dist(path[i], path[i + 1]) + manhattan_dist(path[j], path[j + 1]);
                int new_dist = manhattan_dist(path[i], path[j]) + manhattan_dist(path[i + 1], path[j + 1]);
                if (new_dist < old_dist) {
                    // 反转 i+1 到 j 之间的所有节点
                    int left = i + 1, right = j;
                    while (left < right) {
                        pos_t tmp = path[left];
                        path[left] = path[right];
                        path[right] = tmp;
                        left++;
                        right--;
                    }
                    improved = true;
                }
            }
        }
    }
}

static pos_t *optimized_greedy_algorithm(int *out_len) {
    farm_t *farm = farm_get_instance();
    int n = farm->current_size;

    // 1. 统计病田数量
    int count = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (s_drone->one_zero_matrix[i][j] != CROP_DAMAGE_NONE)
                count++;
    *out_len = count;
    if (count == 0)
        return NULL;

    // 2. 提取所有病田坐标
    pos_t *points = (pos_t *)malloc(sizeof(pos_t) * count);
    int idx = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (s_drone->one_zero_matrix[i][j] != CROP_DAMAGE_NONE) {
                points[idx].x = i;
                points[idx].y = j;
                idx++;
            }

    // 3. 最近邻贪心构造初始路径（与 greedy_algorithm 相同）
    pos_t *path = (pos_t *)malloc(sizeof(pos_t) * count);
    bool *visited = (bool *)calloc(count, sizeof(bool));
    int cur_x = 0, cur_y = 0;
    for (int step = 0; step < count; step++) {
        int nearest = -1;
        int min_dist = 1e9;
        for (int i = 0; i < count; i++) {
            if (!visited[i]) {
                int dist = abs(points[i].x - cur_x) + abs(points[i].y - cur_y);
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest = i;
                }
            }
        }
        path[step] = points[nearest];
        visited[nearest] = true;
        cur_x = points[nearest].x;
        cur_y = points[nearest].y;
    }
    free(points);
    free(visited);

    // 4. 应用 2-opt 优化
    two_opt_optimize(path, count);
    // reset_matrix();

    return path;
}

pos_t *drone_auto_path(int *out_len) { // 前端接口，用来前端写路径可视化，即飞行轨迹，并非喷药
    return optimized_greedy_algorithm(out_len);
}

bool drone_ensure_pesticide(pos_t pos) {
    farm_t *farm = farm_get_instance();
    field_t *field = farm->fields[pos.x][pos.y];
    if (field->crop_type == CROP_TYPE_NONE || field->stage == CROP_STAGE_READY || !field_is_damaged(field)) {
        s_drone->one_zero_matrix[pos.x][pos.y] = CROP_DAMAGE_NONE; // 死了或成熟了标记为无病
        return false;                                              // 期间可能死了或成熟了
    }
    if (s_drone->pesticide_storage[field->damage] > 0) { // 有药用药
        s_drone->pesticide_storage[field->damage]--;
        field_use_pesticide(field);
        player_use_pesticide_exp();
        s_drone->one_zero_matrix[pos.x][pos.y] = CROP_DAMAGE_NONE; // 喷药后标记为无病
        event_send(EVENT_ON_PLAYER_PESTICIDE_CHANGE, player_get_instance());
        return true;
    }
    return false;
}

bool drone_add_pesticide(crop_pesticide_t pesticide, int n) { // player接口
    player_t *player = player_get_instance();
    if (player->pesticide_bag[pesticide] >= n &&
        s_drone->storage_capacity >= s_drone->pesticide_storage[pesticide] + n) {
        s_drone->pesticide_storage[pesticide] += n;
        player->pesticide_bag[pesticide] -= n;
        event_send(EVENT_ON_PLAYER_PESTICIDE_CHANGE, player);
        return true;
    }
    return false;
}

bool drone_remove_pesticide(crop_pesticide_t pesticide, int n) {
    player_t *player = player_get_instance();
    if (s_drone->pesticide_storage[pesticide] >= n) {
        s_drone->pesticide_storage[pesticide] -= n;
        player->pesticide_bag[pesticide] += n;
        event_send(EVENT_ON_PLAYER_PESTICIDE_CHANGE, player);
        return true;
    }
    return false;
}

void drone_move(pos_t vector) {
    pos_t new_pos = {s_drone->current_pos.x + (vector.x * s_drone->speed) / 100,
                     s_drone->current_pos.y + (vector.y * s_drone->speed) / 100};
    s_drone->current_pos = new_pos;
    farm_t *farm = farm_get_instance();
    if (new_pos.x >= farm->current_size * 100)
        s_drone->current_pos.x = farm->current_size * 100 - 1;
    if (new_pos.x < 0)
        s_drone->current_pos.x = 0;
    if (new_pos.y >= farm->current_size * 100)
        s_drone->current_pos.y = farm->current_size * 100 - 1;
    if (new_pos.y < 0)
        s_drone->current_pos.y = 0;
}

bool drone_save() {
    if (!s_drone)
        return false;

    FIL fil;
    UINT bw;
    if (f_open(&fil, "0:/drone_save.dat", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;

    if (f_write(&fil, s_drone, sizeof(drone_t), &bw) != FR_OK || bw != sizeof(drone_t)) {
        f_close(&fil);
        return false;
    }

    f_close(&fil);
    return true;
}

bool drone_load() {
    FIL fil;
    UINT br;
    if (f_open(&fil, "0:/drone_save.dat", FA_READ) != FR_OK)
        return false;

    // 直接读回静态无人机存储区，避免再次进入 drone_init()
    if (!s_drone) {
        s_drone = &s_drone_storage;
    }

    drone_t *drone = s_drone;
    if (f_read(&fil, drone, sizeof(drone_t), &br) != FR_OK || br != sizeof(drone_t)) {
        f_close(&fil);
        return false;
    }

    f_close(&fil);
    return true;
}

bool drone_delete() {
    // 删除无人机存档文件；文件不存在时也视为已经删除成功
    FRESULT res = f_unlink("0:/drone_save.dat");
    return res == FR_OK || res == FR_NO_FILE;
}