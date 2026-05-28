#include "ui.h"

#include "audio.h"
#include "data.h"
#include "drone.h"
#include "icon.h"
#include "joystick.h"
#include "player.h"
#include "ui_common.h"
#include "ui_drone.h"
#include "ui_farm.h"
#include "ui_grid_list.h"
#include "ui_main_cb.h"
#include "ui_message.h"
#include "ui_shop.h"
#include "ui_storage.h"
#include "ui_storage_cb.h"
#include "ui_window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FARM_GRID_N farm_get_instance()->current_size
#define FARM_BLOCK_SIZE 80

lv_timer_t *ui_timer_update_100ms;
lv_timer_t *ui_timer_update_1s;

static lv_obj_t *g_screen_main = NULL;
static lv_obj_t *g_main_layer = NULL;

static lv_obj_t *shop_btn = NULL;
static lv_obj_t *storage_btn = NULL;
static lv_obj_t *plant_btn = NULL;
static lv_obj_t *setting_btn = NULL;
static lv_obj_t *g_seed_items[CROP_TYPE_NONE];
static lv_obj_t *g_seed_count_labels[CROP_TYPE_NONE];

static lv_obj_t *g_plant_window = NULL;
static lv_obj_t *g_shop_window = NULL;
static lv_obj_t *g_storage_window = NULL;
static lv_obj_t *g_setting_window = NULL;

static lv_obj_t *g_gold_bar_label = NULL;

static lv_obj_t *g_prop_scarecrow = NULL;

typedef struct {
    lv_obj_t *level_label;
    lv_obj_t *exp_label;
    lv_obj_t *exp_bar;
} ui_player_exp_ctx_t;

ui_player_exp_ctx_t g_player_exp_ctx;

static lv_obj_t *ui_seed_table_create(lv_obj_t *parent);
static lv_obj_t *ui_plant_window_create(void);
static lv_obj_t *ui_setting_window_create(void);
static void ui_gold_bar_create(lv_obj_t *parent);
static void ui_gold_bar_refresh(void);
static void ui_exp_bar_create(lv_obj_t *parent);
static void ui_exp_bar_refresh(void);
static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x,
                                    lv_coord_t y);
static void ui_update_100ms(lv_timer_t *timer);
static void ui_update_1s(lv_timer_t *timer);
static void ui_main_icon_btns_hide(bool hide);
static void ui_seed_table_refresh(void);
static void ui_decorations_create(void);

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

lv_obj_t *ui_main_screen_create(void) {
    if (g_screen_main && lv_obj_is_valid(g_screen_main)) {
        return g_screen_main;
    }

    /* 这个地方的布局优化了1mol次，走了十年弯路，特此记录，原来大道至简...我悟了...吗？... */

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

    // 农田模块
    ui_farm_module_create(g_screen_main, g_main_layer);

    // 金币条和经验条
    ui_gold_bar_create(g_screen_main);
    ui_exp_bar_create(g_screen_main);

    // 无人机模块
    ui_drone_module_create(g_main_layer, g_screen_main);

    // 悬浮按钮
    shop_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_shop_btn, 40, 380);
    storage_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_storage_btn, 40, 460);
    plant_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_plant_btn, 920, 450);
    setting_btn = ui_icon_btn_create(g_screen_main, 47, 47, &icon_setting_btn, 920, 40);

    lv_obj_add_event_cb(plant_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_plant_window_toggle);
    lv_obj_add_event_cb(storage_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_storage_window_toggle);
    lv_obj_add_event_cb(shop_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_shop_window_toggle);
    lv_obj_add_event_cb(setting_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_setting_window_toggle);

    // 装饰物
    ui_decorations_create();

    // g_storage_window_toggle.window_ref = &g_storage_window;

    // lv_obj_add_event_cb(g_screen_main, ui_main_screen_click_cb, LV_EVENT_CLICKED, NULL);

    return g_screen_main;
}

void ui_main_handle_event(event_t *event) {
    if (!event) {
        return;
    }

    switch (event->type) {
        case EVENT_ON_DRONE_TO_FREE:
            ui_main_icon_btns_hide(false);
            break;
        case EVENT_ON_DRONE_TO_MOVING:
            ui_main_icon_btns_hide(true);
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
        default:
            break;
    }
}

// 主界面更新定时器注册
void ui_main_update_timer_init(void) {
    ui_timer_update_100ms = lv_timer_create(ui_update_100ms, 100, NULL);
    ui_timer_update_1s = lv_timer_create(ui_update_1s, 1000, NULL);
}

// 启动更新定时器
void ui_main_update_timer_start(void) {
    lv_timer_resume(ui_timer_update_100ms);
    lv_timer_resume(ui_timer_update_1s);
}

// 暂停更新定时器
void ui_main_update_timer_pause(void) {
    lv_timer_pause(ui_timer_update_100ms);
    lv_timer_pause(ui_timer_update_1s);
}

// 根据无人机状态显示/隐藏主界面上的悬浮按钮
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

// 金币条创建
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

// 金币条刷新
static void ui_gold_bar_refresh(void) {
    if (g_gold_bar_label && lv_obj_is_valid(g_gold_bar_label)) {
        lv_label_set_text_fmt(g_gold_bar_label, "%d", player_get_instance()->coins);
    }
}

// 经验条创建
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

// 经验条刷新
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

// 图标按钮创建
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

// 种子表创建
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

        seeds[i] = (ui_drag_to_plant_desc_t){.type = i, .img = drag_img, .fields = ui_farm_get_grid()};
        ui_grid_list_bind_item_event(obj, ui_main_seed_drag_event_cb, LV_EVENT_PRESSING, &seeds[i]);
        ui_grid_list_bind_item_event(obj, ui_main_seed_drag_event_cb, LV_EVENT_RELEASED, &seeds[i]);
    }

    ui_seed_table_refresh();

    return grid;
}

// 种子表刷新
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

// 种植窗口创建
static lv_obj_t *ui_plant_window_create(void) {
    lv_obj_t *grid = ui_seed_table_create(g_screen_main);

    lv_obj_t *div = ui_window_create("PLANT", grid, false);
    lv_obj_set_align(div, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(div, -20, -20);
    lv_obj_set_size(div, 206, 290);

    g_plant_window = div;

    return div;
}

// 设置窗口创建
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
static void ui_update_100ms(lv_timer_t *timer) {
    (void)timer;

    ui_drone_update_100ms();

    /* 装饰物 */
    if (g_prop_scarecrow) {
        float angle = sinf(lv_tick_get() * 0.002f) * 30;
        lv_img_set_angle(g_prop_scarecrow, angle);
    }
}

// 1s定时器回调（主要用于田地状态刷新）
static void ui_update_1s(lv_timer_t *timer) {
    (void)timer;

    ui_farm_refresh_all();
}
