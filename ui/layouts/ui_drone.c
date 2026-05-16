#include "player.h"
#include "ui_common.h"
#include "ui_drone_cb.h"
#include "ui_grid_list.h"
#include "ui_window.h"

#include "drone.h"
#include "icon.h"

static lv_obj_t *g_drone_window = NULL;

typedef struct {
    lv_obj_t *obj;
    lv_obj_t *speed_label;
    lv_obj_t *storage_label;
    lv_obj_t *pest_card_title;
    lv_obj_t *pest_row_name_labels[CROP_PESTICIDE_NONE];
    lv_obj_t *pest_result_labels[CROP_DAMAGE_NONE];
    lv_obj_t *pesticide_card_title;
    lv_obj_t *pesticide_row_name_labels[CROP_PESTICIDE_NONE];
    lv_obj_t *pesticide_result_labels[CROP_PESTICIDE_NONE];
    lv_obj_t *detect_btn;
    lv_obj_t *spray_btn;
    lv_obj_t *pesticide_load_labels[CROP_PESTICIDE_NONE];
    lv_obj_t *pesticide_bag_labels[CROP_PESTICIDE_NONE];
    ui_grid_list_t *pesticide_bag_list;
    lv_obj_t *state_label;
} drone_panel_ctx_t;

static drone_panel_ctx_t g_drone_window_ctx;
static drone_panel_ctx_t g_drone_hud_ctx;
static drone_pesticide_btn_desc_t g_drone_pesticide_btn_desc[CROP_PESTICIDE_NONE][2];
static drone_mode_btn_desc_t g_drone_detect_btn_desc = {.target_state = DRONE_STATE_DETECTING};
static drone_mode_btn_desc_t g_drone_spray_btn_desc = {.target_state = DRONE_STATE_AUTO};

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

