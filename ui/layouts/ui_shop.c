#include "ui_common.h"
#include "ui_grid_list.h"
#include "ui_shop_cb.h"
#include "ui_window.h"

#include "drone.h"
#include "farm.h"
#include "icon.h"
#include "player.h"

#include "ui.h"

typedef enum {
    SHOP_UPGRADE_DRONE_SPEED,
    SHOP_UPGRADE_DRONE_STORAGE,
    SHOP_UPGRADE_DRONE_ALGO,
    SHOP_UPGRADE_FARM_SIZE,
    SHOP_UPGRADE_NONE,
} shop_upgrade_type_t;

typedef struct {
    lv_obj_t *obj;
    lv_obj_t *name_label;
    lv_obj_t *unit_price_label;
    lv_obj_t *qty_label;
    lv_obj_t *total_row;
    lv_obj_t *total_prefix_label;
    lv_obj_t *total_origin_label;
    lv_obj_t *total_discount_label;
    lv_obj_t *gold_label;
    lv_obj_t *buy_btn;
    lv_obj_t *buy_btn_label;
    lv_obj_t *qty_minus_btn;
    lv_obj_t *qty_plus_btn;
    lv_obj_t *selected_item_obj;
    lv_obj_t *upgrade_item_btns[SHOP_UPGRADE_NONE];
    bool has_selection;
    shop_item_kind_t selected_kind;
    uint8_t selected_id;
    int selected_qty;
} shop_window_ctx_t;

static shop_window_ctx_t g_shop_window_ctx;
static shop_item_desc_t g_shop_seed_desc[CROP_TYPE_NONE];
static shop_item_desc_t g_shop_pesticide_desc[CROP_PESTICIDE_NONE];
static shop_item_desc_t g_shop_upgrade_desc[SHOP_UPGRADE_NONE];

static int ui_shop_get_unit_price(shop_item_kind_t kind, uint8_t id, bool *available);
static const char *ui_shop_get_item_name(shop_item_kind_t kind, uint8_t id);
static bool ui_shop_try_buy(shop_item_kind_t kind, uint8_t id, int qty);

