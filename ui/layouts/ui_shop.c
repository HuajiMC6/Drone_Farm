#include "ui_common.h"
#include "ui_grid_list.h"
#include "ui_shop_cb.h"
#include "ui_window.h"

#include "icon.h"
#include "player.h"

#include "ui.h"

// 商店窗口上下文
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
    bool has_selection;
    shop_item_kind_t selected_kind;
    uint8_t selected_id;
    int selected_qty;
} shop_window_ctx_t;

static shop_window_ctx_t g_shop_window_ctx;
static shop_item_desc_t g_shop_seed_desc[CROP_TYPE_NONE];
static shop_item_desc_t g_shop_pesticide_desc[CROP_PESTICIDE_NONE];

static int ui_shop_get_unit_price(shop_item_kind_t kind, uint8_t id, bool *available);
static const char *ui_shop_get_item_name(shop_item_kind_t kind, uint8_t id);

// 工具函数：获取商品单价
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
        default:
            break;
    }

    if (available) {
        *available = false;
    }
    return 0;
}

// 工具函数：获取商品名称
static const char *ui_shop_get_item_name(shop_item_kind_t kind, uint8_t id) {
    switch (kind) {
        case SHOP_KIND_SEED:
            return id < CROP_TYPE_NONE ? crop_type_name((crop_type_t)id) : "Unknown Seed";
        case SHOP_KIND_PESTICIDE:
            return id < CROP_PESTICIDE_NONE ? crop_pesticide_name((crop_pesticide_t)id) : "Unknown Pesticide";
        default:
            return "Unknown";
    }
}

