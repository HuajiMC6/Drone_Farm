#include "drone.h"
#include "enum.h"
#include "event.h"
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
        memset(s_drone, 0, sizeof(*s_drone));
        s_drone->speed = 10;
        s_drone->algorithm_level = 0;
        s_drone->speed_level = 0;
        s_drone->storage_level = 0;
        s_drone->storage_capacity = 10;
        for (int i = 0; i < 10; i++)
            for (int j = 0; j < 10; j++) s_drone->one_zero_matrix[i][j] = 0;
        for (int i = 0; i < 4; i++) s_drone->pesticide_storage[i] = 0;
        s_drone->current_pos.x = 0, s_drone->current_pos.y = 0;
        s_drone->drone_state = DRONE_STATE_FREE;
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
        if (field->damage != CROP_DAMAGE_NONE)
            s_drone->one_zero_matrix[matrix_pos.x][matrix_pos.y] = 1;
        return field_get_damage(field);
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

static void reset_matrix() {
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++) s_drone->one_zero_matrix[i][j] = 0;
}

static int manhattan_dist(pos_t a, pos_t b) { // 曼哈顿距离
    return abs(a.x - b.x) + abs(a.y - b.y);
}

static pos_t *traversal_algorithm(int *out_len) { // 全部遍历，可以不写，交给前端，节省空间，但更重要的是保持一致性
    farm_t *farm = farm_get_instance();
    int n = farm->current_size;
    // 先统计病田数量，以便分配准确大小的数组
    int count = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (s_drone->one_zero_matrix[i][j] == 1)
                count++;
    if (count == 0) {
        *out_len = 0;
        return NULL;
    }
    *out_len = n * n;
    pos_t *arr = (pos_t *)malloc(sizeof(pos_t) * n * n);
    int index = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i % 2 == 0)
                arr[index].x = i, arr[index].y = j, index++;
            else
                arr[index].x = i, arr[index].y = n - 1 - j, index++;
        }
    }
    reset_matrix();
    return arr;
}

static pos_t *greedy_algorithm(int *out_len) {
    farm_t *farm = farm_get_instance();
    int n = farm->current_size;

    // 先统计病田数量，以便分配准确大小的数组
    int count = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (s_drone->one_zero_matrix[i][j] == 1)
                count++;
    *out_len = count;
    if (count == 0)
        return NULL;
    // 提取所有病田坐标
    pos_t *points = (pos_t *)malloc(sizeof(pos_t) * count);
    int idx = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (s_drone->one_zero_matrix[i][j] == 1) {
                points[idx].x = i;
                points[idx].y = j;
                idx++;
            }

    // 最近邻贪心构造路径
    pos_t *path = (pos_t *)malloc(sizeof(pos_t) * count);
    bool *visited = (bool *)calloc(count, sizeof(bool));

    pos_t cur_pos = {0, 0};
    for (int step = 0; step < count; step++) {
        int nearest = -1;
        int min_dist = 1e9;
        for (int i = 0; i < count; i++) {
            if (!visited[i]) {
                int dist = manhattan_dist(points[i], cur_pos);
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest = i;
                }
            }
        }
        path[step] = points[nearest];
        visited[nearest] = true;
        cur_pos = points[nearest];
    }
    free(points);
    free(visited);
    reset_matrix();
    return path;
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
            if (s_drone->one_zero_matrix[i][j] == 1)
                count++;
    *out_len = count;
    if (count == 0)
        return NULL;

    // 2. 提取所有病田坐标
    pos_t *points = (pos_t *)malloc(sizeof(pos_t) * count);
    int idx = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (s_drone->one_zero_matrix[i][j] == 1) {
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
    reset_matrix();

    return path;
}

pos_t *drone_auto_path(int *out_len) { // 前端接口，用来前端写路径可视化，即飞行轨迹，并非喷药
    if (s_drone->algorithm_level == 0)
        return traversal_algorithm(out_len);
    else if (s_drone->algorithm_level == 1)
        return greedy_algorithm(out_len);
    else if (s_drone->algorithm_level == 2)
        return optimized_greedy_algorithm(out_len);
    return NULL;
}

bool drone_ensure_pesticide(pos_t pos) {
    farm_t *farm = farm_get_instance();
    field_t *field = farm->fields[pos.x][pos.y];
    if (field->crop_type == CROP_TYPE_NONE || field->stage == CROP_STAGE_READY || field->damage == CROP_DAMAGE_NONE)
        return false;                                    // 期间可能死了或成熟了
    if (s_drone->pesticide_storage[field->damage] > 0) { // 有药用药
        s_drone->pesticide_storage[field->damage]--;
        field_use_pesticide(field);
        player_use_pesticide_exp();
        return true;
    }
    return false;
}

bool drone_add_pesticide(crop_pesticide_t pesticide, int n) { // player接口
    player_t *player=player_get_instance();
    if (player->pesticide_bag[pesticide]>=n&&s_drone->storage_capacity >= s_drone->pesticide_storage[pesticide] + n) {
        s_drone->pesticide_storage[pesticide] += n;
        player->pesticide_bag[pesticide]-=n;
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