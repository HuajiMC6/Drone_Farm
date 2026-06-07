#include "ui_main.h"

#include "audio.h"
#include "data.h"
#include "drone.h"
#include "icon.h"
#include "joystick.h"
#include "player.h"
#include "ui.h"
#include "ui_common.h"
#include "ui_drone.h"
#include "ui_farm.h"
#include "ui_farm_cb.h"
#include "ui_grid_list.h"
#include "ui_main_cb.h"
#include "ui_message.h"
#include "ui_setting.h"
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
static lv_obj_t *harvest_btn = NULL;
static lv_obj_t *farm_upgrade_btn = NULL;

static lv_obj_t *g_seed_items[CROP_TYPE_NONE];
static lv_obj_t *g_seed_count_labels[CROP_TYPE_NONE];

static lv_obj_t *g_plant_window = NULL;
static lv_obj_t *g_shop_window = NULL;
static lv_obj_t *g_storage_window = NULL;
static lv_obj_t *g_setting_window = NULL;
static lv_obj_t *g_farm_upgrade_window = NULL;

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
static lv_obj_t *ui_farm_upgrade_window_create(void);
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
static ui_window_toggle_desc_t g_farm_upgrade_window_toggle = {
    .create = ui_farm_upgrade_window_create,
    .window_ref = &g_farm_upgrade_window,
};

lv_obj_t *ui_main_screen_create(void) {
    if (g_screen_main && lv_obj_is_valid(g_screen_main)) {
        return g_screen_main;
    }

    /* 这个地方的布局优化了1mol次，走了十年弯路，特此记录🤳，原来大道至简😭...... */

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
    shop_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_btn_shop, 40, 380);
    storage_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_btn_storage, 40, 460);
    farm_upgrade_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_btn_farm_upgrade, 40, 170);
    plant_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_btn_plant, 920, 450);
    harvest_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_btn_harvest, 920, 370);
    setting_btn = ui_icon_btn_create(g_screen_main, 56, 56, &icon_btn_setting, 920, 40);

    lv_obj_add_event_cb(plant_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_plant_window_toggle);
    lv_obj_add_event_cb(storage_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_storage_window_toggle);
    lv_obj_add_event_cb(shop_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_shop_window_toggle);
    lv_obj_add_event_cb(setting_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED, &g_setting_window_toggle);
    lv_obj_add_event_cb(harvest_btn, ui_farm_harvest_all_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(farm_upgrade_btn, ui_main_floating_button_click_cb, LV_EVENT_CLICKED,
                        &g_farm_upgrade_window_toggle);

    // 装饰物
    ui_decorations_create();

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
        lv_obj_add_flag(harvest_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(farm_upgrade_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(shop_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(storage_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(plant_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(setting_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(harvest_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(farm_upgrade_btn, LV_OBJ_FLAG_HIDDEN);
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
    lv_label_set_text_fmt(g_gold_bar_label, "%d", player_get_instance()->coins);
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

    return btn;
}

// 种子表创建
static lv_obj_t *ui_seed_table_create(lv_obj_t *parent) {
    ui_grid_list_cfg_t cfg = ui_grid_list_cfg_make(60, 60, 3, 3);

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

        lv_obj_t *item_img = lv_img_create(obj);
        lv_img_set_src(item_img, icon_get_crop(i, CROP_STAGE_SEED));
        lv_obj_align(item_img, LV_ALIGN_TOP_MID, 0, -10);

        lv_obj_t *item1_label = lv_label_create(obj);
        lv_label_set_text(item1_label, crop_type_name(i));
        lv_obj_set_style_text_color(item1_label, lv_color_make(60, 42, 29), 0);
        lv_obj_set_style_text_align(item1_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(item1_label, LV_ALIGN_BOTTOM_MID, 0, 0);

        g_seed_count_labels[i] = lv_label_create(obj);
        lv_label_set_text(g_seed_count_labels[i], "x0");
        lv_obj_set_style_text_color(g_seed_count_labels[i], lv_color_make(60, 42, 29), 0);
        lv_obj_align(g_seed_count_labels[i], LV_ALIGN_TOP_RIGHT, -2, 2);

        seeds[i] = (ui_drag_to_plant_desc_t){.type = i, .img = icon_get_crop_item(i), .fields = ui_farm_get_grid()};
        ui_grid_list_bind_item_event(obj, ui_farm_seed_drag_event_cb, LV_EVENT_PRESSING, &seeds[i]);
        ui_grid_list_bind_item_event(obj, ui_farm_seed_drag_event_cb, LV_EVENT_RELEASED, &seeds[i]);
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

    return div;
}

// 装饰物生成
static void ui_decorations_create(void) {
    // 稻草人
    g_prop_scarecrow = lv_img_create(g_main_layer);
    lv_img_set_src(g_prop_scarecrow, img_prop_scarecrow);
    lv_obj_set_pos(g_prop_scarecrow, 120, 120);
}

// 田地升级弹窗创建
static lv_obj_t *ui_farm_upgrade_window_create(void) {
    lv_obj_t *body = ui_div_create(g_screen_main);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(body, 12, 0);
    lv_obj_set_style_pad_row(body, 6, 0);

    farm_t *farm = farm_get_instance();
    lv_obj_t *current_size = lv_label_create(body);
    lv_label_set_text_fmt(current_size, "Current Size: %dx%d", farm->current_size, farm->current_size);

    lv_obj_t *next_size = lv_label_create(body);
    lv_obj_t *spacer = ui_transparent_cont_create(body, LV_PCT(100), 10);
    if (farm->size_level < FARM_SIZE_LEVEL_MAX) {
        lv_label_set_text_fmt(next_size, "Next Size: %dx%d", farm_size_by_level[farm->size_level + 1],
                              farm_size_by_level[farm->size_level + 1]);

        lv_obj_t *price_label = lv_label_create(body);
        int price = farm_size_update_price[farm->size_level];
        double discount = level_discount[player_get_instance()->level_stage];
        int discount_price = (int)(price * discount);
        char buf[48];
        if (discount_price < price) {
            snprintf(buf, sizeof(buf), "Upgrade Price: %d (x%.2f)", discount_price, discount);
        } else {
            snprintf(buf, sizeof(buf), "Upgrade Price: %d", price);
        }
        lv_label_set_text(price_label, buf);
        lv_obj_set_style_text_color(price_label, lv_color_hex(0xb66258), 0);

        lv_obj_t *upgrade_btn = lv_btn_create(body);
        lv_obj_set_size(upgrade_btn, 120, 40);
        lv_obj_add_style(upgrade_btn, &ui_style_btn_yellow, 0);
        lv_obj_t *upgrade_btn_label = lv_label_create(upgrade_btn);
        lv_label_set_text(upgrade_btn_label, "Upgrade");
        lv_obj_center(upgrade_btn_label);
        lv_obj_add_event_cb(upgrade_btn, ui_main_farm_upgrade_btn_click_cb, LV_EVENT_CLICKED, NULL);
    } else {
        lv_label_set_text(next_size, "Max Size Reached");
        lv_obj_set_style_text_color(next_size, lv_color_hex(0xb66258), 0);
        lv_obj_set_style_text_font(next_size, &lv_font_montserrat_20, 0);
    }

    lv_obj_t *div = ui_window_create("FARM UPGRADE", body, true);
    lv_obj_set_size(div, 250, 200);
    lv_obj_center(div);

    ui_window_disable_keep_alive(div); // 这个窗口关闭时直接删除对象，不保持存活

    return div;
}

// 100ms 定时器
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
