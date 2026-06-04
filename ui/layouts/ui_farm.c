#include "ui_farm.h"

#include "icon.h"
#include "player.h"
#include "ui.h"
#include "ui_farm_cb.h"
#include "ui_main_cb.h"
#include "ui_message.h"
#include "ui_window.h"

#include <stdio.h>
#include <string.h>

#define FARM_GRID_N farm_get_instance()->current_size
#define FARM_BLOCK_SIZE 80

static lv_obj_t *g_screen_main = NULL;
static lv_obj_t *g_main_layer = NULL;
static lv_obj_t *farm_grid = NULL;
static farm_block_t g_farm_blocks[10][10];
static lv_obj_t *g_field_upgrade_window = NULL;

// 地块升级窗口上下文
typedef struct {
    lv_obj_t *output_label;
    lv_obj_t *ready_time_label;
    lv_obj_t *tolerance_label;

    lv_obj_t *output_price_label;
    lv_obj_t *ready_time_price_label;
    lv_obj_t *tolerance_price_label;

    lv_obj_t *output_upgrade_btn;
    lv_obj_t *ready_time_upgrade_btn;
    lv_obj_t *tolerance_upgrade_btn;

    field_t *current_field;
} ui_field_upgrade_window_ctx_t;

static ui_field_upgrade_window_ctx_t g_field_upgrade_window_ctx;

static void ui_farm_flex_cont_height_refresh(void);
static void ui_farm_grid_create(lv_obj_t *parent);
static void ui_farm_grid_update(void);
static lv_obj_t *ui_field_upgrade_window_item_create(lv_obj_t *parent, lv_event_cb_t btn_event_cb,
                                                     void *event_user_data, lv_obj_t **price_label, lv_obj_t **btn);
static lv_obj_t *ui_field_upgrade_window_create(void);
static void ui_field_update(int x, int y);
static void ui_field_update_bars(farm_block_t *block);
static lv_obj_t *ui_crop_growing_bar(lv_obj_t *parent);
static lv_obj_t *ui_crop_death_bar(lv_obj_t *parent);

// 根据当前农田大小动态调整容器高度
static void ui_farm_flex_cont_height_refresh(void) {
    if (FARM_GRID_N * FARM_BLOCK_SIZE + 200 > 600) {
        lv_obj_set_height(g_main_layer, FARM_GRID_N * FARM_BLOCK_SIZE + 200);
    } else {
        lv_obj_set_height(g_main_layer, 600);
    }
}

// 生长进度条
static lv_obj_t *ui_crop_growing_bar(lv_obj_t *parent) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_add_event_cb(bar, ui_farm_crop_bar_draw_part_end_cb, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_set_size(bar, 75, 8);
    lv_obj_set_align(bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(bar, 0, -2);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_50, LV_PART_MAIN);
    return bar;
}

// 死亡倒计时进度条（仅在作物即将死亡时显示）
static lv_obj_t *ui_crop_death_bar(lv_obj_t *parent) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_add_event_cb(bar, ui_farm_crop_bar_draw_part_end_cb, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_set_size(bar, 75, 8);
    lv_obj_set_align(bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(bar, 0, -10);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_50, LV_PART_MAIN);
    return bar;
}

