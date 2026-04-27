#include "ui.h"

#include "drone.h"
#include "enum.h"
#include "icon.h"
#include "joystick.h"
#include "player.h"
#include "ui_common.h"
#include "ui_drone_cb.h"
#include "ui_grid_list.h"
#include "ui_main_cb.h"
#include "ui_window.h"

#define FARM_GRID_N farm_get_instance()->current_size
#define FARM_BLOCK_SIZE 80
#define DRONE_COORD_SCALNG_FACTOR (FARM_BLOCK_SIZE / 100.0)

static lv_obj_t *g_screen_main = NULL;

static lv_obj_t *farm_grid = NULL;
static lv_obj_t *g_drone = NULL;

static lv_obj_t *shop_btn = NULL;
static lv_obj_t *storage_btn = NULL;
static lv_obj_t *plant_btn = NULL;
static lv_obj_t *setting_btn = NULL;
static lv_obj_t *g_seed_items[CROP_TYPE_NONE];
static lv_obj_t *g_seed_count_labels[CROP_TYPE_NONE];

static farm_block_t g_farm_blocks[10][10];
uint8_t ui_drone_pest_count[CROP_DAMAGE_NONE];

static lv_timer_t *ui_timer_drone_update_100ms = NULL;
static lv_timer_t *ui_timer_update_1s = NULL;

static lv_obj_t *g_plant_window = NULL;
static lv_obj_t *g_storage_window = NULL;
static lv_obj_t *g_shop_window = NULL;
static lv_obj_t *g_setting_window = NULL;
static lv_obj_t *g_drone_window = NULL;

static lv_obj_t *ui_seed_table_create(lv_obj_t *parent);
static lv_obj_t *ui_plant_window_create(void);
static lv_obj_t *ui_storage_window_create(void);
static lv_obj_t *ui_setting_window_create(void);
static lv_obj_t *ui_drone_create(lv_obj_t *parent);
static void ui_farm_grid_create(lv_obj_t *parent);
static void ui_field_update(int x, int y);
static lv_obj_t *ui_crop_grwoing_bar(lv_obj_t *parent);
static void ui_gold_bar_create(lv_obj_t *parent);
static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x,
                                    lv_coord_t y);
static const void *ui_crop_drag_img(crop_type_t type);
static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y, bool anim, void *anim_cb);
static void ui_drone_update_100ms(lv_timer_t *timer);
static void ui_update_1s(lv_timer_t *timer);
static void ui_drone_timer_resume(void);
static void ui_main_icon_btns_hide(bool hide);
static void ui_seed_table_refresh(void);

lv_obj_t *ui_shop_window_create(void);
lv_obj_t *ui_drone_window_create(void);

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
static ui_window_toggle_desc_t g_drone_window_toggle = {.create = ui_drone_window_create,
                                                        .window_ref = &g_drone_window};

lv_obj_t *ui_main_screen_create(void) {
    if (g_screen_main && lv_obj_is_valid(g_screen_main)) {
        return g_screen_main;
    }

    g_screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_img_src(g_screen_main, &icon_farm_bg, 0);
    lv_obj_set_style_bg_img_tiled(g_screen_main, true, 0);

    ui_farm_grid_create(g_screen_main);
    lv_obj_add_event_cb(g_screen_main, ui_main_screen_click_cb, LV_EVENT_CLICKED, farm_grid);

    ui_gold_bar_create(g_screen_main);

    g_drone = ui_drone_create(g_screen_main);
    ui_drone_hud_create(g_screen_main);
    ui_drone_set_pos(-40, 40, false, NULL);

    shop_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_shop_btn, 40, 380);
    storage_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_storage_btn, 40, 460);
    plant_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_plant_btn, 920, 450);
    setting_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_setting_btn, 920, 40);

    lv_obj_add_event_cb(plant_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_plant_window_toggle);
    lv_obj_add_event_cb(storage_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_storage_window_toggle);
    lv_obj_add_event_cb(shop_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_shop_window_toggle);
    lv_obj_add_event_cb(setting_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_setting_window_toggle);

    lv_obj_add_event_cb(g_screen_main, ui_main_screen_click_cb, LV_EVENT_CLICKED, NULL);

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
            break;
        }
        case EVENT_ON_PLAYER_COIN_CHANGE:
        case EVENT_ON_PLAYER_SEED_CHANGE:
        case EVENT_ON_PLAYER_EXPERIENCE_CHANGE:
        case EVENT_ON_PLAYER_LEVEL_UPGRADE:
            if (event->type == EVENT_ON_PLAYER_SEED_CHANGE) {
                ui_seed_table_refresh();
            }
            if (event->type == EVENT_ON_PLAYER_COIN_CHANGE || event->type == EVENT_ON_PLAYER_LEVEL_UPGRADE ||
                event->type == EVENT_ON_PLAYER_SEED_CHANGE) {
                ui_shop_refresh();
            }
            break;
        case EVENT_ON_DRONE_TO_FREE:
            if (ui_timer_drone_update_100ms) {
                lv_timer_pause(ui_timer_drone_update_100ms);
            }
            ui_drone_set_pos(-40, 40, true, NULL);
            ui_main_icon_btns_hide(false);
            ui_drone_hud_set_visible(false);
            ui_drone_window_refresh();
            break;
        case EVENT_ON_DRONE_TO_MOVING:
            ui_drone_set_pos(0, 0, true, ui_drone_timer_resume);
            ui_main_icon_btns_hide(true);
            if (g_drone_window && lv_obj_is_valid(g_drone_window) && ui_window_is_visible(g_drone_window)) {
                ui_window_hide(g_drone_window);
            }
            ui_drone_hud_set_visible(true);
            ui_drone_window_refresh();
            break;
        default:
            break;
    }
}

