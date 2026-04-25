#include "ui.h"
#include "drone.h"
#include "enum.h"
#include "icon.h"
#include "joystick.h"
#include "sdram_malloc.h"
#include "ui_common.h"
#include "ui_event.h"
#include "ui_grid_list.h"
#include "ui_window.h"

#define FARM_GRID_N farm_get_instance()->current_size
#define FARM_BLOCK_SIZE 80
#define DRONE_COORD_SCALNG_FACTOR (FARM_BLOCK_SIZE / 100.0)

static lv_obj_t *g_screen_load;
static lv_obj_t *g_screen_main;

static lv_obj_t *farm_grid = NULL;
static lv_obj_t *g_drone = NULL;

static lv_obj_t *shop_btn = NULL;
static lv_obj_t *storage_btn = NULL;
static lv_obj_t *plant_btn = NULL;
static lv_obj_t *setting_btn = NULL;

drone_window_ctx_t g_drone_window_ctx;

static farm_block_t g_farm_blocks[10][10];
static uint8_t pest_count[CROP_DAMAGE_NONE];

typedef struct {
    crop_pesticide_t pesticide;
    int delta;
} drone_pesticide_btn_desc_t;

typedef struct {
    drone_state_t target_state;
} drone_mode_btn_desc_t;

static drone_pesticide_btn_desc_t g_drone_pesticide_btn_desc[CROP_PESTICIDE_NONE][2];
static drone_mode_btn_desc_t g_drone_detect_btn_desc = {.target_state = DRONE_STATE_DETECTING};
static drone_mode_btn_desc_t g_drone_spray_btn_desc = {.target_state = DRONE_STATE_AUTO};

lv_timer_t *ui_timer_drone_update_100ms;
lv_timer_t *ui_timer_update_1s;

static void ui_screen_main_create();
static void ui_farm_grid_create(lv_obj_t *parent);
static void ui_field_update(int x, int y);
static lv_obj_t *ui_crop_grwoing_bar(lv_obj_t *parent);
static void ui_gold_bar_create(lv_obj_t *parent);
static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x,
                                    lv_coord_t y);
static lv_obj_t *ui_seed_table_create(lv_obj_t *parent);
static lv_obj_t *ui_plant_window_create();
static lv_obj_t *ui_storage_window_create();
static lv_obj_t *ui_shop_window_create();
static lv_obj_t *ui_setting_window_create();
static lv_obj_t *ui_drone_create(lv_obj_t *parent);
static lv_obj_t *drone_window_create();
static const void *ui_crop_drag_img(crop_type_t type);
static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb);
static void ui_drone_update_100ms();
static void ui_update_1s();
static void ui_drone_timer_resume();
static void ui_main_icon_btns_hide(bool hide);
static void ui_drone_window_refresh(void);
static void drone_mode_btn_click_cb(lv_event_t *e);
static void drone_pesticide_btn_click_cb(lv_event_t *e);
static int ui_drone_pesticide_used(const drone_t *drone);
static void ui_drone_btn_set_text(lv_obj_t *btn, const char *text);

static lv_obj_t *g_plant_window = NULL;
static lv_obj_t *g_storage_window = NULL;
static lv_obj_t *g_shop_window = NULL;
static lv_obj_t *g_setting_window = NULL;

static ui_window_toggle_desc_t g_plant_window_toggle = {.create = ui_plant_window_create,
                                                        .window_ref = &g_plant_window};
static ui_window_toggle_desc_t g_storage_window_toggle = {
    .create = ui_storage_window_create,
    .window_ref = &g_storage_window,
};
static ui_window_toggle_desc_t g_shop_window_toggle = {.create = ui_shop_window_create, .window_ref = &g_shop_window};
static ui_window_toggle_desc_t g_setting_window_toggle = {
    .create = ui_setting_window_create,
    .window_ref = &g_setting_window,
};
static ui_window_toggle_desc_t g_drone_window_toggle = {.create = drone_window_create,
                                                        .window_ref = &g_drone_window_ctx.obj};

void ui_init(void) {
    ui_screen_main_create();

    ui_screen_switch(UI_SCREEN_MAIN);
}

void ui_screen_switch(ui_screen_type_t screen) {
    switch (screen) {
        case UI_SCREEN_LOAD:
            lv_scr_load(g_screen_load);
            break;
        case UI_SCREEN_MAIN:
            lv_scr_load(g_screen_main);
            break;
        default:
            lv_scr_load(g_screen_main);
    }
}