static void ui_drone_panel_refresh(drone_panel_ctx_t *ctx) {
    if (!ctx || !ctx->obj || !lv_obj_is_valid(ctx->obj)) {
        return;
    }

    drone_t *drone = drone_get_instance();
    if (!drone) {
        return;
    }

    if (ctx->state_label) {
        lv_label_set_text_fmt(ctx->state_label, "State: %s", drone_state_name(drone->drone_state));
    }
    if (ctx->speed_label) {
        lv_label_set_text_fmt(ctx->speed_label, "%d m/s", drone->speed);
    }
    if (ctx->storage_label) {
        lv_label_set_text_fmt(ctx->storage_label, "%d / pesticide", drone->storage_capacity);
    }

    bool detecting = drone->drone_state == DRONE_STATE_DETECTING;
    bool spraying = drone->drone_state == DRONE_STATE_AUTO;

    bool show_dual_stats = (ctx == &g_drone_hud_ctx) && (ctx->pesticide_card_title != NULL);

    if (show_dual_stats) {
        if (ctx->pest_card_title) {
            lv_label_set_text(ctx->pest_card_title, "Last Scan Pest Count");
        }

        for (crop_damage_t i = 0; i < CROP_DAMAGE_NONE; i++) {
            if (ctx->pest_row_name_labels[i]) {
                lv_label_set_text(ctx->pest_row_name_labels[i], crop_pest_name(i));
            }

            if (ctx->pest_result_labels[i]) {
                lv_label_set_text_fmt(ctx->pest_result_labels[i], "%d", ui_drone_pest_count[i]);
            }
        }

        if (ctx->pesticide_card_title) {
            lv_label_set_text(ctx->pesticide_card_title, "Loaded Pesticide Count");
        }

        for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
            if (ctx->pesticide_row_name_labels[i]) {
                lv_label_set_text(ctx->pesticide_row_name_labels[i], crop_pesticide_name(i));
            }

            if (ctx->pesticide_result_labels[i]) {
                lv_label_set_text_fmt(ctx->pesticide_result_labels[i], "%d", drone->pesticide_storage[i]);
            }
        }
    } else {
        if (ctx->pest_card_title) {
            lv_label_set_text(ctx->pest_card_title, spraying ? "Loaded Pesticide Count" : "Last Scan Pest Count");
        }

        for (crop_damage_t i = 0; i < CROP_DAMAGE_NONE; i++) {
            if (ctx->pest_row_name_labels[i]) {
                lv_label_set_text(ctx->pest_row_name_labels[i],
                                  spraying ? crop_pesticide_name((crop_pesticide_t)i) : crop_pest_name(i));
            }

            if (ctx->pest_result_labels[i]) {
                lv_label_set_text_fmt(ctx->pest_result_labels[i], "%d",
                                      spraying ? drone->pesticide_storage[i] : ui_drone_pest_count[i]);
            }
        }
    }

    player_t *player = player_get_instance();
    if (player) {
        for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
            if (ctx->pesticide_load_labels[i]) {
                lv_label_set_text_fmt(ctx->pesticide_load_labels[i], "%d", drone->pesticide_storage[i]);
            }

            if (ctx->pesticide_bag_labels[i]) {
                lv_label_set_text_fmt(ctx->pesticide_bag_labels[i], "%s: %d", crop_pesticide_name(i),
                                      player->pesticide_bag[i]);
            }
        }
    }

    if (ctx->detect_btn) {
        ui_drone_btn_set_text(ctx->detect_btn, detecting ? "Recall" : "Start Detect");
        lv_obj_set_style_bg_color(ctx->detect_btn, detecting ? lv_color_hex(0x2f9a5f) : lv_color_hex(0xefcd76), 0);
        if (spraying) {
            lv_obj_add_state(ctx->detect_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(ctx->detect_btn, LV_STATE_DISABLED);
        }
    }

    if (ctx->spray_btn) {
        ui_drone_btn_set_text(ctx->spray_btn, spraying ? "Stop Spray" : "Start Spray");
        lv_obj_set_style_bg_color(ctx->spray_btn, spraying ? lv_color_hex(0x2f9a5f) : lv_color_hex(0xefcd76), 0);
        if (detecting) {
            lv_obj_add_state(ctx->spray_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(ctx->spray_btn, LV_STATE_DISABLED);
        }
    }
}

void ui_drone_window_refresh(void) {
    ui_drone_panel_refresh(&g_drone_window_ctx);
    ui_drone_panel_refresh(&g_drone_hud_ctx);
}

void ui_drone_hud_set_visible(bool visible) {
    if (!g_drone_hud_ctx.obj || !lv_obj_is_valid(g_drone_hud_ctx.obj)) {
        return;
    }

    if (visible) {
        lv_obj_move_foreground(g_drone_hud_ctx.obj);
        lv_obj_clear_flag(g_drone_hud_ctx.obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_drone_hud_ctx.obj, LV_OBJ_FLAG_HIDDEN);
    }
}

// 创建透明容器用于承载 flex 或 grid 布局并避免默认样式干扰
static lv_obj_t *ui_drone_transparent_container_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

// 创建统一卡片并集中管理背景色边框圆角和内边距
static lv_obj_t *ui_drone_card_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, lv_coord_t pad) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xf9efcf), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, pad, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

// 创建状态胶囊和对应文本标签
static lv_obj_t *ui_drone_state_chip_create(lv_obj_t *parent, lv_obj_t **state_label) {
    lv_obj_t *state_chip = lv_obj_create(parent);
    lv_obj_set_size(state_chip, 150, 22);
    lv_obj_set_style_bg_color(state_chip, lv_color_hex(0xcdecd4), 0);
    lv_obj_set_style_bg_opa(state_chip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(state_chip, lv_color_hex(0x3c7a52), 0);
    lv_obj_set_style_border_width(state_chip, 1, 0);
    lv_obj_set_style_radius(state_chip, 16, 0);
    lv_obj_set_style_pad_all(state_chip, 0, 0);
    lv_obj_clear_flag(state_chip, LV_OBJ_FLAG_SCROLLABLE);

    *state_label = lv_label_create(state_chip);
    lv_obj_set_style_text_color(*state_label, lv_color_hex(0x175537), 0);
    lv_obj_center(*state_label);

    return state_chip;
}

// 创建基础信息项用于展示标题和值例如 Speed 与 Capacity
static void ui_drone_info_item_create(lv_obj_t *parent, const char *title, lv_obj_t **value_label, lv_coord_t w) {
    lv_obj_t *item = ui_drone_transparent_container_create(parent, w, LV_SIZE_CONTENT);
    lv_obj_set_layout(item, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(item, 2, 0);

    lv_obj_t *key = lv_label_create(item);
    lv_label_set_text(key, title);
    lv_obj_set_style_text_color(key, lv_color_hex(0x6f5c41), 0);

    *value_label = lv_label_create(item);
    lv_label_set_text(*value_label, "--");
}

// 创建模式按钮并绑定统一回调通过 desc 区分 Detect 或 Spray
static lv_obj_t *ui_drone_mode_button_create(lv_obj_t *parent, lv_coord_t w, const char *text, void *desc) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, 34);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xefcd76), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x8a6333), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 8, 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, text);
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(btn, ui_drone_mode_button_click_cb, LV_EVENT_CLICKED, desc);

    return btn;
}

