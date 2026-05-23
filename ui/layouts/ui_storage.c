#include "ui_common.h"
#include "ui_grid_list.h"
#include "ui_storage_cb.h"
#include "ui_window.h"

#include "icon.h"
#include "player.h"
#include "ui.h"

typedef struct {
    ui_grid_list_t *list;
    lv_obj_t *obj;
    lv_obj_t *list_obj;
    lv_obj_t *empty_label;
    lv_obj_t *selected_name_label;
    lv_obj_t *owned_label;
    lv_obj_t *qty_label;
    lv_obj_t *unit_price_label;
    lv_obj_t *total_label;
    lv_obj_t *hint_label;
    lv_obj_t *minus_btn;
    lv_obj_t *plus_btn;
    lv_obj_t *sell_btn;
    lv_obj_t *sell_btn_label;
    lv_obj_t *selected_item_obj;
    crop_type_t selected_type;
    int selected_qty;
} ui_storage_window_ctx_t;

static ui_storage_window_ctx_t g_storage_window_ctx;
static ui_storage_crop_desc_t g_storage_crop_desc[CROP_TYPE_NONE];
static lv_obj_t *g_storage_items[CROP_TYPE_NONE];
static lv_obj_t *g_storage_count_labels[CROP_TYPE_NONE];

static void ui_storage_window_rebuild_list(player_t *player);
static void ui_storage_window_select(crop_type_t type, lv_obj_t *target);
static void ui_storage_window_clear_selection(void);
static void ui_storage_window_update_controls(player_t *player);

void ui_storage_window_refresh(void) {
    player_t *player = player_get_instance();
    if (!player || !g_storage_window_ctx.obj || !lv_obj_is_valid(g_storage_window_ctx.obj)) {
        return;
    }

    ui_storage_window_rebuild_list(player);
    ui_storage_window_update_controls(player);
}