void ui_event_handler(event_t *event) {
    if (event == NULL)
        return;

    switch (event->type) // 后续细致处理
    {
        case EVENT_ON_FIELD_PLANTED:
        case EVENT_ON_FIELD_CLEARED:
        case EVENT_ON_FIELD_HARVESTED:
        case EVENT_ON_CROP_STAGE_CHANGE:
        case EVENT_ON_PEST_DETECTED:
        case EVENT_ON_PEST_SUFFERING:
        case EVENT_ON_PEST_CLEARED:
        case EVENT_ON_FIELD_UPGRADE:
            field_t *data = event->data;
            ui_field_update(data->x, data->y);
            break;
        case EVENT_ON_PLAYER_COIN_CHANGE:
        case EVENT_ON_PLAYER_EXPERIENCE_CHANGE:
        case EVENT_ON_PLAYER_LEVEL_UPGRADE:
            // Handle player-related events
            break;
        case EVENT_ON_DRONE_TO_FREE:
            lv_timer_pause(ui_timer_drone_update_100ms);
            ui_drone_set_pos(-40, 40, true, NULL);
            ui_main_icon_btns_hide(false);
            ui_drone_window_refresh();
            break;
        case EVENT_ON_DRONE_TO_MOVING:
            ui_drone_set_pos(0, 0, true, ui_drone_timer_resume);
            ui_main_icon_btns_hide(true);
            ui_drone_window_refresh();
            break;
        default:
            break;
    }
}

static void ui_screen_main_create() {
    g_screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_img_src(g_screen_main, &icon_farm_bg, 0);
    lv_obj_set_style_bg_img_tiled(g_screen_main, true, 0);

    /* 田地 */
    ui_farm_grid_create(g_screen_main);
    lv_obj_add_event_cb(g_screen_main, screen_main_click_cb, LV_EVENT_CLICKED, farm_grid);

    /* 金币显示 */
    ui_gold_bar_create(g_screen_main);

    /* 无人机 */
    g_drone = ui_drone_create(g_screen_main);
    ui_drone_set_pos(-40, 40, false, NULL);

    /* 相关按钮 */
    shop_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_shop_btn, 40, 380);
    storage_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_storage_btn, 40, 460);
    plant_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_plant_btn, 920, 450);
    setting_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_setting_btn, 920, 40);

    /* 按钮回调 */
    lv_obj_add_event_cb(plant_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_plant_window_toggle);
    lv_obj_add_event_cb(storage_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_storage_window_toggle);
    lv_obj_add_event_cb(shop_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_shop_window_toggle);
    lv_obj_add_event_cb(setting_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_setting_window_toggle);

    lv_obj_add_event_cb(g_screen_main, screen_main_click_cb, LV_EVENT_CLICKED, NULL);
}