// 商店窗口刷新
void ui_shop_refresh(void) {
    if (!g_shop_window_ctx.obj || !lv_obj_is_valid(g_shop_window_ctx.obj)) {
        return;
    }

    player_t *player = player_get_instance();
    if (!player) {
        return;
    }

    if (!g_shop_window_ctx.has_selection) {
        return;
    }

    bool available = true;
    int unit = ui_shop_get_unit_price(g_shop_window_ctx.selected_kind, g_shop_window_ctx.selected_id, &available);
    int qty = g_shop_window_ctx.selected_qty;
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
        lv_obj_clear_state(g_shop_window_ctx.qty_minus_btn, LV_STATE_DISABLED);
    }
    if (g_shop_window_ctx.qty_plus_btn) {
        lv_obj_clear_state(g_shop_window_ctx.qty_plus_btn, LV_STATE_DISABLED);
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

// 处理商品被点击事件，更新选中状态和刷新右侧商品信息
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

// 减少购买数量
void ui_shop_qty_minus_click_handle(void) {
    if (!g_shop_window_ctx.has_selection) {
        return;
    }
    if (g_shop_window_ctx.selected_qty > 1) {
        g_shop_window_ctx.selected_qty--;
    }
    ui_shop_refresh();
}

// 增加购买数量
void ui_shop_qty_plus_click_handle(void) {
    if (!g_shop_window_ctx.has_selection) {
        return;
    }
    if (g_shop_window_ctx.selected_qty < 99) {
        g_shop_window_ctx.selected_qty++;
    }
    ui_shop_refresh();
}

// 获取当前选中的购买信息（商品类型和数量），供购买事件处理使用
bool ui_shop_get_selected_purchase(shop_item_desc_t *desc, int *qty) {
    if (!desc || !qty || !g_shop_window_ctx.has_selection) {
        return false;
    }

    *desc = (shop_item_desc_t){
        .kind = g_shop_window_ctx.selected_kind,
        .id = g_shop_window_ctx.selected_id,
    };

    *qty = g_shop_window_ctx.selected_qty;
    if (*qty < 1) {
        *qty = 1;
    }

    return true;
}

// 购买成功后的处理
void ui_shop_after_buy_success(shop_item_kind_t kind) {
    (void)kind;
    g_shop_window_ctx.selected_qty = 1;
}

// 商店窗口创建
lv_obj_t *ui_shop_window_create(void) {
    lv_obj_t *body = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 8, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(body, 0, 0);

    lv_obj_t *div = ui_window_create("SHOP", body, true);
    lv_obj_center(div);
    lv_obj_set_size(div, 524, 420);

    g_shop_window_ctx = (shop_window_ctx_t){0};

    lv_obj_t *left = lv_obj_create(body);
    lv_obj_set_size(left, 240, LV_PCT(100));
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
    lv_obj_set_style_bg_color(right, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(right, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(right, 1, 0);
    lv_obj_set_style_radius(right, 10, 0);
    lv_obj_set_style_pad_all(right, 10, 0);
    lv_obj_set_style_pad_row(right, 6, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *seed_card = ui_card_create_with_flex(left, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_t *seed_title = ui_label_colored(seed_card, "Seed Sales", lv_color_hex(0x5b421f));

    ui_grid_list_cfg_t seed_cfg = ui_grid_list_cfg_make(100, 100, 2, (CROP_TYPE_NONE + 1) / 2);
    ui_grid_list_cfg_set_pad(&seed_cfg, 8, 8, 0);
    ui_grid_list_t *seed_list = ui_grid_list_create(seed_card, &seed_cfg);
    if (seed_list) {
        lv_obj_t *seed_grid = ui_grid_list_get_obj(seed_list);
        lv_obj_clear_flag(seed_grid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(seed_grid, LV_SCROLLBAR_MODE_OFF);

        for (crop_type_t i = 0; i < CROP_TYPE_NONE; i++) {
            g_shop_seed_desc[i] = (shop_item_desc_t){.kind = SHOP_KIND_SEED, .id = i};
            lv_obj_t *item = ui_grid_list_add_icon_item(seed_list, icon_get_crop_item(i), ui_shop_item_click_cb,
                                                        &g_shop_seed_desc[i]);
            if (item) {
                lv_obj_set_style_border_width(item, 2, LV_STATE_CHECKED);
                lv_obj_set_style_border_color(item, lv_color_hex(0x8b5e3c), LV_STATE_CHECKED);
            }
        }
    }

    lv_obj_t *pest_card = ui_card_create_with_flex(left, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_t *pest_title = ui_label_colored(pest_card, "Pesticide Sales", lv_color_hex(0x5b421f));

    ui_grid_list_cfg_t pesticide_cfg = ui_grid_list_cfg_make(100, 100, 2, (CROP_PESTICIDE_NONE + 1) / 2);
    ui_grid_list_cfg_set_pad(&pesticide_cfg, 8, 8, 0);
    ui_grid_list_t *pesticide_list = ui_grid_list_create(pest_card, &pesticide_cfg);
    if (pesticide_list) {
        lv_obj_t *pesticide_grid = ui_grid_list_get_obj(pesticide_list);
        lv_obj_clear_flag(pesticide_grid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(pesticide_grid, LV_SCROLLBAR_MODE_OFF);

        for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
            g_shop_pesticide_desc[i] = (shop_item_desc_t){.kind = SHOP_KIND_PESTICIDE, .id = i};
            lv_obj_t *item = ui_grid_list_add_icon_item(pesticide_list, icon_get_pesticide(i), ui_shop_item_click_cb,
                                                        &g_shop_pesticide_desc[i]);
            if (item) {
                lv_obj_set_style_border_width(item, 2, LV_STATE_CHECKED);
                lv_obj_set_style_border_color(item, lv_color_hex(0x8b5e3c), LV_STATE_CHECKED);
            }
        }
    }

    lv_obj_t *r_title = ui_label_colored(right, "Item Details", lv_color_hex(0x5b421f));

    g_shop_window_ctx.name_label = lv_label_create(right);
    lv_label_set_text(g_shop_window_ctx.name_label, "Name: -");

    g_shop_window_ctx.unit_price_label = lv_label_create(right);
    lv_label_set_text(g_shop_window_ctx.unit_price_label, "Unit Price: -");

    lv_obj_t *qty_title = ui_label_colored(right, "Quantity", lv_color_hex(0x5b421f));

    /* 数量加减行：弹性 ROW */
    lv_obj_t *qty_row = ui_transparent_cont_create(right, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(qty_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(qty_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(qty_row, 12, 0);
    lv_obj_set_style_pad_all(qty_row, 4, 0);

    g_shop_window_ctx.qty_minus_btn = lv_btn_create(qty_row);
    lv_obj_set_size(g_shop_window_ctx.qty_minus_btn, 36, 30);
    lv_obj_add_style(g_shop_window_ctx.qty_minus_btn, &ui_style_btn_yellow, 0);
    lv_obj_t *minus_label = lv_label_create(g_shop_window_ctx.qty_minus_btn);
    lv_label_set_text(minus_label, "-");
    lv_obj_center(minus_label);
    lv_obj_add_event_cb(g_shop_window_ctx.qty_minus_btn, ui_shop_qty_minus_click_cb, LV_EVENT_CLICKED, NULL);

    g_shop_window_ctx.qty_label = lv_label_create(qty_row);
    lv_label_set_text(g_shop_window_ctx.qty_label, "1");

    g_shop_window_ctx.qty_plus_btn = lv_btn_create(qty_row);
    lv_obj_set_size(g_shop_window_ctx.qty_plus_btn, 36, 30);
    lv_obj_add_style(g_shop_window_ctx.qty_plus_btn, &ui_style_btn_pink, 0);
    lv_obj_t *plus_label = lv_label_create(g_shop_window_ctx.qty_plus_btn);
    lv_label_set_text(plus_label, "+");
    lv_obj_center(plus_label);
    lv_obj_add_event_cb(g_shop_window_ctx.qty_plus_btn, ui_shop_qty_plus_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *spacer = ui_div_create(right);
    lv_obj_set_flex_grow(spacer, 1);

    g_shop_window_ctx.total_row = ui_transparent_cont_create(right, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(g_shop_window_ctx.total_row, 8, 0);
    lv_obj_set_flex_flow(g_shop_window_ctx.total_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g_shop_window_ctx.total_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    g_shop_window_ctx.total_prefix_label = lv_label_create(g_shop_window_ctx.total_row);
    lv_label_set_text(g_shop_window_ctx.total_prefix_label, "Total:");

    g_shop_window_ctx.total_origin_label = lv_label_create(g_shop_window_ctx.total_row);
    lv_label_set_text(g_shop_window_ctx.total_origin_label, "-");

    g_shop_window_ctx.total_discount_label = lv_label_create(g_shop_window_ctx.total_row);
    lv_label_set_text(g_shop_window_ctx.total_discount_label, "");
    lv_obj_set_style_text_color(g_shop_window_ctx.total_discount_label, lv_color_hex(0x2e8b57), 0);
    lv_obj_add_flag(g_shop_window_ctx.total_discount_label, LV_OBJ_FLAG_HIDDEN);

    g_shop_window_ctx.gold_label = ui_label_colored(right, "Gold: 0", lv_color_hex(0x6a4f23));

    g_shop_window_ctx.buy_btn = lv_btn_create(right);
    lv_obj_set_size(g_shop_window_ctx.buy_btn, LV_PCT(100), 42);
    lv_obj_add_style(g_shop_window_ctx.buy_btn, &ui_style_btn_yellow, 0);
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

// 商店事件处理
void ui_shop_handle_event(event_t *event) {
    if (!event) {
        return;
    }

    switch (event->type) {
        case EVENT_ON_PLAYER_COIN_CHANGE:
        case EVENT_ON_PLAYER_LEVEL_UPGRADE:
        case EVENT_ON_PLAYER_SEED_CHANGE:
            ui_shop_refresh();
            break;
        default:
            break;
    }
}