// 创建一条图标名称数值统计项并使用 grid 保持列对齐
static void ui_drone_stat_entry_create(lv_obj_t *parent, lv_coord_t w, const void *icon_src, const char *name,
                                       lv_obj_t **name_label, lv_obj_t **value_label) {
    static lv_coord_t col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    lv_obj_t *entry = ui_drone_transparent_container_create(parent, w, LV_SIZE_CONTENT);
    lv_obj_set_layout(entry, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(entry, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(entry, 6, 0);

    lv_obj_t *icon = lv_img_create(entry);
    lv_img_set_src(icon, icon_src ? icon_src : &icon_pest_unknown);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    *name_label = lv_label_create(entry);
    lv_label_set_text(*name_label, name);
    lv_obj_set_grid_cell(*name_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    *value_label = lv_label_create(entry);
    lv_label_set_text(*value_label, "0");
    lv_obj_set_grid_cell(*value_label, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
}

// 创建模式控制行左侧为名称右侧为操作按钮
static void ui_drone_mode_row_create(lv_obj_t *parent, const char *name, lv_coord_t btn_w, lv_obj_t **btn_out,
                                     drone_mode_btn_desc_t *desc) {
    lv_obj_t *row = ui_drone_transparent_container_create(parent, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);

    *btn_out = ui_drone_mode_button_create(row, btn_w,
                                           desc == &g_drone_detect_btn_desc ? "Start Detect" : "Start Spray", desc);
}

// 创建农药装载行包含名称减号当前装载量加号并使用 grid 管理列对齐
static void ui_drone_pesticide_row_create(lv_obj_t *parent, crop_pesticide_t i, lv_obj_t **load_label) {
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
                                   LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    lv_obj_t *row = ui_drone_transparent_container_create(parent, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(row, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(row, 6, 0);

    lv_obj_t *name = lv_label_create(row);
    lv_label_set_text(name, crop_pesticide_name(i));
    lv_obj_set_grid_cell(name, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *minus_btn = lv_btn_create(row);
    lv_obj_set_size(minus_btn, 28, 28);
    lv_obj_set_style_bg_color(minus_btn, lv_color_hex(0xefcd76), 0);
    lv_obj_set_style_border_color(minus_btn, lv_color_hex(0x8a6333), 0);
    lv_obj_set_style_border_width(minus_btn, 1, 0);
    lv_obj_set_style_radius(minus_btn, 8, 0);
    lv_obj_set_grid_cell(minus_btn, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_t *minus_label = lv_label_create(minus_btn);
    lv_label_set_text(minus_label, "-");
    lv_obj_center(minus_label);

    *load_label = lv_label_create(row);
    lv_label_set_text(*load_label, "0");
    lv_obj_set_style_text_align(*load_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(*load_label, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *add_btn = lv_btn_create(row);
    lv_obj_set_size(add_btn, 28, 28);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0xf4cdca), 0);
    lv_obj_set_style_border_color(add_btn, lv_color_hex(0xb66258), 0);
    lv_obj_set_style_border_width(add_btn, 1, 0);
    lv_obj_set_style_radius(add_btn, 8, 0);
    lv_obj_set_grid_cell(add_btn, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_t *add_label = lv_label_create(add_btn);
    lv_label_set_text(add_label, "+");
    lv_obj_center(add_label);

    g_drone_pesticide_btn_desc[i][0] = (drone_pesticide_btn_desc_t){.pesticide = i, .delta = 1};
    g_drone_pesticide_btn_desc[i][1] = (drone_pesticide_btn_desc_t){.pesticide = i, .delta = -1};
    lv_obj_add_event_cb(add_btn, ui_drone_pesticide_button_click_cb, LV_EVENT_CLICKED,
                        &g_drone_pesticide_btn_desc[i][0]);
    lv_obj_add_event_cb(minus_btn, ui_drone_pesticide_button_click_cb, LV_EVENT_CLICKED,
                        &g_drone_pesticide_btn_desc[i][1]);
}

lv_obj_t *ui_drone_window_create(void) {
    drone_t *drone = drone_get_instance();

    // 窗口主体采用横向 flex：左侧信息区 + 右侧背包区。
    lv_obj_t *body = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_left(body, 4, 0);
    lv_obj_set_style_pad_right(body, 4, 0);
    lv_obj_set_style_pad_top(body, 8, 0);
    lv_obj_set_style_pad_bottom(body, 8, 0);
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(body, 10, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *div = ui_window_create(lv_scr_act(), "DRONE OPERATION", body, true);
    lv_obj_center(div);
    lv_obj_set_size(div, 714, 432);

    // 左右面板容器：仅负责分区，不承担视觉样式。
    lv_obj_t *left_panel = ui_drone_transparent_container_create(body, 382, 366);
    lv_obj_set_layout(left_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(left_panel, 4, 0);

    lv_obj_t *right_panel = ui_drone_transparent_container_create(body, 304, 366);
    lv_obj_set_layout(right_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Base Info 卡片：头部（标题+状态）+ 底部（速度/容量）。
    lv_obj_t *base_card = ui_drone_card_create(left_panel, 382, 96, 8);
    lv_obj_set_layout(base_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(base_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(base_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(base_card, 6, 0);

    lv_obj_t *base_header = ui_drone_transparent_container_create(base_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(base_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(base_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(base_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *base_title = lv_label_create(base_header);
    lv_label_set_text(base_title, "Base Info");
    lv_obj_set_style_text_color(base_title, lv_color_hex(0x5b421f), 0);

    ui_drone_state_chip_create(base_header, &g_drone_window_ctx.state_label);

    lv_obj_t *base_values = ui_drone_transparent_container_create(base_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(base_values, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(base_values, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(base_values, 10, 0);

    ui_drone_info_item_create(base_values, "Speed", &g_drone_window_ctx.speed_label, 154);
    ui_drone_info_item_create(base_values, "Capacity", &g_drone_window_ctx.storage_label, 154);

    // 虫害/装药统计卡片：按 2x2 行列生成，避免手写坐标。
    lv_obj_t *pest_card = ui_drone_card_create(left_panel, 382, 116, 8);
    lv_obj_set_layout(pest_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pest_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(pest_card, 6, 0);

    g_drone_window_ctx.pest_card_title = lv_label_create(pest_card);
    lv_label_set_text(g_drone_window_ctx.pest_card_title, "Last Scan Pest Count");
    lv_obj_set_style_text_color(g_drone_window_ctx.pest_card_title, lv_color_hex(0x5b421f), 0);

    lv_obj_t *pest_rows = ui_drone_transparent_container_create(pest_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(pest_rows, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pest_rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(pest_rows, 4, 0);

    for (crop_damage_t r = 0; r < 2; r++) {
        lv_obj_t *row = ui_drone_transparent_container_create(pest_rows, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        for (crop_damage_t c = 0; c < 2; c++) {
            crop_damage_t i = (crop_damage_t)(r * 2 + c);
            const void *pest_icon = icon_get_pest(i);
            ui_drone_stat_entry_create(row, 170, pest_icon, crop_pest_name(i),
                                       &g_drone_window_ctx.pest_row_name_labels[i],
                                       &g_drone_window_ctx.pest_result_labels[i]);
        }
    }

    // 模式控制卡片：两行（Detect/Recall、Auto Spray）。
    lv_obj_t *mode_card = ui_drone_card_create(left_panel, 382, 146, 8);
    lv_obj_set_layout(mode_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mode_card, 14, 0);

    lv_obj_t *mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Mode Control");
    lv_obj_set_style_text_color(mode_title, lv_color_hex(0x5b421f), 0);
    ui_drone_mode_row_create(mode_card, "Detect / Recall", 132, &g_drone_window_ctx.detect_btn,
                             &g_drone_detect_btn_desc);
    ui_drone_mode_row_create(mode_card, "Auto Spray", 132, &g_drone_window_ctx.spray_btn, &g_drone_spray_btn_desc);

    // 右侧背包卡片：上半为装载调节列表，下半为背包 grid list。
    lv_obj_t *bag_card = ui_drone_card_create(right_panel, 304, 366, 8);
    lv_obj_set_layout(bag_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bag_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(bag_card, 6, 0);

    lv_obj_t *bag_title = lv_label_create(bag_card);
    lv_label_set_text(bag_title, "Pesticide Load (+/-)");
    lv_obj_set_style_text_color(bag_title, lv_color_hex(0x5b421f), 0);

    for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
        ui_drone_pesticide_row_create(bag_card, i, &g_drone_window_ctx.pesticide_load_labels[i]);
    }

    lv_obj_t *bag_subtitle = lv_label_create(bag_card);
    lv_label_set_text(bag_subtitle, "Backpack");
    lv_obj_set_style_text_color(bag_subtitle, lv_color_hex(0x5b421f), 0);

    ui_grid_list_cfg_t bag_list_cfg;
    ui_grid_list_cfg_init(&bag_list_cfg);
    bag_list_cfg.item_w = 272;
    bag_list_cfg.item_h = 30;
    bag_list_cfg.col_count = 1;
    bag_list_cfg.row_count = CROP_PESTICIDE_NONE;
    bag_list_cfg.pad_col = 0;
    bag_list_cfg.pad_row = 4;
    bag_list_cfg.pad_all = 0;

    g_drone_window_ctx.pesticide_bag_list = ui_grid_list_create(bag_card, &bag_list_cfg);
    if (g_drone_window_ctx.pesticide_bag_list) {
        // 让 grid list 跟随容器布局居中，而不是固定绝对坐标。
        lv_obj_t *bag_list_obj = ui_grid_list_get_obj(g_drone_window_ctx.pesticide_bag_list);
        lv_obj_set_width(bag_list_obj, 272);
        lv_obj_set_style_align(bag_list_obj, LV_ALIGN_CENTER, 0);

        for (crop_pesticide_t i = 0; i < CROP_PESTICIDE_NONE; i++) {
            lv_obj_t *item = ui_grid_list_add_item(g_drone_window_ctx.pesticide_bag_list);
            if (!item) {
                break;
            }

            lv_obj_set_style_pad_all(item, 0, 0);
            lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *label = lv_label_create(item);
            lv_label_set_text_fmt(label, "%s: 0", crop_pesticide_name(i));
            lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);
            g_drone_window_ctx.pesticide_bag_labels[i] = label;
        }
    }

    g_drone_window_ctx.obj = div;
    ui_drone_window_refresh();

    (void)drone;
    return div;
}

void ui_drone_hud_create(lv_obj_t *parent) {
    if (g_drone_hud_ctx.obj && lv_obj_is_valid(g_drone_hud_ctx.obj)) {
        return;
    }

    // HUD 根节点采用横向 flex，将左右悬浮信息区分离。
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_left(root, 12, 0);
    lv_obj_set_style_pad_right(root, 12, 0);
    lv_obj_set_style_pad_top(root, 0, 0);
    lv_obj_set_style_pad_bottom(root, 0, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);

    // 左侧 HUD：基础信息 + 双统计信息。
    lv_obj_t *left_panel = ui_drone_transparent_container_create(root, 248, 320);
    lv_obj_set_style_bg_color(left_panel, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(left_panel, LV_OPA_COVER, 0);
    lv_obj_set_layout(left_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left_panel, 4, 0);

    // 右侧 HUD：模式控制。
    lv_obj_t *right_panel = ui_drone_transparent_container_create(root, 214, 178);
    lv_obj_set_style_bg_color(right_panel, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(right_panel, LV_OPA_COVER, 0);
    lv_obj_set_layout(right_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);

    // HUD 基础信息卡片，结构与窗口保持一致，便于统一维护。
    lv_obj_t *base_card = ui_drone_card_create(left_panel, 248, 96, 8);
    lv_obj_set_layout(base_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(base_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(base_card, 6, 0);

    lv_obj_t *base_header = ui_drone_transparent_container_create(base_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(base_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(base_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(base_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *base_title = lv_label_create(base_header);
    lv_label_set_text(base_title, "Base Info");
    lv_obj_set_style_text_color(base_title, lv_color_hex(0x5b421f), 0);

    ui_drone_state_chip_create(base_header, &g_drone_hud_ctx.state_label);

    lv_obj_t *base_values = ui_drone_transparent_container_create(base_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(base_values, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(base_values, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(base_values, 8, 0);

    ui_drone_info_item_create(base_values, "Speed", &g_drone_hud_ctx.speed_label, 74);
    ui_drone_info_item_create(base_values, "Capacity", &g_drone_hud_ctx.storage_label, 100);

    // HUD 虫害统计卡片。
    lv_obj_t *pest_card = ui_drone_card_create(left_panel, 248, 106, 8);
    lv_obj_set_layout(pest_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pest_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(pest_card, 4, 0);

    g_drone_hud_ctx.pest_card_title = lv_label_create(pest_card);
    lv_label_set_text(g_drone_hud_ctx.pest_card_title, "Last Scan Pest Count");
    lv_obj_set_style_text_color(g_drone_hud_ctx.pest_card_title, lv_color_hex(0x5b421f), 0);
    lv_obj_t *pest_rows = ui_drone_transparent_container_create(pest_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(pest_rows, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pest_rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(pest_rows, 2, 0);

    for (crop_damage_t r = 0; r < 2; r++) {
        lv_obj_t *row = ui_drone_transparent_container_create(pest_rows, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        for (crop_damage_t c = 0; c < 2; c++) {
            crop_damage_t i = (crop_damage_t)(r * 2 + c);
            const void *pest_icon = icon_get_pest(i);
            ui_drone_stat_entry_create(row, 110, pest_icon, crop_pest_name(i), &g_drone_hud_ctx.pest_row_name_labels[i],
                                       &g_drone_hud_ctx.pest_result_labels[i]);
        }
    }

    // HUD 装药统计卡片，始终展示无人机当前装药量。
    lv_obj_t *loaded_card = ui_drone_card_create(left_panel, 248, 106, 8);
    lv_obj_set_layout(loaded_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(loaded_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(loaded_card, 4, 0);

    g_drone_hud_ctx.pesticide_card_title = lv_label_create(loaded_card);
    lv_label_set_text(g_drone_hud_ctx.pesticide_card_title, "Loaded Pesticide Count");
    lv_obj_set_style_text_color(g_drone_hud_ctx.pesticide_card_title, lv_color_hex(0x5b421f), 0);

    lv_obj_t *loaded_rows = ui_drone_transparent_container_create(loaded_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(loaded_rows, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(loaded_rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(loaded_rows, 2, 0);

    for (crop_pesticide_t r = 0; r < 2; r++) {
        lv_obj_t *row = ui_drone_transparent_container_create(loaded_rows, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        for (crop_pesticide_t c = 0; c < 2; c++) {
            crop_pesticide_t i = (crop_pesticide_t)(r * 2 + c);
            const void *pest_icon = icon_get_pest((crop_damage_t)i);
            ui_drone_stat_entry_create(row, 110, pest_icon, crop_pesticide_name(i),
                                       &g_drone_hud_ctx.pesticide_row_name_labels[i],
                                       &g_drone_hud_ctx.pesticide_result_labels[i]);
        }
    }

    // HUD 模式卡片：复用统一模式行构建函数。
    lv_obj_t *mode_card = ui_drone_card_create(right_panel, 214, 178, 8);
    lv_obj_set_layout(mode_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mode_card, 16, 0);

    lv_obj_t *mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Mode Control");
    lv_obj_set_style_text_color(mode_title, lv_color_hex(0x5b421f), 0);

    ui_drone_mode_row_create(mode_card, "Detect / Recall", 126, &g_drone_hud_ctx.detect_btn, &g_drone_detect_btn_desc);
    ui_drone_mode_row_create(mode_card, "Auto Spray", 126, &g_drone_hud_ctx.spray_btn, &g_drone_spray_btn_desc);

    g_drone_hud_ctx.obj = root;
    ui_drone_panel_refresh(&g_drone_hud_ctx);
}