static int ui_shop_get_unit_price(shop_item_kind_t kind, uint8_t id, bool *available) {
    if (available) {
        *available = true;
    }

    switch (kind) {
        case SHOP_KIND_SEED:
            if (id < CROP_TYPE_NONE)
                return seed_price[id];
            break;
        case SHOP_KIND_PESTICIDE:
            if (id < CROP_PESTICIDE_NONE)
                return pesticide_price[id];
            break;
        case SHOP_KIND_UPGRADE: {
            drone_t *drone = drone_get_instance();
            farm_t *farm = farm_get_instance();
            if (!drone || !farm) {
                if (available)
                    *available = false;
                return 0;
            }

            switch ((shop_upgrade_type_t)id) {
                case SHOP_UPGRADE_DRONE_SPEED:
                    if (drone->speed_level >= 3) {
                        if (available)
                            *available = false;
                        return 0;
                    }
                    return drone_speed_update_price[drone->speed_level];
                case SHOP_UPGRADE_DRONE_STORAGE:
                    if (drone->storage_level >= 3) {
                        if (available)
                            *available = false;
                        return 0;
                    }
                    return drone_storage_update_price[drone->storage_level];
                case SHOP_UPGRADE_DRONE_ALGO:
                    if (drone->algorithm_level >= 2) {
                        if (available)
                            *available = false;
                        return 0;
                    }
                    return drone_algorithm_update_price[drone->algorithm_level];
                case SHOP_UPGRADE_FARM_SIZE:
                    if (farm->size_level >= 3) {
                        if (available)
                            *available = false;
                        return 0;
                    }
                    return farm_size_update_price[farm->size_level];
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    if (available) {
        *available = false;
    }
    return 0;
}

static const char *ui_shop_get_item_name(shop_item_kind_t kind, uint8_t id) {
    static const char *upgrade_names[SHOP_UPGRADE_NONE] = {
        "Drone Speed Upgrade",
        "Drone Storage Upgrade",
        "Drone Algorithm Upgrade",
        "Farm Size Upgrade",
    };

    switch (kind) {
        case SHOP_KIND_SEED:
            return id < CROP_TYPE_NONE ? crop_type_name((crop_type_t)id) : "Unknown Seed";
        case SHOP_KIND_PESTICIDE:
            return id < CROP_PESTICIDE_NONE ? crop_pesticide_name((crop_pesticide_t)id) : "Unknown Pesticide";
        case SHOP_KIND_UPGRADE:
            return id < SHOP_UPGRADE_NONE ? upgrade_names[id] : "Unknown Upgrade";
        default:
            return "Unknown";
    }
}

static bool ui_shop_try_buy(shop_item_kind_t kind, uint8_t id, int qty) {
    switch (kind) {
        case SHOP_KIND_SEED:
            return id < CROP_TYPE_NONE ? player_buy_seed((crop_type_t)id, qty) : false;
        case SHOP_KIND_PESTICIDE:
            return id < CROP_PESTICIDE_NONE ? player_buy_pesticide((crop_pesticide_t)id, qty) : false;
        case SHOP_KIND_UPGRADE:
            switch ((shop_upgrade_type_t)id) {
                case SHOP_UPGRADE_DRONE_SPEED:
                    return player_buy_drone_speed_update();
                case SHOP_UPGRADE_DRONE_STORAGE:
                    return player_buy_drone_storage_update();
                case SHOP_UPGRADE_DRONE_ALGO:
                    return player_buy_drone_algorithm_update();
                case SHOP_UPGRADE_FARM_SIZE:
                    return player_buy_farm_size_update();
                default:
                    return false;
            }
        default:
            return false;
    }
}

void ui_shop_refresh(void) {
    if (!g_shop_window_ctx.obj || !lv_obj_is_valid(g_shop_window_ctx.obj)) {
        return;
    }

    player_t *player = player_get_instance();
    if (!player) {
        return;
    }

    for (uint8_t i = 0; i < SHOP_UPGRADE_NONE; i++) {
        lv_obj_t *btn = g_shop_window_ctx.upgrade_item_btns[i];
        if (!btn || !lv_obj_is_valid(btn)) {
            continue;
        }

        bool up_available = true;
        ui_shop_get_unit_price(SHOP_KIND_UPGRADE, i, &up_available);
        if (up_available) {
            lv_obj_clear_state(btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(btn, LV_STATE_DISABLED);
        }
    }

    if (!g_shop_window_ctx.has_selection) {
        return;
    }

    bool available = true;
    int unit = ui_shop_get_unit_price(g_shop_window_ctx.selected_kind, g_shop_window_ctx.selected_id, &available);
    int qty = g_shop_window_ctx.selected_kind == SHOP_KIND_UPGRADE ? 1 : g_shop_window_ctx.selected_qty;
    if (qty < 1) {
        qty = 1;
    }

    int origin_total = unit * qty;
    int discount_total = (int)(origin_total * level_discount[player->level_stage]);
    int discount_pct = (int)(level_discount[player->level_stage] * 100 + 0.5);

    if (g_shop_window_ctx.name_label) {
        lv_label_set_text_fmt(g_shop_window_ctx.name_label, "Name: %s",
                              ui_shop_get_item_name(g_shop_window_ctx.selected_kind, g_shop_window_ctx.selected_id));
    }
    if (g_shop_window_ctx.unit_price_label) {
        lv_label_set_text_fmt(g_shop_window_ctx.unit_price_label, "Unit Price: %d", unit);
    }
    if (g_shop_window_ctx.qty_label) {
        lv_label_set_text_fmt(g_shop_window_ctx.qty_label, "%d", qty);
    }
    if (g_shop_window_ctx.total_prefix_label && g_shop_window_ctx.total_origin_label) {
        if (discount_total < origin_total) {
            lv_label_set_text(g_shop_window_ctx.total_prefix_label, "Total:");
            lv_label_set_text_fmt(g_shop_window_ctx.total_origin_label, " %d ", origin_total);
            lv_obj_set_style_text_decor(g_shop_window_ctx.total_origin_label, LV_TEXT_DECOR_STRIKETHROUGH, 0);

            if (g_shop_window_ctx.total_discount_label) {
                lv_label_set_text_fmt(g_shop_window_ctx.total_discount_label, "%d (%d%% off)", discount_total,
                                      100 - discount_pct);
                lv_obj_clear_flag(g_shop_window_ctx.total_discount_label, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            lv_label_set_text(g_shop_window_ctx.total_prefix_label, "Total:");
            lv_label_set_text_fmt(g_shop_window_ctx.total_origin_label, " %d ", discount_total);
            lv_obj_set_style_text_decor(g_shop_window_ctx.total_origin_label, LV_TEXT_DECOR_NONE, 0);

            if (g_shop_window_ctx.total_discount_label) {
                lv_label_set_text(g_shop_window_ctx.total_discount_label, "");
                lv_obj_add_flag(g_shop_window_ctx.total_discount_label, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    if (g_shop_window_ctx.gold_label) {
        lv_label_set_text_fmt(g_shop_window_ctx.gold_label, "Gold: %d", player->coins);
    }

    bool enough = discount_total <= player->coins;
    bool can_buy = available && enough;

    if (g_shop_window_ctx.qty_minus_btn) {
        if (g_shop_window_ctx.selected_kind == SHOP_KIND_UPGRADE) {
            lv_obj_add_state(g_shop_window_ctx.qty_minus_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(g_shop_window_ctx.qty_minus_btn, LV_STATE_DISABLED);
        }
    }
    if (g_shop_window_ctx.qty_plus_btn) {
        if (g_shop_window_ctx.selected_kind == SHOP_KIND_UPGRADE) {
            lv_obj_add_state(g_shop_window_ctx.qty_plus_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(g_shop_window_ctx.qty_plus_btn, LV_STATE_DISABLED);
        }
    }

    if (g_shop_window_ctx.buy_btn && g_shop_window_ctx.buy_btn_label) {
        if (!available) {
            lv_obj_add_state(g_shop_window_ctx.buy_btn, LV_STATE_DISABLED);
            lv_label_set_text(g_shop_window_ctx.buy_btn_label, "Unavailable");
        } else if (!can_buy) {
            lv_obj_add_state(g_shop_window_ctx.buy_btn, LV_STATE_DISABLED);
            lv_label_set_text(g_shop_window_ctx.buy_btn_label, "Have no enough golds");
        } else {
            lv_obj_clear_state(g_shop_window_ctx.buy_btn, LV_STATE_DISABLED);
            lv_label_set_text(g_shop_window_ctx.buy_btn_label, "Buy Now");
        }
    }
}

void ui_shop_item_click_handle(const shop_item_desc_t *desc, lv_obj_t *target) {
    if (!desc || !target) {
        return;
    }

    if (g_shop_window_ctx.selected_item_obj && lv_obj_is_valid(g_shop_window_ctx.selected_item_obj)) {
        lv_obj_clear_state(g_shop_window_ctx.selected_item_obj, LV_STATE_CHECKED);
    }
    g_shop_window_ctx.selected_item_obj = target;
    lv_obj_add_state(target, LV_STATE_CHECKED);

    g_shop_window_ctx.has_selection = true;
    g_shop_window_ctx.selected_kind = desc->kind;
    g_shop_window_ctx.selected_id = desc->id;
    g_shop_window_ctx.selected_qty = 1;
    ui_shop_refresh();
}

void ui_shop_qty_minus_click_handle(void) {
    if (!g_shop_window_ctx.has_selection || g_shop_window_ctx.selected_kind == SHOP_KIND_UPGRADE) {
        return;
    }
    if (g_shop_window_ctx.selected_qty > 1) {
        g_shop_window_ctx.selected_qty--;
    }
    ui_shop_refresh();
}

void ui_shop_qty_plus_click_handle(void) {
    if (!g_shop_window_ctx.has_selection || g_shop_window_ctx.selected_kind == SHOP_KIND_UPGRADE) {
        return;
    }
    if (g_shop_window_ctx.selected_qty < 99) {
        g_shop_window_ctx.selected_qty++;
    }
    ui_shop_refresh();
}

void ui_shop_buy_click_handle(void) {
    if (!g_shop_window_ctx.has_selection) {
        return;
    }

    int qty = g_shop_window_ctx.selected_kind == SHOP_KIND_UPGRADE ? 1 : g_shop_window_ctx.selected_qty;
    if (qty < 1) {
        qty = 1;
    }

    if (ui_shop_try_buy(g_shop_window_ctx.selected_kind, g_shop_window_ctx.selected_id, qty)) {
        if (g_shop_window_ctx.selected_kind != SHOP_KIND_UPGRADE) {
            g_shop_window_ctx.selected_qty = 1;
        }
    }

    ui_shop_refresh();
}

lv_obj_t *ui_shop_window_create(void) {
    lv_obj_t *body = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 8, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *div = ui_window_create(lv_scr_act(), "SHOP", body, true);
    lv_obj_center(div);
    lv_obj_set_size(div, 524, 420);

    g_shop_window_ctx = (shop_window_ctx_t){0};

    lv_obj_t *left = lv_obj_create(body);
    lv_obj_set_size(left, 240, LV_PCT(100));
    lv_obj_set_pos(left, 0, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_style_pad_right(left, 14, 0);
    lv_obj_set_style_pad_row(left, 8, 0);
    lv_obj_set_scroll_dir(left, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *right = lv_obj_create(body);
    lv_obj_set_size(right, 260, LV_PCT(100));
    lv_obj_set_pos(right, 240, 0);
    lv_obj_set_style_bg_color(right, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(right, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(right, 1, 0);
    lv_obj_set_style_radius(right, 10, 0);
    lv_obj_set_style_pad_all(right, 10, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *seed_card = lv_obj_create(left);
    lv_obj_set_width(seed_card, LV_PCT(100));
    lv_obj_set_height(seed_card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(seed_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(seed_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(seed_card, 1, 0);
    lv_obj_set_style_radius(seed_card, 10, 0);
    lv_obj_set_style_pad_all(seed_card, 8, 0);
    lv_obj_set_style_pad_row(seed_card, 6, 0);
    lv_obj_set_flex_flow(seed_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(seed_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(seed_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(seed_card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *seed_title = lv_label_create(seed_card);
    lv_label_set_text(seed_title, "Seed Sales");
    lv_obj_set_style_text_color(seed_title, lv_color_hex(0x5b421f), 0);

    ui_grid_list_cfg_t seed_cfg;
    ui_grid_list_cfg_init(&seed_cfg);
    seed_cfg.item_w = 100;
    seed_cfg.item_h = 100;
    seed_cfg.col_count = 2;
    seed_cfg.row_count = (CROP_TYPE_NONE + 1) / 2;
    seed_cfg.pad_col = 8;
    seed_cfg.pad_row = 8;
    seed_cfg.pad_all = 0;
    ui_grid_list_t *seed_list = ui_grid_list_create(seed_card, &seed_cfg);
    if (seed_list) {
        lv_obj_t *seed_grid = ui_grid_list_get_obj(seed_list);
        lv_obj_clear_flag(seed_grid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(seed_grid, LV_SCROLLBAR_MODE_OFF);

        for (crop_type_t i = 0; i < CROP_TYPE_NONE; i++) {
            g_shop_seed_desc[i] = (shop_item_desc_t){.kind = SHOP_KIND_SEED, .id = i};
            lv_obj_t *item = ui_grid_list_add_item(seed_list);
            if (!item) {
                continue;
            }
            lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_pad_all(item, 0, 0);
            lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *icon = lv_img_create(item);
            lv_img_set_src(icon, &icon_crop_wheat_ripe);
            lv_obj_center(icon);
            ui_grid_list_bind_item_event(item, ui_shop_item_click_cb, LV_EVENT_CLICKED, &g_shop_seed_desc[i]);
        }
    }

    lv_obj_t *pest_card = lv_obj_create(left);
    lv_obj_set_width(pest_card, LV_PCT(100));
    lv_obj_set_height(pest_card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(pest_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(pest_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(pest_card, 1, 0);
    lv_obj_set_style_radius(pest_card, 10, 0);
    lv_obj_set_style_pad_all(pest_card, 8, 0);
    lv_obj_set_style_pad_row(pest_card, 6, 0);
    lv_obj_set_flex_flow(pest_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pest_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(pest_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(pest_card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *pest_title = lv_label_create(pest_card);
    lv_label_set_text(pest_title, "Pesticide Sales");
    lv_obj_set_style_text_color(pest_title, lv_color_hex(0x5b421f), 0);

    ui_grid_list_cfg_t pesticide_cfg;
    ui_grid_list_cfg_init(&pesticide_cfg);
    pesticide_cfg.item_w = 100;
    pesticide_cfg.item_h = 100;
    pesticide_cfg.col_count = 2;
    pesticide_cfg.row_count = (CROP_PESTICIDE_NONE + 1) / 2;
    pesticide_cfg.pad_col = 8;
    pesticide_cfg.pad_row = 8;
    pesticide_cfg.pad_all = 0;
    ui_grid_list_t *pesticide_list = ui_grid_list_create(pest_card, &pesticide_cfg);
    if (pesticide_list) {
        lv_obj_t *pesticide_grid = ui_grid_list_get_obj(pesticide_list);
        lv_obj_clear_flag(pesticide_grid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(pesticide_grid, LV_SCROLLBAR_MODE_OFF);

        for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
            g_shop_pesticide_desc[i] = (shop_item_desc_t){.kind = SHOP_KIND_PESTICIDE, .id = i};
            lv_obj_t *item = ui_grid_list_add_item(pesticide_list);
            if (!item) {
                continue;
            }
            lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_pad_all(item, 0, 0);
            lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *icon = lv_img_create(item);
            lv_img_set_src(icon, &icon_crop_wheat_ripe);
            lv_obj_center(icon);
            ui_grid_list_bind_item_event(item, ui_shop_item_click_cb, LV_EVENT_CLICKED, &g_shop_pesticide_desc[i]);
        }
    }

    lv_obj_t *upgrade_card = lv_obj_create(left);
    lv_obj_set_width(upgrade_card, LV_PCT(100));
    lv_obj_set_height(upgrade_card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(upgrade_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(upgrade_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(upgrade_card, 1, 0);
    lv_obj_set_style_radius(upgrade_card, 10, 0);
    lv_obj_set_style_pad_all(upgrade_card, 8, 0);
    lv_obj_set_style_pad_row(upgrade_card, 6, 0);
    lv_obj_set_flex_flow(upgrade_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(upgrade_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(upgrade_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(upgrade_card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *upgrade_title = lv_label_create(upgrade_card);
    lv_label_set_text(upgrade_title, "Upgrade Purchases");
    lv_obj_set_style_text_color(upgrade_title, lv_color_hex(0x5b421f), 0);

    ui_grid_list_cfg_t upgrade_cfg;
    ui_grid_list_cfg_init(&upgrade_cfg);
    upgrade_cfg.item_w = 100;
    upgrade_cfg.item_h = 100;
    upgrade_cfg.col_count = 2;
    upgrade_cfg.row_count = 2;
    upgrade_cfg.pad_col = 8;
    upgrade_cfg.pad_row = 8;
    upgrade_cfg.pad_all = 0;
    ui_grid_list_t *upgrade_list = ui_grid_list_create(upgrade_card, &upgrade_cfg);
    if (upgrade_list) {
        lv_obj_t *upgrade_grid = ui_grid_list_get_obj(upgrade_list);
        lv_obj_clear_flag(upgrade_grid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(upgrade_grid, LV_SCROLLBAR_MODE_OFF);

        for (uint8_t i = 0; i < SHOP_UPGRADE_NONE; i++) {
            g_shop_upgrade_desc[i] = (shop_item_desc_t){.kind = SHOP_KIND_UPGRADE, .id = i};
            lv_obj_t *item = ui_grid_list_add_item(upgrade_list);
            if (!item) {
                continue;
            }
            lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_pad_all(item, 0, 0);
            lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *icon = lv_img_create(item);
            lv_img_set_src(icon, &icon_crop_wheat_ripe);
            lv_obj_center(icon);
            ui_grid_list_bind_item_event(item, ui_shop_item_click_cb, LV_EVENT_CLICKED, &g_shop_upgrade_desc[i]);
            g_shop_window_ctx.upgrade_item_btns[i] = item;
        }
    }

    lv_obj_t *r_title = lv_label_create(right);
    lv_label_set_text(r_title, "Item Details");
    lv_obj_set_style_text_color(r_title, lv_color_hex(0x5b421f), 0);
    lv_obj_set_pos(r_title, 0, 0);

    g_shop_window_ctx.name_label = lv_label_create(right);
    lv_label_set_text(g_shop_window_ctx.name_label, "Name: -");
    lv_obj_set_pos(g_shop_window_ctx.name_label, 0, 32);

    g_shop_window_ctx.unit_price_label = lv_label_create(right);
    lv_label_set_text(g_shop_window_ctx.unit_price_label, "Unit Price: -");
    lv_obj_set_pos(g_shop_window_ctx.unit_price_label, 0, 56);

    lv_obj_t *qty_title = lv_label_create(right);
    lv_label_set_text(qty_title, "Quantity");
    lv_obj_set_pos(qty_title, 0, 92);

    g_shop_window_ctx.qty_minus_btn = lv_btn_create(right);
    lv_obj_set_size(g_shop_window_ctx.qty_minus_btn, 36, 30);
    lv_obj_set_pos(g_shop_window_ctx.qty_minus_btn, 0, 116);
    lv_obj_t *minus_label = lv_label_create(g_shop_window_ctx.qty_minus_btn);
    lv_label_set_text(minus_label, "-");
    lv_obj_center(minus_label);
    lv_obj_add_event_cb(g_shop_window_ctx.qty_minus_btn, ui_shop_qty_minus_click_cb, LV_EVENT_CLICKED, NULL);

    g_shop_window_ctx.qty_label = lv_label_create(right);
    lv_label_set_text(g_shop_window_ctx.qty_label, "1");
    lv_obj_set_pos(g_shop_window_ctx.qty_label, 52, 122);

    g_shop_window_ctx.qty_plus_btn = lv_btn_create(right);
    lv_obj_set_size(g_shop_window_ctx.qty_plus_btn, 36, 30);
    lv_obj_set_pos(g_shop_window_ctx.qty_plus_btn, 90, 116);
    lv_obj_t *plus_label = lv_label_create(g_shop_window_ctx.qty_plus_btn);
    lv_label_set_text(plus_label, "+");
    lv_obj_center(plus_label);
    lv_obj_add_event_cb(g_shop_window_ctx.qty_plus_btn, ui_shop_qty_plus_click_cb, LV_EVENT_CLICKED, NULL);

    g_shop_window_ctx.total_row = lv_obj_create(right);
    lv_obj_set_size(g_shop_window_ctx.total_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_pos(g_shop_window_ctx.total_row, 0, 242);
    lv_obj_set_style_bg_opa(g_shop_window_ctx.total_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_shop_window_ctx.total_row, 0, 0);
    lv_obj_set_style_pad_all(g_shop_window_ctx.total_row, 0, 0);
    lv_obj_set_style_pad_column(g_shop_window_ctx.total_row, 8, 0);
    lv_obj_set_flex_flow(g_shop_window_ctx.total_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_shop_window_ctx.total_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(g_shop_window_ctx.total_row, LV_OBJ_FLAG_SCROLLABLE);

    g_shop_window_ctx.total_prefix_label = lv_label_create(g_shop_window_ctx.total_row);
    lv_label_set_text(g_shop_window_ctx.total_prefix_label, "Total:");

    g_shop_window_ctx.total_origin_label = lv_label_create(g_shop_window_ctx.total_row);
    lv_label_set_text(g_shop_window_ctx.total_origin_label, "-");

    g_shop_window_ctx.total_discount_label = lv_label_create(g_shop_window_ctx.total_row);
    lv_label_set_text(g_shop_window_ctx.total_discount_label, "");
    lv_obj_set_style_text_color(g_shop_window_ctx.total_discount_label, lv_color_hex(0x2e8b57), 0);
    lv_obj_add_flag(g_shop_window_ctx.total_discount_label, LV_OBJ_FLAG_HIDDEN);

    g_shop_window_ctx.gold_label = lv_label_create(right);
    lv_label_set_text(g_shop_window_ctx.gold_label, "Gold: 0");
    lv_obj_set_pos(g_shop_window_ctx.gold_label, 0, 268);
    lv_obj_set_style_text_color(g_shop_window_ctx.gold_label, lv_color_hex(0x6a4f23), 0);

    g_shop_window_ctx.buy_btn = lv_btn_create(right);
    lv_obj_set_size(g_shop_window_ctx.buy_btn, LV_PCT(100), 42);
    lv_obj_set_pos(g_shop_window_ctx.buy_btn, 0, 292);
    g_shop_window_ctx.buy_btn_label = lv_label_create(g_shop_window_ctx.buy_btn);
    lv_label_set_text(g_shop_window_ctx.buy_btn_label, "Select Item");
    lv_obj_center(g_shop_window_ctx.buy_btn_label);
    lv_obj_add_state(g_shop_window_ctx.buy_btn, LV_STATE_DISABLED);
    lv_obj_add_event_cb(g_shop_window_ctx.buy_btn, ui_shop_buy_click_cb, LV_EVENT_CLICKED, NULL);

    g_shop_window_ctx.obj = div;
    g_shop_window_ctx.has_selection = false;
    g_shop_window_ctx.selected_qty = 1;
    ui_shop_refresh();

    return div;
}