void ui_main_update_timer_init(void) {
    ui_timer_drone_update_100ms = lv_timer_create(ui_drone_update_100ms, 100, NULL);
    ui_timer_update_1s = lv_timer_create(ui_update_1s, 1000, NULL);
    lv_timer_pause(ui_timer_drone_update_100ms);
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

            lv_obj_t *bg_layer = ui_div_create(farm_grid);
            lv_obj_set_size(bg_layer, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_set_pos(bg_layer, FARM_BLOCK_SIZE * i, FARM_BLOCK_SIZE * j);
            lv_obj_set_style_bg_img_src(bg_layer, &icon_field_bg, 0);
            lv_obj_add_flag(bg_layer, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_add_flag(bg_layer, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_pad_all(bg_layer, -2, 0);
            lv_obj_set_style_border_width(bg_layer, 2, LV_STATE_CHECKED);
            lv_obj_add_event_cb(bg_layer, ui_main_field_block_click_cb, LV_EVENT_CLICKED, g_screen_main);
            lv_obj_set_user_data(bg_layer, block);

            block->obj = ui_div_create(bg_layer);
            lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
            lv_obj_add_flag(block->obj, LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_center(block->obj);

            block->growing_bar = ui_crop_grwoing_bar(block->obj);
            lv_bar_set_range(block->growing_bar, 0, field->ready_time);
            lv_bar_set_value(block->growing_bar, field->growing_time, LV_ANIM_OFF);

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
    lv_obj_add_event_cb(drone, ui_main_drone_click_cb, LV_EVENT_CLICKED, &g_drone_window_toggle);
    lv_animimg_start(drone);

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

        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, g_drone);
        lv_anim_set_exec_cb(&ax, lv_obj_set_x);
        lv_anim_set_time(&ax, lv_anim_speed_to_time(speed, x_start, x_end));
        lv_anim_set_path_cb(&ax, lv_anim_path_ease_in_out);
        lv_anim_set_values(&ax, x_start, x_end);
        lv_anim_start(&ax);

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

static lv_obj_t *ui_crop_grwoing_bar(lv_obj_t *parent) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_add_event_cb(bar, ui_main_crop_growing_bar_draw_part_end_cb, LV_EVENT_DRAW_PART_END, NULL);
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
        g_seed_items[i] = obj;

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

static lv_obj_t *ui_plant_window_create(void) {
    lv_obj_t *grid = ui_seed_table_create(g_screen_main);

    lv_obj_t *div = ui_window_create(g_screen_main, "PLANT", grid, false);
    lv_obj_set_align(div, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(div, -20, -20);
    lv_obj_set_size(div, 206, 290);

    g_plant_window = div;

    return div;
}

static lv_obj_t *ui_storage_window_create(void) {
    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_t *div = ui_window_create(g_screen_main, "STORAGE", body, true);

    lv_obj_center(div);
    lv_obj_set_size(div, 700, 400);

    g_storage_window = div;

    return div;
}

static lv_obj_t *ui_setting_window_create(void) {
    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_t *div = ui_window_create(g_screen_main, "SETTING", body, true);

    lv_obj_center(div);
    lv_obj_set_size(div, 300, 400);

    g_setting_window = div;

    return div;
}

static void ui_drone_update_100ms(lv_timer_t *timer) {
    (void)timer;

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
    }
}

static void ui_update_1s(lv_timer_t *timer) {
    (void)timer;

    for (int i = 0; i < FARM_GRID_N; i++) {
        for (int j = 0; j < FARM_GRID_N; j++) {
            farm_block_t *block = &g_farm_blocks[i][j];
            if (block->is_planted) {
                lv_bar_set_range(block->growing_bar, 0, block->field->ready_time);
                lv_bar_set_value(block->growing_bar, block->field->growing_time, LV_ANIM_OFF);
            }
        }
    }

    ui_seed_table_refresh();
    ui_drone_window_refresh();
}

static void ui_drone_timer_resume(void) {
    if (ui_timer_drone_update_100ms) {
        lv_timer_resume(ui_timer_drone_update_100ms);
    }
}
