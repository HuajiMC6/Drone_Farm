#include "ui_main_cb.h"

#include "audio.h"
#include "player.h"
#include "ui_common.h"
#include "ui_window.h"

static bool ui_main_obj_overlap(lv_obj_t *obj1, lv_obj_t *obj2, lv_coord_t hor_offset, lv_coord_t ver_offset);
static void ui_main_toggle_window_from_desc(ui_window_toggle_desc_t *desc);

void ui_reset(void);

static void ui_main_toggle_window_from_desc(ui_window_toggle_desc_t *desc) {
    if (!desc || !desc->create) {
        return;
    }

    lv_obj_t *window = NULL;
    if (desc->window_ref) {
        window = *desc->window_ref;
        if (window && !lv_obj_is_valid(window)) {
            *desc->window_ref = NULL;
            window = NULL;
        }
    }

    if (!window) {
        window = desc->create();
        if (desc->window_ref) {
            *desc->window_ref = window;
        }
        return;
    }

    if (ui_window_is_visible(window)) {
        ui_window_hide(window);
    } else {
        ui_window_show(window);
    }
}

void ui_main_field_block_click_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    farm_block_t *info = lv_obj_get_user_data(btn);

    static uint32_t last_click_tick = 0;
    uint32_t current_tick = lv_tick_get();

    // 检测双击or单击
    if (current_tick - last_click_tick < 250) { // 双击，触发收获判定
        last_click_tick = 0;

        if (info->is_planted) {
            player_harvest(info->field);
            lv_obj_add_state(btn, LV_STATE_CHECKED); // 收获保持选中状态
        }
    } else { // 单击，触发选中状态切换
        last_click_tick = current_tick;

        if (!lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            return;
        }

        lv_obj_t *parent = lv_obj_get_parent(btn);
        lv_obj_t *child;
        uint8_t idx = 0;
        while ((child = lv_obj_get_child(parent, idx++)) != NULL) {
            if (child != btn && lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE)) {
                lv_obj_clear_state(child, LV_STATE_CHECKED);
            }
        }
    }
}

void ui_main_field_block_long_press_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    farm_block_t *block = lv_obj_get_user_data(btn);
    void (*window_show)(void *) = lv_event_get_user_data(e);
    if (block) {
        window_show(block);
    }
}

void ui_main_filed_output_upgrade_click_cb(lv_event_t *e) {
    field_t **field = lv_event_get_user_data(e);
    if (!field || !*field) {
        return;
    }

    player_buy_field_output_upgrade(*field);
}

void ui_main_ready_time_upgrade_click_cb(lv_event_t *e) {
    field_t **field = lv_event_get_user_data(e);
    if (!field || !*field) {
        return;
    }

    player_buy_field_ready_time_upgrade(*field);
}

void ui_main_tolerance_upgrade_click_cb(lv_event_t *e) {
    field_t **field = lv_event_get_user_data(e);
    if (!field || !*field) {
        return;
    }

    player_buy_field_tolerance_upgrade(*field);
}

void ui_main_screen_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (target == obj || lv_obj_get_parent(target) == obj) {
        return;
    }

    lv_obj_t *current_window = ui_window_get_current();
    if (current_window) {
        ui_window_hide_current();
    } else {
        lv_obj_t *child;
        uint8_t idx = 0;
        while ((child = lv_obj_get_child(obj, idx++)) != NULL) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }
}

void ui_main_floating_button_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_window_toggle_desc_t *desc = lv_event_get_user_data(e);
    ui_main_toggle_window_from_desc(desc);
}