// 更新指定农田格子的生长和死亡进度条状态
static void ui_field_update_bars(farm_block_t *block) {
    if (!block) {
        return;
    }
    if (block->is_planted) {
        lv_bar_set_range(block->growing_bar, 0, block->field->ready_time);
        lv_bar_set_value(block->growing_bar, block->field->growing_time, LV_ANIM_OFF);

        int old_death_percentage = lv_bar_get_value(block->death_bar);
        int death_percentage = field_get_death_percentage(block->field);
        if (death_percentage > 50) {
            lv_obj_clear_flag(block->death_bar, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(block->death_bar, death_percentage, LV_ANIM_OFF);
        } else {
            lv_obj_add_flag(block->death_bar, LV_OBJ_FLAG_HIDDEN);
        }

        // 当死亡进度达到75%且作物正在感染虫害时，弹出警告提示玩家作物即将死亡
        if (death_percentage != old_death_percentage /* 防止重复提示 */
            && death_percentage == 75 && field_is_damaged(block->field)) {
            char message[64];
            snprintf(message, sizeof(message), "The crop in (%d, %d) is about to die!", block->x + 1, block->y + 1);
            ui_message_show(message, UI_MESSAGE_TYPE_WARNING, UI_MESSAGE_CONFIRM);
        }
    }
}

// 更新指定地块
static void ui_field_update(int x, int y) {
    farm_block_t *block = &g_farm_blocks[x][y];
    block->is_planted = block->field->crop_type != CROP_TYPE_NONE;
    block->has_pest = field_is_damaged(block->field);
    block->is_detected = block->field->is_detected;

    ui_field_update_bars(block);

    if (block->is_planted) {
        lv_obj_clear_flag(block->obj, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(block->crop_img, icon_get_crop(block->field->crop_type, block->field->stage));

        if (block->has_pest) {
            lv_obj_clear_flag(block->pest_img, LV_OBJ_FLAG_HIDDEN);
            lv_img_set_src(block->pest_img,
                           block->is_detected ? icon_get_pest(block->field->damage) : &icon_pest_unknown);
        } else {
            lv_obj_add_flag(block->pest_img, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(block->obj, LV_OBJ_FLAG_HIDDEN);
    }
}

// 创建农田
static void ui_farm_grid_create(lv_obj_t *parent) {
    if (farm_grid != NULL) {
        return;
    }

    farm_grid = ui_div_create(parent);
    lv_obj_set_size(farm_grid, FARM_GRID_N * FARM_BLOCK_SIZE, FARM_GRID_N * FARM_BLOCK_SIZE);
    lv_obj_center(farm_grid);
    lv_obj_move_to_index(farm_grid, 0);

    ui_farm_flex_cont_height_refresh();

    for (int i = 0; i < FARM_GRID_N; i++) {
        for (int j = 0; j < FARM_GRID_N; j++) {
            field_t *field = farm_get_instance()->fields[i][j];
            farm_block_t *block = &g_farm_blocks[i][j];
            block->field = field;
            block->is_planted = field->crop_type != CROP_TYPE_NONE;
            block->has_pest = field_is_damaged(field);
            block->is_detected = field->is_detected;
            block->x = i;
            block->y = j;

            lv_obj_t *bg_layer = ui_div_create(farm_grid);
            lv_obj_set_size(bg_layer, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_set_pos(bg_layer, FARM_BLOCK_SIZE * i, FARM_BLOCK_SIZE * j);
            lv_obj_set_style_bg_img_src(bg_layer, &icon_field_bg, 0);
            lv_obj_add_flag(bg_layer, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_add_flag(bg_layer, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_pad_all(bg_layer, -2, 0);
            lv_obj_set_style_border_width(bg_layer, 2, LV_STATE_CHECKED);
            lv_obj_add_event_cb(bg_layer, ui_farm_field_block_click_cb, LV_EVENT_CLICKED, g_screen_main);
            lv_obj_add_event_cb(bg_layer, ui_farm_field_block_long_press_cb, LV_EVENT_LONG_PRESSED, NULL);
            lv_obj_set_user_data(bg_layer, block);

            block->obj = ui_div_create(bg_layer);
            lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_add_flag(block->obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_center(block->obj);

            block->growing_bar = ui_crop_growing_bar(block->obj);
            lv_bar_set_range(block->growing_bar, 0, field->ready_time);
            lv_bar_set_value(block->growing_bar, field->growing_time, LV_ANIM_OFF);

            block->death_bar = ui_crop_death_bar(block->obj);
            lv_obj_add_flag(block->death_bar, LV_OBJ_FLAG_HIDDEN);

            block->crop_img = lv_img_create(block->obj);
            lv_obj_center(block->crop_img);

            block->pest_img = lv_img_create(block->obj);
            lv_obj_align(block->pest_img, LV_ALIGN_TOP_RIGHT, -4, 4);

            if (block->is_planted) {
                lv_img_set_src(block->crop_img, icon_get_crop(field->crop_type, field->stage));

                if (block->has_pest) {
                    lv_img_set_src(block->pest_img,
                                   block->is_detected ? icon_get_pest(block->field->damage) : &icon_pest_unknown);
                } else {
                    lv_obj_add_flag(block->pest_img, LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                lv_obj_add_flag(block->obj, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// 更新农田网格（删除后重新创建）
static void ui_farm_grid_update(void) {
    if (g_screen_main && farm_grid) {
        lv_obj_remove_event_cb_with_user_data(g_screen_main, ui_main_screen_click_cb, farm_grid);
    }
    if (farm_grid && lv_obj_is_valid(farm_grid)) {
        lv_obj_del(farm_grid);
    }
    farm_grid = NULL;
    memset(g_farm_blocks, 0, sizeof(g_farm_blocks));
    ui_farm_grid_create(g_main_layer);
    if (g_screen_main && farm_grid) {
        lv_obj_add_event_cb(g_screen_main, ui_main_screen_click_cb, LV_EVENT_CLICKED, farm_grid);
    }
}

// 地块升级窗口升级项ui模板创建
static lv_obj_t *ui_field_upgrade_window_item_create(lv_obj_t *parent, lv_event_cb_t btn_event_cb,
                                                     void *event_user_data, lv_obj_t **price_label, lv_obj_t **btn) {
    lv_obj_t *cont = ui_div_create(parent);
    lv_obj_set_size(cont, lv_pct(100), 32);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(cont);

    *price_label = lv_label_create(cont);
    lv_obj_align_to(*price_label, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(*price_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(*price_label, lv_color_hex(0xb66258), 0);

    *btn = lv_btn_create(cont);
    lv_obj_set_size(*btn, 28, 28);
    lv_obj_set_style_bg_color(*btn, lv_color_hex(0xf4cdca), 0);
    lv_obj_set_style_border_color(*btn, lv_color_hex(0xb66258), 0);
    lv_obj_set_style_border_width(*btn, 1, 0);
    lv_obj_align(*btn, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_t *btn_label = lv_label_create(*btn);
    lv_label_set_text(btn_label, "+");
    lv_obj_center(btn_label);
    if (btn_event_cb) {
        lv_obj_add_event_cb(*btn, btn_event_cb, LV_EVENT_CLICKED, event_user_data);
    }

    return label;
}

// 地块升级窗口创建
static lv_obj_t *ui_field_upgrade_window_create(void) {
    lv_obj_t *body = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 8, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 8, 0);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t *win = ui_window_create("Field Info", body, false);
    lv_obj_set_size(win, 200, 180);

    ui_window_hide(win);
    ui_window_follow_scroll(win, g_main_layer);

    g_field_upgrade_window = win;

    g_field_upgrade_window_ctx.output_label = ui_field_upgrade_window_item_create(
        body, ui_farm_output_upgrade_click_cb, &g_field_upgrade_window_ctx.current_field,
        &g_field_upgrade_window_ctx.output_price_label, &g_field_upgrade_window_ctx.output_upgrade_btn);
    g_field_upgrade_window_ctx.ready_time_label = ui_field_upgrade_window_item_create(
        body, ui_farm_ready_time_upgrade_click_cb, &g_field_upgrade_window_ctx.current_field,
        &g_field_upgrade_window_ctx.ready_time_price_label, &g_field_upgrade_window_ctx.ready_time_upgrade_btn);
    g_field_upgrade_window_ctx.tolerance_label = ui_field_upgrade_window_item_create(
        body, ui_farm_tolerance_upgrade_click_cb, &g_field_upgrade_window_ctx.current_field,
        &g_field_upgrade_window_ctx.tolerance_price_label, &g_field_upgrade_window_ctx.tolerance_upgrade_btn);

    return win;
}

// 地块升级窗口刷新并展示
void ui_field_upgrade_window_refresh(farm_block_t *block) {
    if (!block || !block->field) {
        return;
    }

    if (!g_field_upgrade_window || !lv_obj_is_valid(g_field_upgrade_window)) {
        g_field_upgrade_window = ui_field_upgrade_window_create();
    }

    field_t *field = block->field;
    g_field_upgrade_window_ctx.current_field = block->field;

    char buf[32];
    snprintf(buf, sizeof(buf), "Output: Lv.%d", field->output_level);
    lv_label_set_text(g_field_upgrade_window_ctx.output_label, buf);

    snprintf(buf, sizeof(buf), "Ready Time: Lv.%d", field->ready_time_level);
    lv_label_set_text(g_field_upgrade_window_ctx.ready_time_label, buf);

    snprintf(buf, sizeof(buf), "Tolerance: Lv.%d", field->tolerance_level);
    lv_label_set_text(g_field_upgrade_window_ctx.tolerance_label, buf);

    double discount = level_discount[player_get_instance()->level_stage];

    if (field->output_level >= 3) {
        snprintf(buf, sizeof(buf), "Achieved Max Level");
        lv_obj_add_state(g_field_upgrade_window_ctx.output_upgrade_btn, LV_STATE_DISABLED);
    } else {
        int price = field_output_upgrade_price[field->output_level];
        int discount_price = (int)(price * discount);
        if (discount_price < price) {
            snprintf(buf, sizeof(buf), "Up Cost: %d (x%.2f)", discount_price, discount);
        } else {
            snprintf(buf, sizeof(buf), "Up Cost: %d", price);
        }
        lv_obj_clear_state(g_field_upgrade_window_ctx.output_upgrade_btn, LV_STATE_DISABLED);
    }
    lv_label_set_text(g_field_upgrade_window_ctx.output_price_label, buf);

    if (field->ready_time_level >= 3) {
        snprintf(buf, sizeof(buf), "Achieved Max Level");
        lv_obj_add_state(g_field_upgrade_window_ctx.ready_time_upgrade_btn, LV_STATE_DISABLED);
    } else {
        int price = field_ready_time_upgrade_price[field->ready_time_level];
        int discount_price = (int)(price * discount);
        if (discount_price < price) {
            snprintf(buf, sizeof(buf), "Up Cost: %d (x%.2f)", discount_price, discount);
        } else {
            snprintf(buf, sizeof(buf), "Up Cost: %d", price);
        }
        lv_obj_clear_state(g_field_upgrade_window_ctx.ready_time_upgrade_btn, LV_STATE_DISABLED);
    }
    lv_label_set_text(g_field_upgrade_window_ctx.ready_time_price_label, buf);

    if (field->tolerance_level >= 3) {
        snprintf(buf, sizeof(buf), "Achieved Max Level");
        lv_obj_add_state(g_field_upgrade_window_ctx.tolerance_upgrade_btn, LV_STATE_DISABLED);
    } else {
        int price = field_tolerance_upgrade_price[field->tolerance_level];
        int discount_price = (int)(price * discount);
        if (discount_price < price) {
            snprintf(buf, sizeof(buf), "Up Cost: %d (x%.2f)", discount_price, discount);
        } else {
            snprintf(buf, sizeof(buf), "Up Cost: %d", price);
        }
        lv_obj_clear_state(g_field_upgrade_window_ctx.tolerance_upgrade_btn, LV_STATE_DISABLED);
    }
    lv_label_set_text(g_field_upgrade_window_ctx.tolerance_price_label, buf);

    lv_obj_align_to(g_field_upgrade_window, block->obj, LV_ALIGN_OUT_RIGHT_TOP, 5, 0);
    ui_window_show(g_field_upgrade_window);
}

// 切换地块刷新窗口对应地块
void ui_field_upgrade_window_switch(farm_block_t *block) {
    if (!block || !ui_window_is_visible(g_field_upgrade_window)) {
        return;
    }

    if (g_field_upgrade_window_ctx.current_field != block->field) {
        ui_field_upgrade_window_refresh(block);
    } else {
        ui_window_hide(g_field_upgrade_window);
    }
}

// 农田模块创建
void ui_farm_module_create(lv_obj_t *screen_main, lv_obj_t *main_layer) {
    g_screen_main = screen_main;
    g_main_layer = main_layer;

    ui_farm_grid_create(g_main_layer);
    lv_obj_add_flag(g_main_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_main_layer, ui_main_screen_click_cb, LV_EVENT_CLICKED, farm_grid);
}

// 暴露给外部的接口：获取农田网格对象
lv_obj_t *ui_farm_get_grid(void) {
    return farm_grid;
}

// 根据坐标获取对应的农田格子数据结构指针
farm_block_t *ui_farm_get_block(int x, int y) {
    if (x < 0 || y < 0 || x >= 10 || y >= 10) {
        return NULL;
    }
    return &g_farm_blocks[x][y];
}

// 刷新农田所有地块的进度条状态
void ui_farm_refresh_all(void) {
    for (int i = 0; i < FARM_GRID_N; i++) {
        for (int j = 0; j < FARM_GRID_N; j++) {
            ui_field_update_bars(&g_farm_blocks[i][j]);
        }
    }
}

// 清除农田所有地块的选中状态（用于点击空白处时取消选中）
void ui_farm_clear_field_selection(lv_obj_t *parent) {
    lv_obj_t *child;
    uint8_t idx = 0;
    while ((child = lv_obj_get_child(parent, idx++)) != NULL) {
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE)) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }
}

// 农田模块事件处理
void ui_farm_handle_event(event_t *event) {
    if (!event) {
        return;
    }

    switch (event->type) {
        case EVENT_ON_FIELD_PLANTED:
        case EVENT_ON_FIELD_CLEARED:
        case EVENT_ON_FIELD_HARVESTED:
        case EVENT_ON_CROP_STAGE_CHANGE:
        case EVENT_ON_PEST_DETECTED:
        case EVENT_ON_PEST_SUFFERING:
        case EVENT_ON_PEST_CLEARED:
        case EVENT_ON_FIELD_UPGRADE: {
            field_t *data = event->data;
            ui_field_update(data->x, data->y);
            if (event->type == EVENT_ON_FIELD_UPGRADE && g_field_upgrade_window_ctx.current_field == data) {
                ui_field_upgrade_window_refresh(&g_farm_blocks[data->x][data->y]);
            }
            break;
        }
        case EVENT_ON_FARM_SIZE_UPGRADE:
            ui_farm_grid_update();
            break;
        default:
            break;
    }
}
