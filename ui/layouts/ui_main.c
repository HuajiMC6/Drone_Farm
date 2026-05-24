#include "ui.h"

#include "audio.h"
#include "data.h"
#include "drone.h"
#include "icon.h"
#include "joystick.h"
#include "player.h"
#include "ui_common.h"
#include "ui_drone_cb.h"
#include "ui_grid_list.h"
#include "ui_main_cb.h"
#include "ui_message.h"
#include "ui_storage_cb.h"
#include "ui_window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FARM_GRID_N farm_get_instance()->current_size
#define FARM_BLOCK_SIZE 80
#define DRONE_COORD_SCALNG_FACTOR (FARM_BLOCK_SIZE / 100.0)

static bool drone_timer_active = false;

static lv_obj_t *g_screen_main = NULL;
static lv_obj_t *g_main_layer = NULL;
static lv_obj_t *g_scroll_part = NULL;

static lv_obj_t *farm_grid = NULL;
static lv_obj_t *g_drone = NULL;
static lv_obj_t *g_drone_still = NULL;
static lv_obj_t *g_drone_flying = NULL;

static lv_obj_t *shop_btn = NULL;
static lv_obj_t *storage_btn = NULL;
static lv_obj_t *plant_btn = NULL;
static lv_obj_t *setting_btn = NULL;
static lv_obj_t *g_seed_items[CROP_TYPE_NONE];
static lv_obj_t *g_seed_count_labels[CROP_TYPE_NONE];

static farm_block_t g_farm_blocks[10][10];
uint8_t ui_drone_pest_count[CROP_DAMAGE_NONE];

static lv_timer_t *ui_timer_update_100ms = NULL;
static lv_timer_t *ui_timer_update_1s = NULL;

static lv_obj_t *g_plant_window = NULL;
static lv_obj_t *g_shop_window = NULL;
static lv_obj_t *g_storage_window = NULL;
static lv_obj_t *g_setting_window = NULL;
static lv_obj_t *g_drone_window = NULL;
static lv_obj_t *g_field_upgrade_window = NULL;

static lv_obj_t *g_gold_bar_label = NULL;

static lv_obj_t *g_prop_scarecrow = NULL;

typedef struct {
    pos_t *path;
    int path_len;
    int path_index;
    int dwell_ticks;
    bool active;
} ui_drone_spray_ctx_t;

static ui_drone_spray_ctx_t g_drone_spray_ctx = {0};

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

typedef struct {
    lv_obj_t *level_label;
    lv_obj_t *exp_label;
    lv_obj_t *exp_bar;
} ui_player_exp_ctx_t;

ui_player_exp_ctx_t g_player_exp_ctx;

typedef struct {
    lv_obj_t *btn;
    lv_obj_t *price_label;
} ui_farm_size_upgrade_ctx_t;

ui_farm_size_upgrade_ctx_t g_farm_size_upgrade_ctx;

static lv_obj_t *ui_seed_table_create(lv_obj_t *parent);
static lv_obj_t *ui_plant_window_create(void);
static lv_obj_t *ui_setting_window_create(void);
static lv_obj_t *ui_drone_create(lv_obj_t *parent);
static void ui_drone_switch_state(bool flying);
static void ui_drone_reset_to_still(void);
static void ui_farm_grid_create(lv_obj_t *parent);
static void ui_field_update(int x, int y);
static void ui_field_update_bars(farm_block_t *block);
static lv_obj_t *ui_field_upgrade_window_create(void);
static void ui_field_upgrade_window_refresh(farm_block_t *block);
static lv_obj_t *ui_crop_growing_bar(lv_obj_t *parent);
static lv_obj_t *ui_crop_death_bar(lv_obj_t *parent);
static void ui_gold_bar_create(lv_obj_t *parent);
static void ui_gold_bar_refresh(void);
static void ui_exp_bar_create(lv_obj_t *parent);
static void ui_exp_bar_refresh(void);
static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x,
                                    lv_coord_t y);
static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb);
static void ui_update_100ms(lv_timer_t *timer);
static void ui_update_1s(lv_timer_t *timer);
static void ui_drone_timer_resume(void);
static void ui_main_icon_btns_hide(bool hide);
static void ui_seed_table_refresh(void);
static void ui_drone_spray_reset(void);
static bool ui_drone_spray_prepare(void);
static bool ui_drone_move_towards_target(pos_t cell);
static pos_t ui_drone_grid_center(pos_t cell);
static void ui_farm_grid_update(void);
static void ui_decorations_create(void);
static void ui_farm_size_upgrade_btn_create(void);
void ui_farm_size_upgrade_btn_refresh(void);

lv_obj_t *ui_shop_window_create(void);
lv_obj_t *ui_drone_window_create(void);
lv_obj_t *ui_storage_window_create(void);
void ui_storage_window_refresh(void);

static ui_window_toggle_desc_t g_plant_window_toggle = {.create = ui_plant_window_create,
                                                        .window_ref = &g_plant_window};
static ui_window_toggle_desc_t g_storage_window_toggle = {
    .create = ui_storage_window_create,
    .window_ref = NULL,
};
static ui_window_toggle_desc_t g_shop_window_toggle = {.create = ui_shop_window_create, .window_ref = &g_shop_window};
static ui_window_toggle_desc_t g_setting_window_toggle = {
    .create = ui_setting_window_create,
    .window_ref = &g_setting_window,
};
static ui_window_toggle_desc_t g_drone_window_toggle = {.create = ui_drone_window_create,
                                                        .window_ref = &g_drone_window};