void ui_main_seed_drag_event_cb(lv_event_t *e) {
    ui_drag_to_plant_desc_t *desc = lv_event_get_user_data(e);
    if (!desc) {
        return;
    }

    static crop_type_t type = CROP_TYPE_NONE;
    static lv_obj_t *img = NULL;
    static lv_obj_t *current_target = NULL;

    switch (lv_event_get_code(e)) {
        case LV_EVENT_PRESSING:
            if (type != desc->type) {
                type = desc->type;
                img = lv_img_create(lv_scr_act());

                lv_point_t point;
                lv_indev_get_point(lv_event_get_indev(e), &point);

                lv_img_set_src(img, desc->img);
                lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
                lv_obj_update_layout(img);
                lv_obj_set_pos(img, point.x - lv_obj_get_width(img) / 2, point.y - lv_obj_get_height(img) / 2);
            }

            lv_point_t vect;
            lv_indev_get_vect(lv_indev_get_act(), &vect);
            lv_coord_t x = lv_obj_get_x_aligned(img);
            lv_coord_t y = lv_obj_get_y_aligned(img);
            lv_obj_set_pos(img, x + vect.x, y + vect.y);

            lv_obj_t *collision_target = NULL;
            for (uint8_t i = 0; i < lv_obj_get_child_cnt(desc->fields); i++) {
                lv_obj_t *child = lv_obj_get_child(desc->fields, i);
                if (ui_main_obj_overlap(img, child, 40, 40)) {
                    collision_target = child;
                    break;
                }
            }

            if (collision_target != current_target) {
                if (current_target) {
                    lv_obj_clear_state(current_target, LV_STATE_CHECKED);
                }
                current_target = collision_target;
                if (current_target) {
                    farm_block_t *target_block = lv_obj_get_user_data(current_target);
                    if (target_block && !target_block->is_planted) {
                        lv_obj_add_state(current_target, LV_STATE_CHECKED);
                    }
                }
            }
            break;

        case LV_EVENT_RELEASED:
            if (current_target) {
                farm_block_t *block_data = lv_obj_get_user_data(current_target);
                if (block_data && !block_data->is_planted) {
                    player_plant(block_data->field, type);
                }
                lv_obj_clear_state(current_target, LV_STATE_CHECKED);
            }

            if (img && lv_obj_is_valid(img)) {
                lv_obj_del(img);
            }
            img = NULL;
            type = CROP_TYPE_NONE;
            current_target = NULL;
            break;

        default:
            break;
    }
}

void ui_main_drone_click_cb(lv_event_t *e) {
    static bool in_drone_click = false;
    if (in_drone_click) {
        lv_event_stop_processing(e);
        return;
    }

    in_drone_click = true;
    lv_event_stop_bubbling(e);

    ui_window_toggle_desc_t *desc = lv_event_get_user_data(e);
    ui_main_toggle_window_from_desc(desc);

    in_drone_click = false;
}

void ui_main_crop_bar_draw_part_end_cb(lv_event_t *e) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    if (dsc->part != LV_PART_INDICATOR) {
        return;
    }

    lv_obj_t *obj = lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = &lv_font_montserrat_8;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX,
                    label_dsc.flag);

    lv_area_t txt_area;
    if (lv_area_get_width(dsc->draw_area) > txt_size.x + 10) {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white();
    } else {
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black();
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

// for debug ---

void ui_setting_reset_game_cb(lv_event_t *e) {
    farm_delete();
    drone_delete();
    player_delete();
}

void ui_setting_add_coins_cb(lv_event_t *e) {
    player_t *player = player_get_instance();
    if (player) {
        player->coins += 100000;
    }
}

void debug_heartbear_timer_set_period(uint32_t period_ms);

void debug_timer_period_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_current_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);

    int value = lv_slider_get_value(slider);
    char buf[16];
    int ms = 19 * value + 50;
    lv_snprintf(buf, sizeof(buf), "%dms", ms);
    lv_label_set_text(label, buf);

    debug_heartbear_timer_set_period(ms);
}

void debug_screenshot_cb(lv_event_t *e) {
    ui_window_hide_current(); // 截图前隐藏当前窗口，保证截图界面干净
}

void debug_volume_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_current_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);

    int value = lv_slider_get_value(slider);
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "Volume: %d%%", value);
    lv_label_set_text(label, buf);

    audio_set_volume(value);
}
// for debug ---

static bool ui_main_obj_overlap(lv_obj_t *obj1, lv_obj_t *obj2, lv_coord_t hor_offset, lv_coord_t ver_offset) {
    lv_area_t a1, a2;
    lv_obj_get_coords(obj1, &a1);
    lv_obj_get_coords(obj2, &a2);

    if (a1.x2 < a2.x1 + hor_offset || a1.x1 > a2.x2 - hor_offset || a1.y2 < a2.y1 + ver_offset ||
        a1.y1 > a2.y2 - ver_offset) {
        return false;
    }

    return true;
}
