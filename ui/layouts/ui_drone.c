#include "ui_common.h"
#include "ui_drone_cb.h"
#include "ui_window.h"

#include "drone.h"
#include "icon.h"

static lv_obj_t *g_drone_window = NULL;

typedef struct {
    lv_obj_t *obj;
    lv_obj_t *speed_label;
    lv_obj_t *algorithm_label;
    lv_obj_t *storage_label;
    lv_obj_t *result_labels[CROP_DAMAGE_NONE];
    lv_obj_t *detect_btn;
    lv_obj_t *spray_btn;
    lv_obj_t *pesticide_num_labels[CROP_PESTICIDE_NONE];
    lv_obj_t *state_label;
} drone_window_ctx_t;

static drone_window_ctx_t g_drone_window_ctx;
static drone_pesticide_btn_desc_t g_drone_pesticide_btn_desc[CROP_PESTICIDE_NONE][2];
static drone_mode_btn_desc_t g_drone_detect_btn_desc = {.target_state = DRONE_STATE_DETECTING};
static drone_mode_btn_desc_t g_drone_spray_btn_desc = {.target_state = DRONE_STATE_AUTO};

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

void ui_drone_window_refresh(void) {
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
            lv_label_set_text_fmt(g_drone_window_ctx.result_labels[i], "%d", ui_drone_pest_count[i]);
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

lv_obj_t *ui_drone_window_create(void) {
    drone_t *drone = drone_get_instance();

    lv_obj_t *body = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf6dc8f), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *div = ui_window_create(lv_scr_act(), "DRONE OPERATION", body, true);
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
    lv_obj_add_event_cb(g_drone_window_ctx.detect_btn, ui_drone_mode_button_click_cb, LV_EVENT_CLICKED,
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
    lv_obj_add_event_cb(g_drone_window_ctx.spray_btn, ui_drone_mode_button_click_cb, LV_EVENT_CLICKED,
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
        lv_obj_add_event_cb(add_btn, ui_drone_pesticide_button_click_cb, LV_EVENT_CLICKED,
                            &g_drone_pesticide_btn_desc[i][0]);
        lv_obj_add_event_cb(minus_btn, ui_drone_pesticide_button_click_cb, LV_EVENT_CLICKED,
                            &g_drone_pesticide_btn_desc[i][1]);
    }

    g_drone_window_ctx.obj = div;
    ui_drone_window_refresh();

    (void)drone;
    return div;
}
