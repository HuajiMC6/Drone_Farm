#include "ui_main_cb.h"

#include "audio.h"
#include "player.h"
#include "ui_common.h"
#include "ui_farm.h"
#include "ui_message.h"
#include "ui_window.h"

static bool ui_main_obj_overlap(lv_obj_t *obj1, lv_obj_t *obj2, lv_coord_t hor_offset, lv_coord_t ver_offset);
static void ui_main_toggle_window_from_desc(ui_window_toggle_desc_t *desc);

void debug_heartbeat_timer_set_period(uint32_t period_ms);

void ui_reset(void);

/* 长按标志位，避免长按后触发click事件 */
static bool ui_farm_field_long_pressed = false;

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

static void ui_farm_fields_clear_checked_state(lv_obj_t *parent) {
    lv_obj_t *child;
    uint8_t idx = 0;
    while ((child = lv_obj_get_child(parent, idx++)) != NULL) {
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE)) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }
}

void ui_main_field_block_click_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    farm_block_t *info = lv_obj_get_user_data(btn);

    // 避免长按后触发click事件
    if (ui_farm_field_long_pressed) {
        ui_farm_field_long_pressed = false;
        return;
    }

    static uint32_t last_click_tick = 0;
    static farm_block_t *last_clicked_block = NULL;
    uint32_t current_tick = lv_tick_get();

    // 检测双击or单击
    if (current_tick - last_click_tick < 250 && info == last_clicked_block) { // 双击同一块土地，触发收获判定
        last_click_tick = 0;

        if (info->is_planted) {
            int output;
            bool result = player_harvest(info->field, &output);

            if (result) {
                char message[64];
                snprintf(message, sizeof(message), "Harvest successful! You got %d units.", output);
                ui_message_show(message, UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
            } else {
                ui_message_show("The crop has NOT been ripe!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
            }

            lv_obj_add_state(btn, LV_STATE_CHECKED); // 收获保持选中状态
        }
    } else { // 单击，触发选中状态切换
        last_click_tick = current_tick;
        last_clicked_block = info;

        // 状态切换先于事件发生，故这里获取到的是切换后的状态
        if (!lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            return; // 即当前是未被选中状态，也就是当前事件是取消选中，不需要执行后面清除其他田地选中状态的操作
        }

        // 反之如果是选中事件，先清除其他田地的选中状态，再设置当前田地为选中状态
        lv_obj_t *parent = lv_obj_get_parent(btn);
        ui_farm_fields_clear_checked_state(parent);
        lv_obj_add_state(btn, LV_STATE_CHECKED);

        // 当窗口显示时点击其他田地切换田地信息窗口
        ui_field_upgrade_window_switch(info);
    }
}

void ui_main_field_block_long_press_cb(lv_event_t *e) {
    ui_farm_field_long_pressed = true;

    lv_obj_t *btn = lv_event_get_current_target(e);
    farm_block_t *block = lv_obj_get_user_data(btn);
    if (block) {
        ui_field_upgrade_window_refresh(block);

        // 长按后保持当前田地选中状态，清除其他田地的选中状态
        ui_farm_fields_clear_checked_state(lv_obj_get_parent(btn));
    }
}

void ui_main_filed_output_upgrade_click_cb(lv_event_t *e) {
    field_t **field = lv_event_get_user_data(e);
    if (!field || !*field) {
        return;
    }

    bool result = player_buy_field_output_upgrade(*field);
    if (!result) {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
    } else {
        ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    }
}

void ui_main_ready_time_upgrade_click_cb(lv_event_t *e) {
    field_t **field = lv_event_get_user_data(e);
    if (!field || !*field) {
        return;
    }

    bool result = player_buy_field_ready_time_upgrade(*field);
    if (!result) {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
    } else {
        ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    }
}

void ui_main_tolerance_upgrade_click_cb(lv_event_t *e) {
    field_t **field = lv_event_get_user_data(e);
    if (!field || !*field) {
        return;
    }

    bool result = player_buy_field_tolerance_upgrade(*field);
    if (!result) {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
    } else {
        ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
    }
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
        ui_farm_fields_clear_checked_state(obj);
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
                img = lv_img_create(lv_layer_top()); // 以lv_layer_top()为父对象，保证拖动时定位不偏移

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

// 弹窗升级按钮回调
void ui_main_farm_upgrade_btn_click_cb(lv_event_t *e) {
    bool result = player_buy_farm_size_update();
    if (result) {
        ui_message_show("Upgrade successful!", UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
        // 升级成功后关闭弹窗
        ui_window_hide_current();
    } else {
        ui_message_show("Not enough gold!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
    }
}

// 一键收获
void ui_main_harvest_all_click_cb(lv_event_t *e) {
    int outputs[CROP_TYPE_NONE] = {0};
    bool result = player_harvest_all(outputs);

    if (!result) {
        ui_message_show("No ripe crops to harvest!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
        return;
    }

    char message[128] = "Harvest successful! You got ";
    for (crop_type_t i = 0; i < CROP_TYPE_NONE; i++) {
        if (outputs[i] > 0) {
            char crop_message[32];
            snprintf(crop_message, sizeof(crop_message), "%s x%d", crop_type_name(i), outputs[i]);
            strncat(message, crop_message, sizeof(message) - strlen(message) - 1);
            if (i < CROP_TYPE_NONE - 1) {
                strncat(message, ", ", sizeof(message) - strlen(message) - 1);
            }
        }
    }
    ui_message_show(message, UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);
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

void ui_setting_add_level_cb(lv_event_t *e) {
    player_t *player = player_get_instance();
    if (!player || player->level >= PLAYER_EXPERIENCE_LEVELS - 1)
        return;

    /* 经验拉到当前等级阈值，触发升级 */
    player->experience = experience_level[player->level];
    player->level++;

    /* 更新等级段 */
    player->level_stage = PLAYER_LEVEL_STAGE_THRESHOLD_COUNT;
    for (int i = 0; i < PLAYER_LEVEL_STAGE_THRESHOLD_COUNT; i++) {
        if (player->level < player_level_stage_thresholds[i]) {
            player->level_stage = i;
            break;
        }
    }
}

void ui_setting_game_speed_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t **btns = (lv_obj_t **)lv_event_get_user_data(e);

    /* 通过 target 反查按钮索引 */
    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (btns[i] == target) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return;

    /* 倍率 → 心跳周期映射: 0.5x→2000ms, 1x→1000ms, 2x→500ms, 5x→200ms */
    static const uint32_t periods[] = {2000, 1000, 500, 200};
    debug_heartbeat_timer_set_period(periods[idx]);

    /* 更新按钮高亮 */
    for (int i = 0; i < 4; i++) {
        if (!btns[i])
            continue;
        if (i == idx) {
            lv_obj_add_style(btns[i], &ui_style_btn_yellow, 0);
        } else {
            lv_obj_remove_style(btns[i], &ui_style_btn_yellow, 0);
        }
    }
}

void debug_volume_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_current_target(e);
    lv_obj_t *label = lv_event_get_user_data(e);

    int value = lv_slider_get_value(slider);
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d%%", value);
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