lv_obj_t *ui_storage_window_create(void) {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_t *body = ui_div_create(screen);
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 8, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *div = ui_window_create("STORAGE", body, true);
    lv_obj_center(div);
    lv_obj_set_size(div, 328, 420);

    g_storage_window_ctx = (ui_storage_window_ctx_t){0};
    g_storage_window_ctx.obj = div;
    g_storage_window_ctx.selected_type = CROP_TYPE_NONE;

    lv_obj_t *title = lv_label_create(body);
    lv_label_set_text(title, "Harvest Bag");
    lv_obj_set_style_text_color(title, lv_color_hex(0x5b421f), 0);

    ui_grid_list_cfg_t cfg;
    ui_grid_list_cfg_init(&cfg);
    cfg.item_w = 95;
    cfg.item_h = 82;
    cfg.col_count = 3;
    cfg.row_count = 1;
    cfg.pad_col = 8;
    cfg.pad_row = 8;
    cfg.pad_all = 0;
    g_storage_window_ctx.list = ui_grid_list_create(body, &cfg);
    if (g_storage_window_ctx.list) {
        g_storage_window_ctx.list_obj = ui_grid_list_get_obj(g_storage_window_ctx.list);
        if (g_storage_window_ctx.list_obj) {
            lv_obj_clear_flag(g_storage_window_ctx.list_obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(g_storage_window_ctx.list_obj, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_width(g_storage_window_ctx.list_obj, LV_PCT(100));
        }
    }

    g_storage_window_ctx.empty_label = lv_label_create(body);
    lv_label_set_text(g_storage_window_ctx.empty_label, "No harvested crops");
    lv_obj_set_style_text_color(g_storage_window_ctx.empty_label, lv_color_hex(0x8a6a3f), 0);

    lv_obj_t *sell_card = lv_obj_create(body);
    lv_obj_set_width(sell_card, LV_PCT(100));
    lv_obj_set_height(sell_card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(sell_card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(sell_card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(sell_card, 1, 0);
    lv_obj_set_style_radius(sell_card, 10, 0);
    lv_obj_set_style_pad_all(sell_card, 8, 0);
    lv_obj_set_style_pad_row(sell_card, 6, 0);
    lv_obj_set_flex_flow(sell_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sell_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(sell_card, LV_OBJ_FLAG_SCROLLABLE);

    g_storage_window_ctx.selected_name_label = lv_label_create(sell_card);
    lv_label_set_text(g_storage_window_ctx.selected_name_label, "Selected: -");
    lv_obj_set_style_text_color(g_storage_window_ctx.selected_name_label, lv_color_hex(0x5b421f), 0);

    g_storage_window_ctx.owned_label = lv_label_create(sell_card);
    lv_label_set_text(g_storage_window_ctx.owned_label, "Owned: x0");
    lv_obj_set_style_text_color(g_storage_window_ctx.owned_label, lv_color_hex(0x5b421f), 0);

    lv_obj_t *qty_row = lv_obj_create(sell_card);
    lv_obj_set_width(qty_row, LV_PCT(100));
    lv_obj_set_height(qty_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(qty_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(qty_row, 0, 0);
    lv_obj_set_style_pad_all(qty_row, 0, 0);
    lv_obj_set_flex_flow(qty_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(qty_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(qty_row, LV_OBJ_FLAG_SCROLLABLE);

    g_storage_window_ctx.minus_btn = lv_btn_create(qty_row);
    lv_obj_set_size(g_storage_window_ctx.minus_btn, 34, 34);
    lv_obj_t *minus_label = lv_label_create(g_storage_window_ctx.minus_btn);
    lv_label_set_text(minus_label, "-");
    lv_obj_center(minus_label);
    lv_obj_add_event_cb(g_storage_window_ctx.minus_btn, ui_storage_qty_minus_click_cb, LV_EVENT_CLICKED, NULL);

    g_storage_window_ctx.qty_label = lv_label_create(qty_row);
    lv_label_set_text(g_storage_window_ctx.qty_label, "0");
    lv_obj_set_style_text_color(g_storage_window_ctx.qty_label, lv_color_hex(0x5b421f), 0);

    g_storage_window_ctx.plus_btn = lv_btn_create(qty_row);
    lv_obj_set_size(g_storage_window_ctx.plus_btn, 34, 34);
    lv_obj_t *plus_label = lv_label_create(g_storage_window_ctx.plus_btn);
    lv_label_set_text(plus_label, "+");
    lv_obj_center(plus_label);
    lv_obj_add_event_cb(g_storage_window_ctx.plus_btn, ui_storage_qty_plus_click_cb, LV_EVENT_CLICKED, NULL);

    g_storage_window_ctx.unit_price_label = lv_label_create(sell_card);
    lv_label_set_text(g_storage_window_ctx.unit_price_label, "Unit Price: 0");
    lv_obj_set_style_text_color(g_storage_window_ctx.unit_price_label, lv_color_hex(0x5b421f), 0);

    g_storage_window_ctx.total_label = lv_label_create(sell_card);
    lv_label_set_text(g_storage_window_ctx.total_label, "Expected Revenue: 0");
    lv_obj_set_style_text_color(g_storage_window_ctx.total_label, lv_color_hex(0x5b421f), 0);

    g_storage_window_ctx.hint_label = lv_label_create(sell_card);
    lv_label_set_text(g_storage_window_ctx.hint_label, "Select a crop above to sell");
    lv_obj_set_style_text_color(g_storage_window_ctx.hint_label, lv_color_hex(0x8a6a3f), 0);

    g_storage_window_ctx.sell_btn = lv_btn_create(sell_card);
    lv_obj_set_size(g_storage_window_ctx.sell_btn, LV_PCT(100), 40);
    g_storage_window_ctx.sell_btn_label = lv_label_create(g_storage_window_ctx.sell_btn);
    lv_label_set_text(g_storage_window_ctx.sell_btn_label, "Sell");
    lv_obj_center(g_storage_window_ctx.sell_btn_label);
    lv_obj_add_event_cb(g_storage_window_ctx.sell_btn, ui_storage_sell_click_cb, LV_EVENT_CLICKED, NULL);

    ui_storage_window_refresh();
    return div;
}

static void ui_storage_window_clear_selection(void) {
    if (g_storage_window_ctx.selected_item_obj && lv_obj_is_valid(g_storage_window_ctx.selected_item_obj)) {
        lv_obj_clear_state(g_storage_window_ctx.selected_item_obj, LV_STATE_CHECKED);
    }

    g_storage_window_ctx.selected_item_obj = NULL;
    g_storage_window_ctx.selected_type = CROP_TYPE_NONE;
    g_storage_window_ctx.selected_qty = 0;
}

static void ui_storage_window_rebuild_list(player_t *player) {
    ui_storage_window_ctx_t *ctx = &g_storage_window_ctx;
    if (!ctx->list || !ctx->list_obj || !lv_obj_is_valid(ctx->list_obj) || !player) {
        return;
    }

    ui_grid_list_cfg_t cfg;
    ui_grid_list_cfg_init(&cfg);
    cfg.item_w = 95;
    cfg.item_h = 82;
    cfg.col_count = 3;
    cfg.pad_col = 8;
    cfg.pad_row = 8;
    cfg.pad_all = 0;

    uint16_t visible_count = 0;
    for (crop_type_t i = 0; i < CROP_TYPE_NONE; i++) {
        if (player->harvest_bag[i] > 0) {
            visible_count++;
        }
    }
    cfg.row_count = visible_count > 0 ? (visible_count + cfg.col_count - 1) / cfg.col_count : 1;

    ui_grid_list_reset(ctx->list, &cfg);

    memset(g_storage_items, 0, sizeof(g_storage_items));
    memset(g_storage_count_labels, 0, sizeof(g_storage_count_labels));

    bool selected_found = false;
    for (crop_type_t i = 0; i < CROP_TYPE_NONE; i++) {
        int owned = player->harvest_bag[i];
        if (owned <= 0) {
            continue;
        }

        lv_obj_t *item = ui_grid_list_add_item(ctx->list);
        if (!item) {
            continue;
        }

        g_storage_crop_desc[i] = (ui_storage_crop_desc_t){.type = i};
        g_storage_items[i] = item;

        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_pad_all(item, 0, 0);
        lv_obj_set_style_border_width(item, 2, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(item, lv_color_hex(0x8b5e3c), LV_STATE_CHECKED);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *icon = lv_img_create(item);
        const void *img = icon_get_crop(i, CROP_STAGE_READY);
        if (!img) {
            img = icon_get_crop(i, CROP_STAGE_SEED);
        }
        if (img) {
            lv_img_set_src(icon, img);
        }
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t *name_label = lv_label_create(item);
        lv_label_set_text(name_label, crop_type_name(i));
        lv_obj_set_style_text_color(name_label, lv_color_hex(0x5b421f), 0);
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_t *count_label = lv_label_create(item);
        lv_label_set_text_fmt(count_label, "x%d", owned);
        lv_obj_set_style_text_color(count_label, lv_color_hex(0x5b421f), 0);
        lv_obj_align(count_label, LV_ALIGN_TOP_RIGHT, -2, 2);
        g_storage_count_labels[i] = count_label;

        lv_obj_add_event_cb(item, ui_storage_item_click_cb, LV_EVENT_CLICKED, &g_storage_crop_desc[i]);

        if (ctx->selected_type == i) {
            ctx->selected_item_obj = item;
            lv_obj_add_state(item, LV_STATE_CHECKED);
            selected_found = true;
        }
    }

    if (ctx->selected_type != CROP_TYPE_NONE && !selected_found) {
        ui_storage_window_clear_selection();
    }

    if (ctx->empty_label && lv_obj_is_valid(ctx->empty_label)) {
        if (visible_count == 0) {
            lv_obj_clear_flag(ctx->empty_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->empty_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_storage_window_update_controls(player_t *player) {
    ui_storage_window_ctx_t *ctx = &g_storage_window_ctx;
    crop_type_t type = ctx->selected_type;
    int owned = 0;
    int unit_price = 0;
    int qty = ctx->selected_qty;

    if (type < CROP_TYPE_NONE) {
        owned = player->harvest_bag[type];
        unit_price = harvest_price[type];
    }

    if (type >= CROP_TYPE_NONE || owned <= 0) {
        ui_storage_window_clear_selection();
        type = CROP_TYPE_NONE;
        owned = 0;
        unit_price = 0;
        qty = 0;
    } else {
        if (qty < 1) {
            qty = 1;
        }
        if (qty > owned) {
            qty = owned;
        }
        ctx->selected_qty = qty;
    }

    if (ctx->selected_name_label) {
        if (type < CROP_TYPE_NONE) {
            lv_label_set_text_fmt(ctx->selected_name_label, "Selected: %s", crop_type_name(type));
        } else {
            lv_label_set_text(ctx->selected_name_label, "Selected: -");
        }
    }
    if (ctx->owned_label) {
        lv_label_set_text_fmt(ctx->owned_label, "Owned: x%d", owned);
    }
    if (ctx->qty_label) {
        lv_label_set_text_fmt(ctx->qty_label, "%d", qty);
    }
    if (ctx->unit_price_label) {
        lv_label_set_text_fmt(ctx->unit_price_label, "Unit Price: %d", unit_price);
    }
    if (ctx->total_label) {
        lv_label_set_text_fmt(ctx->total_label, "Expected Revenue: %d", unit_price * qty);
    }
    if (ctx->hint_label) {
        if (type < CROP_TYPE_NONE) {
            lv_label_set_text(ctx->hint_label, "");
        } else {
            lv_label_set_text(ctx->hint_label, "Select a crop above to sell");
        }
    }

    bool has_selection = type < CROP_TYPE_NONE && qty > 0;
    if (ctx->minus_btn) {
        if (!has_selection || qty <= 1) {
            lv_obj_add_state(ctx->minus_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(ctx->minus_btn, LV_STATE_DISABLED);
        }
    }
    if (ctx->plus_btn) {
        if (!has_selection || qty >= owned) {
            lv_obj_add_state(ctx->plus_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(ctx->plus_btn, LV_STATE_DISABLED);
        }
    }
    if (ctx->sell_btn && ctx->sell_btn_label) {
        if (!has_selection) {
            lv_obj_add_state(ctx->sell_btn, LV_STATE_DISABLED);
            lv_label_set_text(ctx->sell_btn_label, "Sell");
        } else {
            lv_obj_clear_state(ctx->sell_btn, LV_STATE_DISABLED);
            lv_label_set_text_fmt(ctx->sell_btn_label, "Sell x%d", qty);
        }
    }
}

static void ui_storage_window_select(crop_type_t type, lv_obj_t *target) {
    ui_storage_window_ctx_t *ctx = &g_storage_window_ctx;
    if (ctx->selected_item_obj && ctx->selected_item_obj != target && lv_obj_is_valid(ctx->selected_item_obj)) {
        lv_obj_clear_state(ctx->selected_item_obj, LV_STATE_CHECKED);
    }

    ctx->selected_item_obj = target;
    ctx->selected_type = type;
    ctx->selected_qty = 1;
    if (target && lv_obj_is_valid(target)) {
        lv_obj_add_state(target, LV_STATE_CHECKED);
    }

    ui_storage_window_update_controls(player_get_instance());
}

void ui_storage_item_click_handle(const ui_storage_crop_desc_t *desc, lv_obj_t *target) {
    if (!desc || !target) {
        return;
    }

    ui_storage_window_select(desc->type, target);
}

void ui_storage_qty_minus_click_handle(void) {
    ui_storage_window_ctx_t *ctx = &g_storage_window_ctx;
    if (ctx->selected_type >= CROP_TYPE_NONE || ctx->selected_qty <= 1) {
        return;
    }

    ctx->selected_qty--;
    ui_storage_window_update_controls(player_get_instance());
}

void ui_storage_qty_plus_click_handle(void) {
    ui_storage_window_ctx_t *ctx = &g_storage_window_ctx;
    player_t *player = player_get_instance();
    if (!player || ctx->selected_type >= CROP_TYPE_NONE) {
        return;
    }

    int owned = player->harvest_bag[ctx->selected_type];
    if (ctx->selected_qty < owned) {
        ctx->selected_qty++;
    }
    ui_storage_window_update_controls(player);
}

void ui_storage_sell_click_handle(void) {
    ui_storage_window_ctx_t *ctx = &g_storage_window_ctx;
    if (ctx->selected_type >= CROP_TYPE_NONE || ctx->selected_qty <= 0) {
        return;
    }

    if (player_sold(ctx->selected_type, ctx->selected_qty)) {
        ui_storage_window_refresh();
    }
}
