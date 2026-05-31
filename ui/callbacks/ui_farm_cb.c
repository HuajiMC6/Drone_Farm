#include "ui_farm_cb.h"

#include "audio.h"
#include "player.h"
#include "ui_common.h"
#include "ui_farm.h"
#include "ui_message.h"
#include "ui_window.h"

/* ── 内部工具 ── */

static bool obj_overlap(lv_obj_t *obj1, lv_obj_t *obj2, lv_coord_t hor_offset, lv_coord_t ver_offset) {
    lv_area_t a1, a2;
    lv_obj_get_coords(obj1, &a1);
    lv_obj_get_coords(obj2, &a2);

    if (a1.x2 < a2.x1 + hor_offset || a1.x1 > a2.x2 - hor_offset || a1.y2 < a2.y1 + ver_offset ||
        a1.y1 > a2.y2 - ver_offset) {
        return false;
    }

    return true;
}

/* 长按标志位，避免长按后触发click事件 */
static bool g_field_long_pressed = false;

/* ── 地块交互 ── */

void ui_farm_field_block_click_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    farm_block_t *info = lv_obj_get_user_data(btn);

    if (g_field_long_pressed) {
        g_field_long_pressed = false;
        return;
    }

    static uint32_t last_click_tick = 0;
    static farm_block_t *last_clicked_block = NULL;
    uint32_t current_tick = lv_tick_get();

    if (current_tick - last_click_tick < 250 && info == last_clicked_block) {
        last_click_tick = 0;

        if (info->is_planted) {
            int output;
            bool result = player_harvest(info->field, &output);

            if (result) {
                char message[64];
                snprintf(message, sizeof(message), "Harvest successful! You got %d units.", output);
                ui_message_show(message, UI_MESSAGE_TYPE_SUCCESS, UI_MESSAGE_TOAST);

                harvest_audio_play();
            } else {
                ui_message_show("The crop has NOT been ripe!", UI_MESSAGE_TYPE_ERROR, UI_MESSAGE_TOAST);
            }

            lv_obj_add_state(btn, LV_STATE_CHECKED);
        }
    } else {
        last_click_tick = current_tick;
        last_clicked_block = info;

        if (!lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            return;
        }

        lv_obj_t *parent = lv_obj_get_parent(btn);
        ui_farm_clear_field_selection(parent);
        lv_obj_add_state(btn, LV_STATE_CHECKED);

        ui_field_upgrade_window_switch(info);
    }
}

void ui_farm_field_block_long_press_cb(lv_event_t *e) {
    g_field_long_pressed = true;

    lv_obj_t *btn = lv_event_get_current_target(e);
    farm_block_t *block = lv_obj_get_user_data(btn);
    if (block) {
        ui_field_upgrade_window_refresh(block);

        ui_farm_clear_field_selection(lv_obj_get_parent(btn));
    }
}

/* ── 地块升级 ── */

void ui_farm_output_upgrade_click_cb(lv_event_t *e) {
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

void ui_farm_ready_time_upgrade_click_cb(lv_event_t *e) {
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

void ui_farm_tolerance_upgrade_click_cb(lv_event_t *e) {
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

/* ── 拖拽种植 ── */

void ui_farm_seed_drag_event_cb(lv_event_t *e) {
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
                img = lv_img_create(lv_layer_top());

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
                if (obj_overlap(img, child, 40, 40)) {
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

/* ── 进度条绘制 ── */

void ui_farm_crop_bar_draw_part_end_cb(lv_event_t *e) {
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

/* ── 一键收获 ── */

void ui_farm_harvest_all_click_cb(lv_event_t *e) {
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

    harvest_audio_play();
}