static ui_window_toggle_desc_t g_field_upgrade_window_toggle = {.create = ui_field_upgrade_window_create,
                                                                .window_ref = &g_field_upgrade_window};

lv_obj_t *ui_main_screen_create(void) {
    if (g_screen_main && lv_obj_is_valid(g_screen_main)) {
        return g_screen_main;
    }

    /* 这个地方的布局优化了1mol次，走了十年弯路，特此记录，原来大道至简...我悟了...吗?... */

    g_screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_img_src(g_screen_main, &icon_farm_bg, 0);
    lv_obj_set_style_bg_img_tiled(g_screen_main, true, 0);
    lv_obj_add_flag(g_screen_main, LV_OBJ_FLAG_SCROLLABLE);

    // 关闭滑动弹性和惯性，让游戏体验更好
    lv_obj_set_scrollbar_mode(g_screen_main, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_screen_main, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(g_screen_main, LV_OBJ_FLAG_SCROLL_MOMENTUM);

    g_main_layer = ui_div_create(g_screen_main);
    lv_obj_set_size(g_main_layer, 1024, 600);
    lv_obj_set_style_pad_ver(g_main_layer, 100, 0);
    lv_obj_clear_flag(g_main_layer, LV_OBJ_FLAG_SCROLLABLE);

    ui_farm_grid_create(g_main_layer);
    lv_obj_add_flag(g_main_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_main_layer, ui_main_screen_click_cb, LV_EVENT_CLICKED, farm_grid);

    ui_gold_bar_create(g_screen_main);
    ui_exp_bar_create(g_screen_main);

    ui_drone_create(g_main_layer);
    ui_drone_hud_create(g_screen_main);
    ui_drone_set_pos(-40, 40, false, NULL);

    // 启动时获取一次虫害数据，确保无人机窗口初始显示正确
    drone_get_detected_pest_counts(ui_drone_pest_count);

    shop_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_shop_btn, 40, 380);
    storage_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_storage_btn, 40, 460);
    plant_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_plant_btn, 920, 450);
    setting_btn = ui_icon_btn_create(g_screen_main, 47, 47, &icon_setting_btn, 920, 40);

    g_storage_window_toggle.window_ref = &g_storage_window;

    ui_decorations_create();

    ui_farm_size_upgrade_btn_create();

    lv_obj_add_event_cb(plant_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_plant_window_toggle);
    lv_obj_add_event_cb(storage_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_storage_window_toggle);
    lv_obj_add_event_cb(shop_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_shop_window_toggle);
    lv_obj_add_event_cb(setting_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_setting_window_toggle);

    // lv_obj_add_event_cb(g_screen_main, ui_main_screen_click_cb, LV_EVENT_CLICKED, NULL);

    return g_screen_main;
}

void ui_main_handle_event(event_t *event) {
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

            if (event->type == EVENT_ON_PEST_DETECTED || event->type == EVENT_ON_PEST_CLEARED) {
                drone_get_detected_pest_counts(ui_drone_pest_count);
                // 虫害检测/清除时按需刷新无人机窗口
                ui_drone_window_refresh();
            }
            if (event->type == EVENT_ON_FIELD_UPGRADE && g_field_upgrade_window_ctx.current_field == data) {
                // 田地升级时、且与当前显示的field匹配时刷新田地升级窗口
                ui_field_upgrade_window_refresh(&g_farm_blocks[data->x][data->y]);
            }
            break;
        }
        case EVENT_ON_FARM_SIZE_UPGRADE:
            // 农场尺寸变化时，重建整个田地网格
            ui_farm_grid_update();

            // 将无人机移回初始位置，避免无人机位置不正确
            ui_drone_set_pos(-40, 40, false, NULL);

            ui_farm_size_upgrade_btn_refresh();
            break;
        case EVENT_ON_PLAYER_COIN_CHANGE:
        case EVENT_ON_PLAYER_SEED_CHANGE:
        case EVENT_ON_PLAYER_HARVEST_BAG_CHANGE:
        case EVENT_ON_PLAYER_PESTICIDE_CHANGE:
        case EVENT_ON_PLAYER_EXPERIENCE_CHANGE:
        case EVENT_ON_PLAYER_LEVEL_UPGRADE:
            if (event->type == EVENT_ON_PLAYER_COIN_CHANGE) {
                ui_gold_bar_refresh();
            }
            if (event->type == EVENT_ON_PLAYER_SEED_CHANGE) {
                ui_seed_table_refresh();
            }
            if (event->type == EVENT_ON_PLAYER_HARVEST_BAG_CHANGE) {
                ui_storage_window_refresh();
            }
            if (event->type == EVENT_ON_PLAYER_PESTICIDE_CHANGE) {
                ui_drone_window_refresh();
            }
            if (event->type == EVENT_ON_PLAYER_COIN_CHANGE || event->type == EVENT_ON_PLAYER_LEVEL_UPGRADE ||
                event->type == EVENT_ON_PLAYER_SEED_CHANGE) {
                ui_shop_refresh();
            }
            if (event->type == EVENT_ON_PLAYER_EXPERIENCE_CHANGE) {
                ui_exp_bar_refresh();
            }
            break;
        case EVENT_ON_DRONE_TO_FREE:
            drone_timer_active = false; // 无人机切换到空闲状态时暂停无人机更新计时器，节省资源
            if (!g_drone_spray_ctx.active) {
                ui_drone_spray_prepare();
            }
            ui_drone_set_pos(-40, 40, true, ui_drone_reset_to_still);
            ui_main_icon_btns_hide(false);
            ui_drone_hud_set_visible(false);
            ui_drone_window_refresh();
            break;
        case EVENT_ON_DRONE_TO_MOVING:
            if (drone_get_instance()->drone_state == DRONE_STATE_DETECTING) {
                ui_drone_spray_reset();
            } else if (!g_drone_spray_ctx.active) {
                ui_drone_spray_prepare();
            }
            ui_drone_set_pos(0, 0, true, ui_drone_timer_resume);
            ui_main_icon_btns_hide(true);
            if (g_drone_window && lv_obj_is_valid(g_drone_window) && ui_window_is_visible(g_drone_window)) {
                ui_window_hide(g_drone_window);
            }
            ui_drone_hud_set_visible(true);
            ui_drone_window_refresh();
            ui_drone_switch_state(true);
            break;
        default:
            break;
    }
}