static void ui_farm_grid_create(lv_obj_t *parent) {
    if (farm_grid != NULL)
        return;

    farm_grid = ui_div_create(parent);
    lv_obj_set_size(farm_grid, FARM_GRID_N * FARM_BLOCK_SIZE, FARM_GRID_N * FARM_BLOCK_SIZE + 160);
    lv_obj_set_align(farm_grid, LV_ALIGN_TOP_MID);
    lv_obj_set_style_pad_ver(farm_grid, 80, 0);

    for (int i = 0; i < FARM_GRID_N; i++) {
        for (int j = 0; j < FARM_GRID_N; j++) {
            field_t *field = farm_get_instance()->fields[i][j];
            farm_block_t *block = &g_farm_blocks[i][j];
            block->field = field;
            block->is_planted = field->crop_type != CROP_TYPE_NONE;
            block->has_pest = field->is_damaged;
            block->is_detected = field->is_detected;
            block->x = i;
            block->y = j;

            /* 田地 */
            lv_obj_t *bg_layer = ui_div_create(farm_grid);
            lv_obj_set_size(bg_layer, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_set_pos(bg_layer, FARM_BLOCK_SIZE * i, FARM_BLOCK_SIZE * j);
            lv_obj_set_style_bg_img_src(bg_layer, &icon_field_bg, 0);
            lv_obj_add_flag(bg_layer, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_add_flag(bg_layer, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_pad_all(bg_layer, -2, 0);
            lv_obj_set_style_border_width(bg_layer, 2, LV_STATE_CHECKED);
            lv_obj_add_event_cb(bg_layer, farm_block_click_cb, LV_EVENT_CLICKED, g_screen_main);
            lv_obj_set_user_data(bg_layer, block);

            block->obj = ui_div_create(bg_layer);
            lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_add_flag(block->obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_center(block->obj);

            /* 生长进度条 */
            block->growing_bar = ui_crop_grwoing_bar(block->obj);
            lv_bar_set_range(block->growing_bar, 0, field->ready_time);
            lv_bar_set_value(block->growing_bar, field->growing_time, LV_ANIM_OFF);

            /* 作物贴图 */
            block->crop_img = lv_img_create(block->obj);
            lv_obj_center(block->crop_img);

            /* 虫害类型 */
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

static void ui_field_update(int x, int y) {
    farm_block_t *block = &g_farm_blocks[x][y];
    block->is_planted = block->field->crop_type != CROP_TYPE_NONE;
    block->has_pest = block->field->is_damaged;
    block->is_detected = block->field->is_detected;

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

static lv_obj_t *ui_drone_create(lv_obj_t *parent) {
    lv_obj_t *drone = lv_animimg_create(parent);
    static const lv_img_dsc_t *drone_imgs[] = {&icon_drone_0, &icon_drone_1};
    lv_animimg_set_src(drone, (lv_img_dsc_t **)drone_imgs, 2);
    lv_animimg_set_duration(drone, 150);
    lv_animimg_set_repeat_count(drone, LV_ANIM_REPEAT_INFINITE);
    lv_obj_add_flag(drone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(drone, drone_click_cb, LV_EVENT_CLICKED, &g_drone_window_toggle);
    lv_animimg_start(drone);
    // lv_obj_set_size(drone, 40, 40);

    // lv_obj_set_pos(drone, -60, 20);

    return drone;
}

static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb) {
    if (!anim) {
        lv_obj_align_to(g_drone, farm_grid, LV_ALIGN_TOP_LEFT, x - 20, y - 20);
    } else {
        uint32_t speed = drone_get_instance()->speed * DRONE_COORD_SCALNG_FACTOR * 10;
        lv_coord_t x_start = lv_obj_get_x(g_drone);
        lv_coord_t y_start = lv_obj_get_y(g_drone);
        lv_area_t coords;
        lv_obj_get_content_coords(farm_grid, &coords);
        lv_coord_t x_end = coords.x1 + x - 20;
        lv_coord_t y_end = coords.y1 + y - 20;

        // X 动画
        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, g_drone);
        lv_anim_set_exec_cb(&ax, lv_obj_set_x);
        lv_anim_set_time(&ax, lv_anim_speed_to_time(speed, x_start, x_end));
        lv_anim_set_path_cb(&ax, lv_anim_path_ease_in_out);
        lv_anim_set_values(&ax, x_start, x_end);
        lv_anim_start(&ax);

        // Y 动画
        lv_anim_t ay;
        lv_anim_init(&ay);
        lv_anim_set_var(&ay, g_drone);
        lv_anim_set_exec_cb(&ay, lv_obj_set_y);
        lv_anim_set_time(&ay, lv_anim_speed_to_time(speed, y_start, y_end));
        lv_anim_set_path_cb(&ay, lv_anim_path_ease_in_out);
        lv_anim_set_values(&ay, y_start, y_end);
        lv_anim_set_ready_cb(&ay, (lv_anim_ready_cb_t)anim_cb);
        lv_anim_start(&ay);
    }
}

static lv_obj_t *drone_window_create() {
    drone_t *drone = drone_get_instance();

    if (drone->drone_state != DRONE_STATE_FREE) {
        return NULL; // 无人机空闲时才允许打开窗口
    }

    lv_obj_t *body = lv_obj_create(g_screen_main);
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *div = ui_window_create(g_screen_main, "DRONE OPERATION", body);
    lv_obj_center(div);
    lv_obj_set_size(div, 820, 470);

    lv_obj_t *left_panel = lv_obj_create(body);
    lv_obj_set_size(left_panel, 492, 366);
    lv_obj_set_pos(left_panel, 4, 8);
    lv_obj_set_style_bg_opa(left_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_panel, 0, 0);
    lv_obj_set_style_pad_all(left_panel, 0, 0);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *right_panel = lv_obj_create(body);
    lv_obj_set_size(right_panel, 304, 366);
    lv_obj_set_pos(right_panel, 502, 8);
    lv_obj_set_style_bg_opa(right_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_panel, 0, 0);
    lv_obj_set_style_pad_all(right_panel, 0, 0);
    lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *base_card = lv_obj_create(left_panel);
    lv_obj_set_size(base_card, 492, 96);
    lv_obj_set_pos(base_card, 0, 0);
    lv_obj_set_style_bg_color(base_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(base_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(base_card, 1, 0);
    lv_obj_set_style_radius(base_card, 10, 0);
    lv_obj_set_style_pad_all(base_card, 8, 0);
    lv_obj_clear_flag(base_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *base_title = lv_label_create(base_card);
    lv_label_set_text(base_title, "Base Info");
    lv_obj_set_style_text_color(base_title, lv_color_hex(0x5b421f), 0);
    lv_obj_set_pos(base_title, 0, 0);

    lv_obj_t *state_chip = lv_obj_create(base_card);
    lv_obj_set_size(state_chip, 150, 22);
    lv_obj_set_pos(state_chip, 322, 0);
    lv_obj_set_style_bg_color(state_chip, lv_color_hex(0xcdecd4), 0);
    lv_obj_set_style_bg_opa(state_chip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(state_chip, lv_color_hex(0x3c7a52), 0);
    lv_obj_set_style_border_width(state_chip, 1, 0);
    lv_obj_set_style_radius(state_chip, 16, 0);
    lv_obj_set_style_pad_all(state_chip, 0, 0);
    lv_obj_clear_flag(state_chip, LV_OBJ_FLAG_SCROLLABLE);
    g_drone_window_ctx.state_label = lv_label_create(state_chip);
    lv_obj_set_style_text_color(g_drone_window_ctx.state_label, lv_color_hex(0x175537), 0);
    lv_obj_center(g_drone_window_ctx.state_label);

    lv_obj_t *speed_key = lv_label_create(base_card);
    lv_label_set_text(speed_key, "Speed");
    lv_obj_set_style_text_color(speed_key, lv_color_hex(0x6f5c41), 0);
    lv_obj_set_pos(speed_key, 8, 30);
    g_drone_window_ctx.speed_label = lv_label_create(base_card);
    lv_label_set_text(g_drone_window_ctx.speed_label, "--");
    lv_obj_set_pos(g_drone_window_ctx.speed_label, 8, 50);

    lv_obj_t *algo_key = lv_label_create(base_card);
    lv_label_set_text(algo_key, "Algorithm");
    lv_obj_set_style_text_color(algo_key, lv_color_hex(0x6f5c41), 0);
    lv_obj_set_pos(algo_key, 172, 30);
    g_drone_window_ctx.algorithm_label = lv_label_create(base_card);
    lv_label_set_text(g_drone_window_ctx.algorithm_label, "--");
    lv_obj_set_pos(g_drone_window_ctx.algorithm_label, 172, 50);

    lv_obj_t *storage_key = lv_label_create(base_card);
    lv_label_set_text(storage_key, "Capacity");
    lv_obj_set_style_text_color(storage_key, lv_color_hex(0x6f5c41), 0);
    lv_obj_set_pos(storage_key, 336, 30);
    g_drone_window_ctx.storage_label = lv_label_create(base_card);
    lv_label_set_text(g_drone_window_ctx.storage_label, "--");
    lv_obj_set_pos(g_drone_window_ctx.storage_label, 336, 50);

    lv_obj_t *pest_card = lv_obj_create(left_panel);
    lv_obj_set_size(pest_card, 492, 116);
    lv_obj_set_pos(pest_card, 0, 100);
    lv_obj_set_style_bg_color(pest_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(pest_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(pest_card, 1, 0);
    lv_obj_set_style_radius(pest_card, 10, 0);
    lv_obj_set_style_pad_all(pest_card, 8, 0);
    lv_obj_clear_flag(pest_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pest_title = lv_label_create(pest_card);
    lv_label_set_text(pest_title, "Last Scan Pest Count");
    lv_obj_set_style_text_color(pest_title, lv_color_hex(0x5b421f), 0);
    lv_obj_set_pos(pest_title, 0, 0);

    for (crop_damage_t i = 0; i < CROP_DAMAGE_NONE; i++) {
        const void *pest_icon = icon_get_pest(i);
        lv_obj_t *icon = lv_img_create(pest_card);
        lv_img_set_src(icon, pest_icon ? pest_icon : &icon_pest_unknown);
        lv_obj_set_pos(icon, (i % 2) ? 248 : 8, 28 + (i / 2) * 32);

        lv_obj_t *name = lv_label_create(pest_card);
        lv_label_set_text(name, crop_pest_name(i));
        lv_obj_set_pos(name, (i % 2) ? 270 : 30, 28 + (i / 2) * 32);

        g_drone_window_ctx.result_labels[i] = lv_label_create(pest_card);
        lv_label_set_text(g_drone_window_ctx.result_labels[i], "0");
        lv_obj_set_pos(g_drone_window_ctx.result_labels[i], (i % 2) ? 448 : 208, 28 + (i / 2) * 32);
    }

    lv_obj_t *mode_card = lv_obj_create(left_panel);
    lv_obj_set_size(mode_card, 492, 146);
    lv_obj_set_pos(mode_card, 0, 220);
    lv_obj_set_style_bg_color(mode_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(mode_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(mode_card, 1, 0);
    lv_obj_set_style_radius(mode_card, 10, 0);
    lv_obj_set_style_pad_all(mode_card, 8, 0);
    lv_obj_clear_flag(mode_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Mode Control");
    lv_obj_set_style_text_color(mode_title, lv_color_hex(0x5b421f), 0);
    lv_obj_set_pos(mode_title, 0, 0);

    lv_obj_t *detect_name = lv_label_create(mode_card);
    lv_label_set_text(detect_name, "Detect / Recall");
    lv_obj_set_pos(detect_name, 8, 34);

    g_drone_window_ctx.detect_btn = lv_btn_create(mode_card);
    lv_obj_set_size(g_drone_window_ctx.detect_btn, 132, 34);
    lv_obj_set_pos(g_drone_window_ctx.detect_btn, 344, 28);
    lv_obj_set_style_bg_color(g_drone_window_ctx.detect_btn, lv_color_hex(0xefcd76), 0);
    lv_obj_set_style_border_color(g_drone_window_ctx.detect_btn, lv_color_hex(0x8a6333), 0);
    lv_obj_set_style_border_width(g_drone_window_ctx.detect_btn, 1, 0);
    lv_obj_set_style_radius(g_drone_window_ctx.detect_btn, 8, 0);
    lv_obj_t *detect_btn_label = lv_label_create(g_drone_window_ctx.detect_btn);
    lv_label_set_text(detect_btn_label, "Start Detect");
    lv_obj_center(detect_btn_label);
    lv_obj_add_event_cb(g_drone_window_ctx.detect_btn, drone_mode_btn_click_cb, LV_EVENT_CLICKED,
                        &g_drone_detect_btn_desc);

    lv_obj_t *spray_name = lv_label_create(mode_card);
    lv_label_set_text(spray_name, "Auto Spray");
    lv_obj_set_pos(spray_name, 8, 88);

    g_drone_window_ctx.spray_btn = lv_btn_create(mode_card);
    lv_obj_set_size(g_drone_window_ctx.spray_btn, 132, 34);
    lv_obj_set_pos(g_drone_window_ctx.spray_btn, 344, 82);
    lv_obj_set_style_bg_color(g_drone_window_ctx.spray_btn, lv_color_hex(0xefcd76), 0);
    lv_obj_set_style_border_color(g_drone_window_ctx.spray_btn, lv_color_hex(0x8a6333), 0);
    lv_obj_set_style_border_width(g_drone_window_ctx.spray_btn, 1, 0);
    lv_obj_set_style_radius(g_drone_window_ctx.spray_btn, 8, 0);
    lv_obj_t *spray_btn_label = lv_label_create(g_drone_window_ctx.spray_btn);
    lv_label_set_text(spray_btn_label, "Start Spray");
    lv_obj_center(spray_btn_label);
    lv_obj_add_event_cb(g_drone_window_ctx.spray_btn, drone_mode_btn_click_cb, LV_EVENT_CLICKED,
                        &g_drone_spray_btn_desc);

    lv_obj_t *bag_card = lv_obj_create(right_panel);
    lv_obj_set_size(bag_card, 304, 366);
    lv_obj_set_pos(bag_card, 0, 0);
    lv_obj_set_style_bg_color(bag_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(bag_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(bag_card, 1, 0);
    lv_obj_set_style_radius(bag_card, 10, 0);
    lv_obj_set_style_pad_all(bag_card, 8, 0);
    lv_obj_clear_flag(bag_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bag_title = lv_label_create(bag_card);
    lv_label_set_text(bag_title, "Pesticide Bag (+/-)");
    lv_obj_set_style_text_color(bag_title, lv_color_hex(0x5b421f), 0);
    lv_obj_set_pos(bag_title, 0, 0);

    for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
        lv_obj_t *name = lv_label_create(bag_card);
        lv_label_set_text(name, crop_pesticide_name(i));
        lv_obj_set_pos(name, 8, 34 + i * 44);

        g_drone_window_ctx.pesticide_num_labels[i] = lv_label_create(bag_card);
        lv_label_set_text(g_drone_window_ctx.pesticide_num_labels[i], "0");
        lv_obj_set_pos(g_drone_window_ctx.pesticide_num_labels[i], 156, 34 + i * 44);

        lv_obj_t *minus_btn = lv_btn_create(bag_card);
        lv_obj_set_size(minus_btn, 28, 28);
        lv_obj_set_pos(minus_btn, 224, 28 + i * 44);
        lv_obj_set_style_bg_color(minus_btn, lv_color_hex(0xefcd76), 0);
        lv_obj_set_style_border_color(minus_btn, lv_color_hex(0x8a6333), 0);
        lv_obj_set_style_border_width(minus_btn, 1, 0);
        lv_obj_set_style_radius(minus_btn, 8, 0);
        lv_obj_t *minus_label = lv_label_create(minus_btn);
        lv_label_set_text(minus_label, "-");
        lv_obj_center(minus_label);

        lv_obj_t *add_btn = lv_btn_create(bag_card);
        lv_obj_set_size(add_btn, 28, 28);
        lv_obj_set_pos(add_btn, 262, 28 + i * 44);
        lv_obj_set_style_bg_color(add_btn, lv_color_hex(0xf4cdca), 0);
        lv_obj_set_style_border_color(add_btn, lv_color_hex(0xb66258), 0);
        lv_obj_set_style_border_width(add_btn, 1, 0);
        lv_obj_set_style_radius(add_btn, 8, 0);
        lv_obj_t *add_label = lv_label_create(add_btn);
        lv_label_set_text(add_label, "+");
        lv_obj_center(add_label);

        g_drone_pesticide_btn_desc[i][0] = (drone_pesticide_btn_desc_t){.pesticide = i, .delta = 1};
        g_drone_pesticide_btn_desc[i][1] = (drone_pesticide_btn_desc_t){.pesticide = i, .delta = -1};
        lv_obj_add_event_cb(add_btn, drone_pesticide_btn_click_cb, LV_EVENT_CLICKED, &g_drone_pesticide_btn_desc[i][0]);
        lv_obj_add_event_cb(minus_btn, drone_pesticide_btn_click_cb, LV_EVENT_CLICKED,
                            &g_drone_pesticide_btn_desc[i][1]);
    }

    g_drone_window_ctx.obj = div;
    ui_drone_window_refresh();

    return div;
}

static void drone_mode_btn_click_cb(lv_event_t *e) {
    drone_mode_btn_desc_t *desc = lv_event_get_user_data(e);
    if (!desc) {
        return;
    }

    drone_t *drone = drone_get_instance();
    if (!drone) {
        return;
    }

    if (drone->drone_state == desc->target_state) {
        drone_state_switch(DRONE_STATE_FREE);
    } else {
        drone_state_switch(desc->target_state);
    }

    ui_drone_window_refresh();
}

static void drone_pesticide_btn_click_cb(lv_event_t *e) {
    drone_pesticide_btn_desc_t *desc = lv_event_get_user_data(e);
    drone_t *drone = drone_get_instance();
    if (!desc || !drone) {
        return;
    }

    if (drone->drone_state != DRONE_STATE_FREE) {
        return;
    }

    int used = ui_drone_pesticide_used(drone);
    int cur = drone->pesticide_storage[desc->pesticide];

    if (desc->delta > 0) {
        if (used >= drone->storage_capacity) {
            return;
        }
        drone->pesticide_storage[desc->pesticide] = cur + 1;
    } else {
        if (cur <= 0) {
            return;
        }
        drone->pesticide_storage[desc->pesticide] = cur - 1;
    }

    ui_drone_window_refresh();
}

static int ui_drone_pesticide_used(const drone_t *drone) {
    int used = 0;
    for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
        used += drone->pesticide_storage[i];
    }
    return used;
}

static void ui_drone_btn_set_text(lv_obj_t *btn, const char *text) {
    if (!btn || !lv_obj_is_valid(btn)) {
        return;
    }

    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (!label) {
        return;
    }
    lv_label_set_text(label, text);
}

static void ui_drone_window_refresh(void) {
    if (!g_drone_window_ctx.obj || !lv_obj_is_valid(g_drone_window_ctx.obj)) {
        return;
    }

    drone_t *drone = drone_get_instance();
    if (!drone) {
        return;
    }

    int used = ui_drone_pesticide_used(drone);

    if (g_drone_window_ctx.state_label) {
        lv_label_set_text_fmt(g_drone_window_ctx.state_label, "State: %s", drone_state_name(drone->drone_state));
    }
    if (g_drone_window_ctx.speed_label) {
        lv_label_set_text_fmt(g_drone_window_ctx.speed_label, "%d m/s", drone->speed);
    }
    if (g_drone_window_ctx.algorithm_label) {
        lv_label_set_text_fmt(g_drone_window_ctx.algorithm_label, "Lv.%d", drone->algorithm_level + 1);
    }
    if (g_drone_window_ctx.storage_label) {
        lv_label_set_text_fmt(g_drone_window_ctx.storage_label, "%d / %d", used, drone->storage_capacity);
    }

    for (crop_damage_t i = 0; i < CROP_DAMAGE_NONE; i++) {
        if (g_drone_window_ctx.result_labels[i]) {
            lv_label_set_text_fmt(g_drone_window_ctx.result_labels[i], "%d", pest_count[i]);
        }
    }

    for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
        if (g_drone_window_ctx.pesticide_num_labels[i]) {
            lv_label_set_text_fmt(g_drone_window_ctx.pesticide_num_labels[i], "%d", drone->pesticide_storage[i]);
        }
    }

    bool detecting = drone->drone_state == DRONE_STATE_DETECTING;
    bool spraying = drone->drone_state == DRONE_STATE_AUTO;

    if (g_drone_window_ctx.detect_btn) {
        ui_drone_btn_set_text(g_drone_window_ctx.detect_btn, detecting ? "Recall" : "Start Detect");
        lv_obj_set_style_bg_color(g_drone_window_ctx.detect_btn,
                                  detecting ? lv_color_hex(0x2f9a5f) : lv_color_hex(0xefcd76), 0);
        if (spraying) {
            lv_obj_add_state(g_drone_window_ctx.detect_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(g_drone_window_ctx.detect_btn, LV_STATE_DISABLED);
        }
    }

    if (g_drone_window_ctx.spray_btn) {
        ui_drone_btn_set_text(g_drone_window_ctx.spray_btn, spraying ? "Stop Spray" : "Start Spray");
        lv_obj_set_style_bg_color(g_drone_window_ctx.spray_btn,
                                  spraying ? lv_color_hex(0x2f9a5f) : lv_color_hex(0xefcd76), 0);
        if (detecting) {
            lv_obj_add_state(g_drone_window_ctx.spray_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(g_drone_window_ctx.spray_btn, LV_STATE_DISABLED);
        }
    }
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

static void ui_drone_btns_create(lv_obj_t *parent) {
    /* 相关按钮 */
    shop_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_shop_btn, 40, 380);
    storage_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_storage_btn, 40, 460);
    plant_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_plant_btn, 920, 450);
    setting_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_setting_btn, 920, 40);

    /* 按钮回调 */
    lv_obj_add_event_cb(plant_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_plant_window_toggle);
    lv_obj_add_event_cb(storage_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_storage_window_toggle);
    lv_obj_add_event_cb(shop_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_shop_window_toggle);
    lv_obj_add_event_cb(setting_btn, main_floating_btn_click_cb, LV_EVENT_CLICKED, &g_setting_window_toggle);
}

static lv_obj_t *ui_crop_grwoing_bar(lv_obj_t *parent) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_add_event_cb(bar, crop_growing_bar_event, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_set_size(bar, 75, 10);
    lv_obj_set_align(bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(bar, 0, -2);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_50, LV_PART_MAIN);
    return bar;
}

static void ui_gold_bar_create(lv_obj_t *parent) {
    lv_obj_t *bar = ui_div_create(parent);
    lv_obj_set_size(bar, 120, 40);
    lv_obj_set_pos(bar, 200, 8);
    lv_obj_set_style_bg_img_src(bar, &icon_gold_bar_bg, 0);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_FLOATING);
}

static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x,
                                    lv_coord_t y) {
    lv_obj_t *btn = ui_div_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_img_src(btn, img, 0);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING);

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
        if (!obj) {
            break;
        }

        const void *drag_img = ui_crop_drag_img(i);

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

        seeds[i] = (ui_drag_to_plant_desc_t){.type = i, .img = drag_img, .fields = farm_grid};
        ui_grid_list_bind_item_event(obj, drag_to_plant_cb, LV_EVENT_PRESSING, &seeds[i]);
        ui_grid_list_bind_item_event(obj, drag_to_plant_cb, LV_EVENT_RELEASED, &seeds[i]);
    }

    return grid;
}

// 以下函数是ai修改代码时额外生成的，后面更改实现方式
static const void *ui_crop_drag_img(crop_type_t type) {
    const void *img = icon_get_crop(type, CROP_STAGE_READY);
    if (img) {
        return img;
    }

    img = icon_get_crop(type, CROP_STAGE_SEED);
    if (img) {
        return img;
    }

    switch (type) {
        case CROP_TYPE_CORN:
            return &icon_crop_corn_ripe;
        case CROP_TYPE_WHEAT:
            return &icon_crop_wheat_ripe;
        case CROP_TYPE_RICE:
            return &icon_crop_wheat_ripe;
        default:
            return &icon_crop_wheat_ripe;
    }
}

static lv_obj_t *ui_plant_window_create() {
    lv_obj_t *grid = ui_seed_table_create(g_screen_main);

    lv_obj_t *div = ui_window_create(g_screen_main, "PLANT", grid);
    lv_obj_set_align(div, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(div, -20, -20);
    lv_obj_set_size(div, 206, 290);

    g_plant_window = div;

    return div;
}

static lv_obj_t *ui_storage_window_create() {
    // ui_grid_list_t *grid = ui_grid_list_create(g_screen_main, NULL);
    // if (!grid) {
    //     return NULL;
    // }

    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_t *div = ui_window_create(g_screen_main, "STORAGE", body);

    lv_obj_center(div);
    lv_obj_set_size(div, 700, 400);

    g_storage_window = div;

    return div;
}

static lv_obj_t *ui_shop_window_create() {
    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_t *div = ui_window_create(g_screen_main, "SHOP", body);

    lv_obj_center(div);
    lv_obj_set_size(div, 250, 300);

    g_shop_window = div;

    return div;
}

static lv_obj_t *ui_setting_window_create() {
    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_t *div = ui_window_create(g_screen_main, "SETTING", body);

    lv_obj_center(div);
    lv_obj_set_size(div, 300, 400);

    g_setting_window = div;

    return div;
}

void ui_update_timer_init() {
    ui_timer_drone_update_100ms = lv_timer_create(ui_drone_update_100ms, 100, NULL);
    ui_timer_update_1s = lv_timer_create(ui_update_1s, 1000, NULL);
    lv_timer_pause(ui_timer_drone_update_100ms);
}

static void ui_drone_update_100ms() {
    /* UPDATE drone */
    drone_t *drone = drone_get_instance();
    if (drone->drone_state == DRONE_STATE_DETECTING) {
        /* 刷新位置 */
        pos_t vector = {.x = joystick_get_dir_x(), .y = joystick_get_dir_y()};
        drone_move(vector);
        pos_t pos = drone->current_pos;
        ui_drone_set_pos(pos.x * DRONE_COORD_SCALNG_FACTOR, pos.y * DRONE_COORD_SCALNG_FACTOR, false, NULL);

        /* 执行探测任务 */
        if (!g_farm_blocks[pos.x / 100][pos.y / 100].is_detected) {
            crop_damage_t pest = drone_detect_damage();
            if (pest != CROP_DAMAGE_NONE) {
                pest_count[pest]++;
            }
        }
    }
}

static void ui_update_1s() {
    /* UPDATE farm */
    for (int i = 0; i < FARM_GRID_N; i++) {
        for (int j = 0; j < FARM_GRID_N; j++) {
            farm_block_t *block = &g_farm_blocks[i][j];
            if (block->is_planted) {
                /* UPDATE growing bar */
                lv_bar_set_range(block->growing_bar, 0, block->field->ready_time);
                lv_bar_set_value(block->growing_bar, block->field->growing_time, LV_ANIM_OFF);
            }
        }
    }

    ui_drone_window_refresh();
}

static void ui_drone_timer_resume() {
    lv_timer_resume(ui_timer_drone_update_100ms);
}
