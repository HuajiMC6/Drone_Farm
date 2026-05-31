#include "drone.h"
#include "data.h"
#include "event.h"
#include "ff.h"
#include "player.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static drone_t s_drone_storage;
static drone_t *s_drone = NULL;

// 获取无人机单例实例，首次调用自动初始化
drone_t *drone_get_instance() {
    if (s_drone == NULL) {
        drone_init();
    }
    return s_drone;
}

// 无人机模块初始化：绑定静态存储并恢复存档
void drone_init() {
    if (s_drone == NULL) {
        s_drone = &s_drone_storage;
        // 先绑定静态无人机对象，再尝试从 SD 卡恢复
        memset(s_drone, 0, sizeof(*s_drone));
        s_drone->speed = drone_speed_values[0];
        s_drone->algorithm_level = 0;
        s_drone->speed_level = 0;
        s_drone->storage_level = 0;
        s_drone->storage_capacity = drone_storage_capacity_values[0];
        for (int i = 0; i < GAME_GRID_SIZE; i++)
            for (int j = 0; j < GAME_GRID_SIZE; j++) s_drone->one_zero_matrix[i][j] = CROP_DAMAGE_NONE;
        for (int i = 0; i < CROP_PESTICIDE_NONE; i++) s_drone->pesticide_storage[i] = 0;
        s_drone->current_pos.x = 0, s_drone->current_pos.y = 0;
        s_drone->drone_state = DRONE_STATE_FREE;
        if (drone_load()) {
            return;
        }
    }
}

// 切换无人机状态并发送对应事件
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

// 检测当前位置地块的虫害类型，标记为已检测
crop_damage_t drone_detect_damage() {
    farm_t *farm = farm_get_instance();
    pos_t matrix_pos = {s_drone->current_pos.x / GAME_CELL_SIZE, s_drone->current_pos.y / GAME_CELL_SIZE};
    field_t *field = farm->fields[matrix_pos.x][matrix_pos.y];
    if (field->is_detected)
        return CROP_DAMAGE_NONE;
    else {
        field_detect(field); // 标记为已检测
        s_drone->one_zero_matrix[matrix_pos.x][matrix_pos.y] = field_get_damage(field);
        return field_get_damage(field);
    }
}

// 获取无人机检测到的所有虫害数量统计
void drone_get_detected_pest_counts(uint8_t counts[CROP_DAMAGE_NONE]) {
    for (int i = 0; i < CROP_DAMAGE_NONE; i++) counts[i] = 0;
    for (int i = 0; i < GAME_GRID_SIZE; i++)
        for (int j = 0; j < GAME_GRID_SIZE; j++) {
            crop_damage_t damage = s_drone->one_zero_matrix[i][j];
            if (damage != CROP_DAMAGE_NONE) {
                counts[damage]++;
            }
        }
}

// 升级无人机算法等级（player 接口）
bool drone_algorithm_update() { // player接口
    if (s_drone->algorithm_level >= DRONE_ALGORITHM_LEVEL_MAX)
        return false;
    s_drone->algorithm_level++;
    return true;
}

// 升级无人机飞行速度（player 接口）
bool drone_speed_update() { // player接口
    if (s_drone->speed_level >= DRONE_SPEED_LEVEL_MAX)
        return false;
    s_drone->speed_level++;
    s_drone->speed = drone_speed_values[s_drone->speed_level];
    return true;
}

// 升级无人机农药容量（player 接口）
bool drone_storage_update() { // player接口
    if (s_drone->storage_level >= DRONE_STORAGE_LEVEL_MAX)
        return false;
    s_drone->storage_level++;
    s_drone->storage_capacity = drone_storage_capacity_values[s_drone->storage_level];
    return true;
}

// 计算两点间的曼哈顿距离
static int manhattan_dist(pos_t a, pos_t b) { // 曼哈顿距离
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// 对路径执行 2-opt 局部优化（开放路径，起点固定）
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

// 基于贪心+2-opt 优化的喷洒路径规划
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

// 获取无人机自动喷洒路径（前端可视化用）
pos_t *drone_auto_path(int *out_len) { // 前端接口，用来前端写路径可视化，即飞行轨迹，并非喷药
    return optimized_greedy_algorithm(out_len);
}

// 确保对指定位置地块使用农药（有药则施药）
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

// 向无人机装载指定农药（player 接口）
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

// 从无人机卸载指定农药返回背包
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

// 根据摇杆向量移动无人机，边界自动钳制
void drone_move(pos_t vector) {
    pos_t new_pos = {s_drone->current_pos.x + (vector.x * s_drone->speed) / GAME_CELL_SIZE,
                     s_drone->current_pos.y + (vector.y * s_drone->speed) / GAME_CELL_SIZE};
    s_drone->current_pos = new_pos;
    farm_t *farm = farm_get_instance();
    if (new_pos.x >= farm->current_size * GAME_CELL_SIZE)
        s_drone->current_pos.x = farm->current_size * GAME_CELL_SIZE - 1;
    if (new_pos.x < 0)
        s_drone->current_pos.x = 0;
    if (new_pos.y >= farm->current_size * GAME_CELL_SIZE)
        s_drone->current_pos.y = farm->current_size * GAME_CELL_SIZE - 1;
    if (new_pos.y < 0)
        s_drone->current_pos.y = 0;
}

// 将无人机状态保存到 SD 卡
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

// 从 SD 卡恢复无人机状态
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

// 删除无人机存档文件
bool drone_delete() {
    // 删除无人机存档文件；文件不存在时也视为已经删除成功
    FRESULT res = f_unlink("0:/drone_save.dat");
    return res == FR_OK || res == FR_NO_FILE;
}