void ui_main_update_timer_init(void) {
    ui_timer_update_100ms = lv_timer_create(ui_update_100ms, 100, NULL);
    ui_timer_update_1s = lv_timer_create(ui_update_1s, 1000, NULL);
    // lv_timer_pause(ui_timer_update_100ms);
}

static void ui_main_flex_cont_height_refresh(void) {
    if (FARM_GRID_N * FARM_BLOCK_SIZE + 200 > 600) {
        lv_obj_set_height(g_main_layer, FARM_GRID_N * FARM_BLOCK_SIZE + 200);
    } else {
        lv_obj_set_height(g_main_layer, 600);
    }

    // // 更新无人机位置，防止修改容器高度时无人机定位偏移
    // if (g_drone)
    //     ui_drone_set_pos(0, 0, false, NULL);
}

static void ui_farm_grid_create(lv_obj_t *parent) {
    if (farm_grid != NULL)
        return;

    farm_grid = ui_div_create(parent);
    lv_obj_set_size(farm_grid, FARM_GRID_N * FARM_BLOCK_SIZE, FARM_GRID_N * FARM_BLOCK_SIZE);
    lv_obj_center(farm_grid);
    lv_obj_move_to_index(farm_grid, 0); // 确保田地网格在最底层

    // 根据农场尺寸动态调整flex容器高度
    ui_main_flex_cont_height_refresh();

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
            lv_obj_add_event_cb(bg_layer, ui_main_field_block_click_cb, LV_EVENT_CLICKED, g_screen_main);
            lv_obj_add_event_cb(
                bg_layer, ui_main_field_block_long_press_cb, LV_EVENT_LONG_PRESSED,
                ui_field_upgrade_window_refresh); // 长按事件显示田地升级窗口时，根据当前field刷新窗口内容
            lv_obj_set_user_data(bg_layer, block);

            block->obj = ui_div_create(bg_layer);
            lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_add_flag(block->obj, LV_OBJ_FLAG_EVENT_BUBBLE); // 将作物图层的事件冒泡到bg_layer统一处理
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

static void ui_farm_grid_update(void) {
    // 先删除旧网格，再按最新农场尺寸重新创建
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
    ui_window_set_display_relative(win); // 设置为相对显示，保证在田地附近显示而不是固定位置显示
    lv_obj_set_size(win, 200, 180);

    ui_window_hide(win);                        // 初始隐藏，长按田地时显示
    ui_window_follow_scroll(win, g_main_layer); // 让窗口跟随田地网格滚动

    g_field_upgrade_window = win;

    g_field_upgrade_window_ctx.output_label = ui_field_upgrade_window_item_create(
        body, ui_main_filed_output_upgrade_click_cb, &g_field_upgrade_window_ctx.current_field,
        &g_field_upgrade_window_ctx.output_price_label, &g_field_upgrade_window_ctx.output_upgrade_btn);
    g_field_upgrade_window_ctx.ready_time_label = ui_field_upgrade_window_item_create(
        body, ui_main_ready_time_upgrade_click_cb, &g_field_upgrade_window_ctx.current_field,
        &g_field_upgrade_window_ctx.ready_time_price_label, &g_field_upgrade_window_ctx.ready_time_upgrade_btn);
    g_field_upgrade_window_ctx.tolerance_label = ui_field_upgrade_window_item_create(
        body, ui_main_tolerance_upgrade_click_cb, &g_field_upgrade_window_ctx.current_field,
        &g_field_upgrade_window_ctx.tolerance_price_label, &g_field_upgrade_window_ctx.tolerance_upgrade_btn);

    return win;
}

// 根据传入的田地块信息刷新升级窗口内容，并显示在对应田地旁边
static void ui_field_upgrade_window_refresh(farm_block_t *block) {
    if (!block) {
        return;
    }
    field_t *field = block->field;
    if (!field) {
        return;
    }

    if (!g_field_upgrade_window || !lv_obj_is_valid(g_field_upgrade_window)) {
        g_field_upgrade_window = ui_field_upgrade_window_create();
    }

    g_field_upgrade_window_ctx.current_field = block->field;

    char buf[32];
    snprintf(buf, sizeof(buf), "Output: Lv.%d", field->output_level);
    lv_label_set_text(g_field_upgrade_window_ctx.output_label, buf);

    snprintf(buf, sizeof(buf), "Ready Time: Lv.%d", field->ready_time_level);
    lv_label_set_text(g_field_upgrade_window_ctx.ready_time_label, buf);

    snprintf(buf, sizeof(buf), "Tolerance: Lv.%d", field->tolerance_level);
    lv_label_set_text(g_field_upgrade_window_ctx.tolerance_label, buf);

    if (field->output_level >= 3) {
        snprintf(buf, sizeof(buf), "Achieved Max Level");
        lv_obj_add_state(g_field_upgrade_window_ctx.output_upgrade_btn, LV_STATE_DISABLED);
    } else {
        snprintf(buf, sizeof(buf), "Upgrade Cost: %d", field_output_upgrade_price[field->output_level]);
        lv_obj_clear_state(g_field_upgrade_window_ctx.output_upgrade_btn, LV_STATE_DISABLED);
    }
    lv_label_set_text(g_field_upgrade_window_ctx.output_price_label, buf);

    if (field->ready_time_level >= 3) {
        snprintf(buf, sizeof(buf), "Achieved Max Level");
        lv_obj_add_state(g_field_upgrade_window_ctx.ready_time_upgrade_btn, LV_STATE_DISABLED);
    } else {
        snprintf(buf, sizeof(buf), "Upgrade Cost: %d", field_ready_time_upgrade_price[field->ready_time_level]);
        lv_obj_clear_state(g_field_upgrade_window_ctx.ready_time_upgrade_btn, LV_STATE_DISABLED);
    }
    lv_label_set_text(g_field_upgrade_window_ctx.ready_time_price_label, buf);

    if (field->tolerance_level >= 3) {
        snprintf(buf, sizeof(buf), "Achieved Max Level");
        lv_obj_add_state(g_field_upgrade_window_ctx.tolerance_upgrade_btn, LV_STATE_DISABLED);
    } else {
        snprintf(buf, sizeof(buf), "Upgrade Cost: %d", field_tolerance_upgrade_price[field->tolerance_level]);
        lv_obj_clear_state(g_field_upgrade_window_ctx.tolerance_upgrade_btn, LV_STATE_DISABLED);
    }
    lv_label_set_text(g_field_upgrade_window_ctx.tolerance_price_label, buf);

    lv_obj_align_to(g_field_upgrade_window, block->obj, LV_ALIGN_OUT_RIGHT_TOP, 5, 0);
    ui_window_show(g_field_upgrade_window);
}

// 切换窗口对应的田地：如果当前窗口显示的田地与传入的田地不同，则刷新窗口内容到新田地，否则隐藏窗口
void ui_field_upgrade_window_switch(farm_block_t *block) {
    if (!block)
        return;
    if (!ui_window_is_visible(g_field_upgrade_window))
        return;

    if (g_field_upgrade_window_ctx.current_field != block->field) {
        ui_field_upgrade_window_refresh(block);
    } else {
        ui_window_hide(g_field_upgrade_window);
    }
}

// static void ui_field_upgrade_window_refresh(void) {
//     field_t *field = g_field_upgrade_window_ctx.current_field;
//     if (!field || !g_field_upgrade_window || !lv_obj_is_valid(g_field_upgrade_window) ||
//         !ui_window_is_visible(g_field_upgrade_window)) {
//         return;
//     }

//     char buf[32];
//     snprintf(buf, sizeof(buf), "Output: Lv.%d", field->output_level);
//     lv_label_set_text(g_field_upgrade_window_ctx.output_label, buf);

//     snprintf(buf, sizeof(buf), "Ready Time: Lv.%d", field->ready_time_level);
//     lv_label_set_text(g_field_upgrade_window_ctx.ready_time_label, buf);

//     snprintf(buf, sizeof(buf), "Tolerance: Lv.%d", field->tolerance_level);
//     lv_label_set_text(g_field_upgrade_window_ctx.tolerance_label, buf);
// }

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

static void ui_field_update_bars(farm_block_t *block) {
    if (!block) {
        return;
    }
    if (block->is_planted) {
        lv_bar_set_range(block->growing_bar, 0, block->field->ready_time);
        lv_bar_set_value(block->growing_bar, block->field->growing_time, LV_ANIM_OFF);

        int death_percentage = field_get_death_percentage(block->field);
        if (death_percentage > 50) {
            lv_obj_clear_flag(block->death_bar, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(block->death_bar, death_percentage, LV_ANIM_OFF);
        } else {
            lv_obj_add_flag(block->death_bar, LV_OBJ_FLAG_HIDDEN);
        }

        if (death_percentage == 75) {
            char message[64];
            snprintf(message, sizeof(message), "The crop in (%d, %d) is about to die!", block->x + 1, block->y + 1);
            ui_message_show(message, UI_MESSAGE_TYPE_WARNING, UI_MESSAGE_CONFIRM);
        }
    }
}

// 创建无人机对象（静止&飞行两个状态，默认是静止状态）
static lv_obj_t *ui_drone_create(lv_obj_t *parent) {
    // 无人机飞行动画和静止图是分开的两个对象，放在同一个容器g_drone里（保证同步移动），切换时通过隐藏/显示来实现，避免频繁创建删除对象
    g_drone = ui_div_create(parent);
    lv_obj_set_size(g_drone, 40, 40);

    /* 静止状态（静态图片） */
    g_drone_still = lv_img_create(g_drone);
    lv_obj_set_pos(g_drone_still, 0, 0);
    lv_img_set_src(g_drone_still, &icon_drone_0);
    lv_obj_add_flag(g_drone_still, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_drone_still, ui_main_drone_click_cb, LV_EVENT_CLICKED, &g_drone_window_toggle);

    /* 飞行状态（帧动画） */
    g_drone_flying = lv_animimg_create(g_drone);
    lv_obj_set_pos(g_drone_flying, 0, 0);
    static const lv_img_dsc_t *drone_imgs[] = {&icon_drone_0, &icon_drone_1};
    lv_animimg_set_src(g_drone_flying, (lv_img_dsc_t **)drone_imgs, 2);
    lv_animimg_set_duration(g_drone_flying, 150);
    lv_animimg_set_repeat_count(g_drone_flying, LV_ANIM_REPEAT_INFINITE);
    lv_obj_add_flag(g_drone_flying, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_drone_flying, ui_main_drone_click_cb, LV_EVENT_CLICKED, &g_drone_window_toggle);
    lv_animimg_start(g_drone_flying);

    ui_drone_switch_state(false); // 默认显示静止状态

    return g_drone;
}

// 根据无人机状态切换显示的对象（静止图或飞行动画）
static void ui_drone_switch_state(bool flying) {
    if (flying) {
        lv_obj_add_flag(g_drone_still, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_drone_flying, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_drone_flying, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_drone_still, LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb) {
    if (!anim) {
        lv_obj_align_to(g_drone, farm_grid, LV_ALIGN_TOP_LEFT, x - 20, y - 20);
    } else {
        uint32_t speed = drone_get_instance()->speed * DRONE_COORD_SCALNG_FACTOR * 10;
        lv_coord_t x_start = lv_obj_get_x(g_drone);
        lv_coord_t y_start = lv_obj_get_y(g_drone);
        /* 统一使用相对于父对象的坐标 */
        lv_coord_t grid_x = lv_obj_get_x(farm_grid);
        lv_coord_t grid_y = lv_obj_get_y(farm_grid);
        lv_coord_t x_end = grid_x + x - 20;
        lv_coord_t y_end = grid_y + y - 20;

        uint32_t x_time = lv_anim_speed_to_time(speed, x_start, x_end);
        uint32_t y_time = lv_anim_speed_to_time(speed, y_start, y_end);

        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, g_drone);
        lv_anim_set_exec_cb(&ax, lv_obj_set_x);
        lv_anim_set_time(&ax, x_time);
        lv_anim_set_path_cb(&ax, lv_anim_path_ease_in_out);
        lv_anim_set_values(&ax, x_start, x_end);

        lv_anim_t ay;
        lv_anim_init(&ay);
        lv_anim_set_var(&ay, g_drone);
        lv_anim_set_exec_cb(&ay, lv_obj_set_y);
        lv_anim_set_time(&ay, y_time);
        lv_anim_set_path_cb(&ay, lv_anim_path_ease_in_out);
        lv_anim_set_values(&ay, y_start, y_end);

        // 根据哪个轴的动画时间更长来设置动画结束回调，确保动画完全结束后再执行回调函数
        if (x_time > y_time) {
            lv_anim_set_ready_cb(&ax, (lv_anim_ready_cb_t)anim_cb);
        } else {
            lv_anim_set_ready_cb(&ay, (lv_anim_ready_cb_t)anim_cb);
        }

        lv_anim_start(&ax);
        lv_anim_start(&ay);
    }
}

// 将无人机切换回静止状态（在飞行结束时调用）
static void ui_drone_reset_to_still(void) {
    ui_drone_switch_state(false);
}

static void ui_main_icon_btns_hide(bool hide) {
    if (hide) {
        lv_obj_add_flag(shop_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(storage_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(plant_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(setting_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(shop_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(storage_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(plant_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(setting_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

// 生长进度条
static lv_obj_t *ui_crop_growing_bar(lv_obj_t *parent) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_add_event_cb(bar, ui_main_crop_bar_draw_part_end_cb, LV_EVENT_DRAW_PART_END, NULL);
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
    lv_obj_add_event_cb(bar, ui_main_crop_bar_draw_part_end_cb, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_set_size(bar, 75, 8);
    lv_obj_set_align(bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(bar, 0, -10);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_50, LV_PART_MAIN);
    return bar;
}

static void ui_gold_bar_create(lv_obj_t *parent) {
    lv_obj_t *bar = ui_div_create(parent);
    lv_obj_set_size(bar, 130, 40);
    lv_obj_set_pos(bar, 720, 15);
    lv_obj_set_style_bg_img_src(bar, &icon_gold_bar_bg, 0);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_FLOATING);

    lv_obj_t *coin_label = lv_label_create(bar);
    lv_label_set_text_fmt(coin_label, "%d", player_get_instance()->coins);
    lv_obj_set_style_text_color(coin_label, lv_color_make(60, 42, 29), 0);
    lv_obj_align(coin_label, LV_ALIGN_RIGHT_MID, -10, 1);
    lv_obj_set_style_text_align(coin_label, LV_TEXT_ALIGN_RIGHT, 0);

    g_gold_bar_label = coin_label;
}

static void ui_gold_bar_refresh(void) {
    if (g_gold_bar_label && lv_obj_is_valid(g_gold_bar_label)) {
        lv_label_set_text_fmt(g_gold_bar_label, "%d", player_get_instance()->coins);
    }
}

static void ui_exp_bar_create(lv_obj_t *parent) {
    lv_obj_t *bar = ui_div_create(parent);
    lv_obj_set_size(bar, 206, 75);
    lv_obj_set_pos(bar, 24, 8);
    lv_obj_set_style_bg_img_src(bar, &icon_exp_bar_bg, 0);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_FLOATING);

    lv_obj_t *level_cont = ui_div_create(bar);
    lv_obj_set_size(level_cont, 75, 75);
    lv_obj_set_pos(level_cont, 0, 0);

    lv_obj_t *level_label = lv_label_create(level_cont);
    player_t *player = player_get_instance();
    lv_label_set_text_fmt(level_label, "%d", player_get_level());
    lv_obj_set_style_text_color(level_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(level_label, &lv_font_montserrat_30, 0);
    lv_obj_center(level_label);

    lv_obj_t *exp_label = lv_label_create(bar);
    lv_label_set_text_fmt(exp_label, "%d / %d", player_get_experience(), player_get_next_level_experience());
    lv_obj_set_style_text_color(exp_label, lv_color_hex(0xC05E01), 0);
    lv_obj_align(exp_label, LV_ALIGN_BOTTOM_RIGHT, -14, -16);

    lv_obj_t *exp_bar = lv_bar_create(bar);
    lv_obj_set_size(exp_bar, 128, 18);
    lv_obj_set_pos(exp_bar, 66, 19);
    lv_obj_set_style_bg_color(exp_bar, lv_color_hex(0xFEC709), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(exp_bar, lv_color_hex(0xFCAC0B), LV_PART_MAIN);
    lv_bar_set_range(exp_bar, player_get_this_level_experience(), player_get_next_level_experience());
    lv_bar_set_value(exp_bar, player_get_experience(), LV_ANIM_OFF);

    g_player_exp_ctx.level_label = level_label;
    g_player_exp_ctx.exp_label = exp_label;
    g_player_exp_ctx.exp_bar = exp_bar;
}

static void ui_exp_bar_refresh(void) {
    player_t *player = player_get_instance();
    if (!player) {
        return;
    }

    if (g_player_exp_ctx.level_label && lv_obj_is_valid(g_player_exp_ctx.level_label)) {
        lv_label_set_text_fmt(g_player_exp_ctx.level_label, "%d", player_get_level());
    }
    if (g_player_exp_ctx.exp_label && lv_obj_is_valid(g_player_exp_ctx.exp_label)) {
        lv_label_set_text_fmt(g_player_exp_ctx.exp_label, "%d / %d", player_get_experience(),
                              player_get_next_level_experience());
    }
    if (g_player_exp_ctx.exp_bar && lv_obj_is_valid(g_player_exp_ctx.exp_bar)) {
        lv_bar_set_range(g_player_exp_ctx.exp_bar, player_get_this_level_experience(),
                         player_get_next_level_experience());
        lv_bar_set_value(g_player_exp_ctx.exp_bar, player_get_experience(), LV_ANIM_OFF);
    }
}

static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x,
                                    lv_coord_t y) {
    lv_obj_t *btn = ui_div_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_img_src(btn, img, 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING);

    // 点击时的阴影效果
    lv_obj_set_style_shadow_width(btn, 8, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_40, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_ofs_x(btn, 1, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_ofs_y(btn, 2, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(btn, 1, LV_STATE_PRESSED);

    lv_obj_add_event_cb(btn, icon_btns_click_audio_play, LV_EVENT_RELEASED, NULL);

    return btn;
}

static lv_obj_t *ui_seed_table_create(lv_obj_t *parent) {
    ui_grid_list_cfg_t cfg;
    ui_grid_list_cfg_init(&cfg);
    cfg.item_w = 60;
    cfg.item_h = 60;
    cfg.col_count = 3;
    cfg.row_count = 3;

    ui_grid_list_t *list = ui_grid_list_create(parent, &cfg);
    if (!list) {
        return NULL;
    }

    lv_obj_t *grid = ui_grid_list_get_obj(list);

    static ui_drag_to_plant_desc_t seeds[CROP_TYPE_NONE];

    crop_type_t i;
    lv_obj_t *obj;
    for (i = 0; i < CROP_TYPE_NONE; i++) {
        obj = ui_grid_list_add_item(list);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_PRESS_LOCK);
        if (!obj) {
            break;
        }
        g_seed_items[i] = obj;

        const void *drag_img = icon_get_crop_item(i);

        lv_obj_t *item_img = lv_img_create(obj);
        if (drag_img) {
            lv_img_set_src(item_img, drag_img);
        }
        lv_obj_align(item_img, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t *item1_label = lv_label_create(obj);
        lv_label_set_text(item1_label, crop_type_name(i));
        lv_obj_set_style_text_color(item1_label, lv_color_make(60, 42, 29), 0);
        lv_obj_set_style_text_align(item1_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(item1_label, LV_ALIGN_BOTTOM_MID, 0, 0);

        g_seed_count_labels[i] = lv_label_create(obj);
        lv_label_set_text(g_seed_count_labels[i], "x0");
        lv_obj_set_style_text_color(g_seed_count_labels[i], lv_color_make(60, 42, 29), 0);
        lv_obj_align(g_seed_count_labels[i], LV_ALIGN_TOP_RIGHT, -2, 2);

        seeds[i] = (ui_drag_to_plant_desc_t){.type = i, .img = drag_img, .fields = farm_grid};
        ui_grid_list_bind_item_event(obj, ui_main_seed_drag_event_cb, LV_EVENT_PRESSING, &seeds[i]);
        ui_grid_list_bind_item_event(obj, ui_main_seed_drag_event_cb, LV_EVENT_RELEASED, &seeds[i]);
    }

    ui_seed_table_refresh();

    return grid;
}

static void ui_seed_table_refresh(void) {
    player_t *player = player_get_instance();
    if (!player) {
        return;
    }

    for (crop_type_t i = 0; i < CROP_TYPE_NONE; i++) {
        if (g_seed_count_labels[i] && lv_obj_is_valid(g_seed_count_labels[i])) {
            lv_label_set_text_fmt(g_seed_count_labels[i], "x%d", player->seed_bag[i]);
        }

        if (g_seed_items[i] && lv_obj_is_valid(g_seed_items[i])) {
            if (player->seed_bag[i] <= 0) {
                lv_obj_add_state(g_seed_items[i], LV_STATE_DISABLED);
            } else {
                lv_obj_clear_state(g_seed_items[i], LV_STATE_DISABLED);
            }
        }
    }
}

static lv_obj_t *ui_plant_window_create(void) {
    lv_obj_t *grid = ui_seed_table_create(g_screen_main);

    lv_obj_t *div = ui_window_create("PLANT", grid, false);
    lv_obj_set_align(div, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(div, -20, -20);
    lv_obj_set_size(div, 206, 290);

    g_plant_window = div;

    return div;
}

static lv_obj_t *ui_setting_window_create(void) {
    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_t *div = ui_window_create("SETTING", body, true);

    lv_obj_t *btn = lv_btn_create(body);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Reset Game");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(btn, ui_setting_reset_game_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn2 = lv_btn_create(body);
    lv_obj_set_size(btn2, 120, 50);
    lv_obj_set_pos(btn2, 0, 70);
    lv_obj_t *btn2_label = lv_label_create(btn2);
    lv_label_set_text(btn2_label, "Add coins");
    lv_obj_center(btn2_label);
    lv_obj_add_event_cb(btn2, ui_setting_add_coins_cb, LV_EVENT_CLICKED, NULL);

    /*Create a slider in the center of the display*/
    lv_obj_t *slider = lv_slider_create(body);
    lv_obj_center(slider);
    /*Create a label below the slider*/
    lv_obj_t *slider_label = lv_label_create(body);
    lv_label_set_text(slider_label, "1000ms");
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(slider, debug_timer_period_slider_event_cb, LV_EVENT_VALUE_CHANGED, slider_label);

    /* Volume Slider */
    lv_obj_t *volume_slider = lv_slider_create(body);
    lv_obj_center(volume_slider);
    lv_obj_set_pos(volume_slider, 0, 100);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, 100, LV_ANIM_OFF);
    lv_obj_t *volume_label = lv_label_create(body);
    lv_label_set_text_fmt(volume_label, "Volume: %d%%", 100);
    lv_obj_align_to(volume_label, volume_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(volume_slider, debug_volume_slider_event_cb, LV_EVENT_VALUE_CHANGED, volume_label);

    // lv_obj_t *btn3 = lv_btn_create(body);
    // lv_obj_set_size(btn3, 120, 50);
    // lv_obj_set_pos(btn3, 0, 180);
    // lv_obj_t *btn3_label = lv_label_create(btn3);
    // lv_label_set_text(btn3_label, "Screenshot");
    // lv_obj_center(btn3_label);
    // lv_obj_add_event_cb(btn3, debug_screenshot_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_center(div);
    lv_obj_set_size(div, 300, 400);

    g_setting_window = div;

    return div;
}

// 装饰物生成
static void ui_decorations_create(void) {
    // 稻草人
    g_prop_scarecrow = lv_img_create(g_main_layer);
    lv_img_set_src(g_prop_scarecrow, img_prop_scarecrow);
    lv_obj_set_pos(g_prop_scarecrow, 70, 120);
}

// 田地大小升级按钮刷新
void ui_farm_size_upgrade_btn_refresh(void) {
    if (farm_get_instance()->size_level >= FARM_SIZE_LEVEL_MAX) {
        if (g_farm_size_upgrade_ctx.btn && lv_obj_is_valid(g_farm_size_upgrade_ctx.btn)) {
            lv_obj_add_state(g_farm_size_upgrade_ctx.btn, LV_STATE_DISABLED);
            lv_label_set_text(g_farm_size_upgrade_ctx.price_label, "Max Level");
        }
    } else {
        if (g_farm_size_upgrade_ctx.price_label && lv_obj_is_valid(g_farm_size_upgrade_ctx.price_label)) {
            lv_label_set_text_fmt(g_farm_size_upgrade_ctx.price_label, "Cost: %d",
                                  farm_size_update_price[farm_get_instance()->size_level]);
        }
    }
    lv_obj_align_to(g_farm_size_upgrade_ctx.btn, farm_grid, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_align_to(g_farm_size_upgrade_ctx.price_label, g_farm_size_upgrade_ctx.btn, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// 田地大小升级按钮创建
static void ui_farm_size_upgrade_btn_create(void) {
    lv_obj_t *btn = lv_btn_create(g_main_layer);
    lv_obj_set_size(btn, 170, 40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xefcd76), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x8a6333), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_pad_all(btn, 3, 0);
    lv_obj_align_to(btn, farm_grid, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Upgrade Farm Size");
    lv_obj_align_to(btn_label, btn, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *price_label = lv_label_create(btn);
    lv_obj_set_style_text_color(price_label, lv_color_hex(0xb66258), 0);
    lv_obj_set_style_text_font(price_label, &lv_font_montserrat_12, 0);
    lv_obj_add_event_cb(btn, ui_main_field_size_upgrade_click_cb, LV_EVENT_CLICKED, NULL);

    g_farm_size_upgrade_ctx.btn = btn;
    g_farm_size_upgrade_ctx.price_label = price_label;

    ui_farm_size_upgrade_btn_refresh();
}

static void ui_update_100ms(lv_timer_t *timer) {
    (void)timer;

    /* 无人机 */
    if (drone_timer_active) {
        drone_t *drone = drone_get_instance();
        if (drone->drone_state == DRONE_STATE_DETECTING) {
            pos_t vector = {.x = joystick_get_dir_x(), .y = joystick_get_dir_y()};
            drone_move(vector);
            pos_t pos = drone->current_pos;
            ui_drone_set_pos(pos.x * DRONE_COORD_SCALNG_FACTOR, pos.y * DRONE_COORD_SCALNG_FACTOR, false, NULL);

            if (!g_farm_blocks[pos.x / 100][pos.y / 100].is_detected) {
                crop_damage_t pest = drone_detect_damage();
                if (pest != CROP_DAMAGE_NONE) {
                    ui_drone_pest_count[pest]++;
                }
            }
        } else if (drone->drone_state == DRONE_STATE_AUTO) {
            if (!g_drone_spray_ctx.active && !ui_drone_spray_prepare()) {
                drone_state_switch(DRONE_STATE_FREE);
                return;
            }

            if (g_drone_spray_ctx.path_index >= g_drone_spray_ctx.path_len) {
                ui_drone_spray_reset();
                drone_state_switch(DRONE_STATE_FREE);
                return;
            }

            pos_t cell = g_drone_spray_ctx.path[g_drone_spray_ctx.path_index];

            if (g_drone_spray_ctx.dwell_ticks > 0) { // 无人机在目标格停留，模拟喷洒农药
                g_drone_spray_ctx.dwell_ticks--;
                if (g_drone_spray_ctx.dwell_ticks == 0) { // 停留结束，执行喷洒效果
                    crop_damage_t pest = field_get_damage(g_farm_blocks[cell.x][cell.y].field);
                    if (drone_ensure_pesticide(cell)) {
                        // 这里暂时先注释掉了，感觉消息太多了很烦
                        // ui_message_show("Pesticide sprayed successfully!", UI_MESSAGE_TYPE_SUCCESS,
                        //                 UI_MESSAGE_TOAST);
                    } else {
                        if (pest != CROP_DAMAGE_NONE) {
                            char message[64];
                            snprintf(message, sizeof(message), "Not enough pesticide against %s!",
                                     crop_pest_name(pest));
                            ui_message_show(message, UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
                        }
                    }
                    g_drone_spray_ctx.path_index++;
                    if (g_drone_spray_ctx.path_index >= g_drone_spray_ctx.path_len) {
                        ui_drone_spray_reset();
                        drone_state_switch(DRONE_STATE_FREE);
                        ui_message_show("Finished spraying all detected ill fields!", UI_MESSAGE_TYPE_SUCCESS,
                                        UI_MESSAGE_TOAST);
                        return;
                    }
                }
            } else if (ui_drone_move_towards_target(cell)) {
                g_drone_spray_ctx.dwell_ticks = 5; // 在目标格停留一段时间（dwell_ticks个定时器周期），模拟喷洒农药
            }
        }
    }

    /* 装饰物 */
    if (g_prop_scarecrow) {
        float angle = sinf(lv_tick_get() * 0.002f) * 30;
        lv_img_set_angle(g_prop_scarecrow, angle);
    }
}

static void ui_update_1s(lv_timer_t *timer) {
    (void)timer;

    for (int i = 0; i < FARM_GRID_N; i++) {
        for (int j = 0; j < FARM_GRID_N; j++) {
            farm_block_t *block = &g_farm_blocks[i][j];
            ui_field_update_bars(block);
        }
    }
}

static void ui_drone_timer_resume(void) {
    drone_timer_active = true;
}

static void ui_drone_spray_reset(void) {
    if (g_drone_spray_ctx.path) {
        free(g_drone_spray_ctx.path);
        g_drone_spray_ctx.path = NULL;
    }

    g_drone_spray_ctx.path_len = 0;
    g_drone_spray_ctx.path_index = 0;
    g_drone_spray_ctx.dwell_ticks = 0;
    g_drone_spray_ctx.active = false;
}

static bool ui_drone_spray_prepare(void) {
    if (g_drone_spray_ctx.active) {
        return true;
    }

    int path_len = 0;
    pos_t *path = drone_auto_path(&path_len);
    if (!path || path_len <= 0) {
        if (path) {
            free(path);
        }
        return false;
    }

    g_drone_spray_ctx.path = path;
    g_drone_spray_ctx.path_len = path_len;
    g_drone_spray_ctx.path_index = 0;
    g_drone_spray_ctx.dwell_ticks = 0;
    g_drone_spray_ctx.active = true;
    return true;
}

static pos_t ui_drone_grid_center(pos_t cell) {
    return (pos_t){.x = cell.x * 100 + 50, .y = cell.y * 100 + 50};
}

static bool ui_drone_move_towards_target(pos_t cell) {
    drone_t *drone = drone_get_instance();
    int step = drone->speed;
    pos_t target = ui_drone_grid_center(cell);

    int dx = target.x - drone->current_pos.x;
    int dy = target.y - drone->current_pos.y;

    if (abs(dx) <= step) {
        drone->current_pos.x = target.x;
    } else {
        drone->current_pos.x += dx > 0 ? step : -step;
    }

    if (abs(dy) <= step) {
        drone->current_pos.y = target.y;
    } else {
        drone->current_pos.y += dy > 0 ? step : -step;
    }

    ui_drone_set_pos(drone->current_pos.x * DRONE_COORD_SCALNG_FACTOR, drone->current_pos.y * DRONE_COORD_SCALNG_FACTOR,
                     false, NULL);
    return drone->current_pos.x == target.x && drone->current_pos.y == target.y;
}
