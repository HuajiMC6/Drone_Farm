#include "ui_event.h"
// #include "ui.h"
#include "ui_common.h"
#include "ui_window.h"

static bool ui_lv_obj_is_overlap(lv_obj_t *obj1, lv_obj_t *obj2, lv_coord_t hor_offset, lv_coord_t ver_offset);

static void ui_window_toggle_from_desc(ui_window_toggle_desc_t *desc) {
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

void farm_block_click_cb(lv_event_t *e) {
    /* 处理地块高亮 */
    lv_obj_t *btn = lv_event_get_target(e);
    if (!lv_obj_has_state(btn, LV_STATE_CHECKED))
        return;

    lv_obj_t *parent = lv_obj_get_parent(btn);
    lv_obj_t *child;
    uint8_t idx = 0;
    while ((child = lv_obj_get_child(parent, idx++)) != NULL) {
        if (child != btn && lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE)) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }

    /* 种植弹窗 */
    farm_block_t *info = lv_obj_get_user_data(btn);
    lv_obj_t *scr = lv_event_get_user_data(e);
    if (info->is_planted) {
        // TO DO...
    }
}

void screen_main_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);

    lv_obj_t *obj = lv_event_get_user_data(e);
    if (obj == NULL) {
        return;
    }

    // 如果点击的目标是父容器或其内部子对象，则不处理
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

void main_floating_btn_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_window_toggle_desc_t *desc = lv_event_get_user_data(e);
    ui_window_toggle_from_desc(desc);
}

void drag_to_plant_cb(lv_event_t *e) {
    lv_obj_t *cell = lv_event_get_current_target(e);
    ui_drag_to_plant_desc_t *desc = lv_event_get_user_data(e);

    static crop_type_t type = CROP_TYPE_NONE;
    static lv_obj_t *img;

    /* 当前悬停的田地对象 */
    static lv_obj_t *current_target = NULL;

    switch (lv_event_get_code(e)) {
        case LV_EVENT_PRESSING:       // 拖动
            if (type != desc->type) { // 说明我拖动新的种子了，否则就还是原来那个种子，就不需要重新创建图片
                type = desc->type;
                img = lv_img_create(lv_scr_act());

                lv_point_t point;
                lv_indev_get_point(lv_event_get_indev(e), &point);

                lv_img_set_src(img, desc->img);
                lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
                lv_obj_update_layout(img); // 使下面lv_obj_get_width/height能获取到正确数值
                lv_obj_set_pos(img, point.x - lv_obj_get_width(img) / 2,
                               point.y - lv_obj_get_height(img) / 2); // 保证图片中心位于手指正下方
            }

            /* 实时更新图片位置，随手指移动 */
            lv_point_t vect;
            lv_indev_get_vect(lv_indev_get_act(), &vect);
            lv_coord_t x = lv_obj_get_x_aligned(img);
            lv_coord_t y = lv_obj_get_y_aligned(img);
            lv_obj_set_pos(img, x + vect.x, y + vect.y);

            /* 经过农田时高亮 */
            uint8_t i;
            lv_obj_t *child = NULL;
            lv_obj_t *collision_target = NULL;
            for (i = 0; i < lv_obj_get_child_cnt(desc->fields); i++) {
                child = lv_obj_get_child(desc->fields, i);

                /* 检测矩形碰撞 */
                if (ui_lv_obj_is_overlap(img, child, 40, 40)) {
                    collision_target = child;
                    break; // 找到第一个碰撞目标，跳出循环
                }
            }

            if (collision_target != current_target) { // 找到新目标了
                // 恢复上一个目标的样式
                if (current_target) {
                    lv_obj_clear_state(current_target, LV_STATE_CHECKED);
                }
                // 高亮新的目标
                current_target = collision_target;
                if (current_target) {
                    if (!((farm_block_t *)lv_obj_get_user_data(current_target))->is_planted) // 已种植区域不高亮
                        lv_obj_add_state(current_target, LV_STATE_CHECKED);
                }
            }

            break;
        case LV_EVENT_RELEASED: // 松手
            /* 种植 */
            if (current_target) {
                farm_block_t *block_data = lv_obj_get_user_data(current_target);
                if (!block_data->is_planted)
                    field_plant(block_data->field, type); // 种植
            }

            /* 删除图片对象，清空静态变量 */
            lv_obj_del(img);
            img = NULL;
            type = CROP_TYPE_NONE;
            break;
        default:
            break;
    }
}

void drone_click_cb(lv_event_t *e) {
    static bool in_drone_click = false;
    if (in_drone_click) {
        lv_event_stop_processing(e);
        return;
    }

    in_drone_click = true;

    /* Prevent parent screen click handler from deleting/altering window in the same click event */
    lv_event_stop_bubbling(e);

    ui_window_toggle_desc_t *desc = lv_event_get_user_data(e);
    ui_window_toggle_from_desc(desc);

    in_drone_click = false;
}

void crop_growing_bar_event(lv_event_t *e) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    if (dsc->part != LV_PART_INDICATOR)
        return;

    lv_obj_t *obj = lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = &lv_font_montserrat_10;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX,
                    label_dsc.flag);

    lv_area_t txt_area;
    /*If the indicator is long enough put the text inside on the right*/
    if (lv_area_get_width(dsc->draw_area) > txt_size.x + 10) {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white();
    }
    /*If the indicator is still short put the text out of it on the right*/
    else {
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black();
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

static bool ui_lv_obj_is_overlap(lv_obj_t *obj1, lv_obj_t *obj2, lv_coord_t hor_offset, lv_coord_t ver_offset) {
    lv_area_t a1, a2;

    // 获取对象全局坐标
    lv_obj_get_coords(obj1, &a1);
    lv_obj_get_coords(obj2, &a2);

    // 检查是否不重叠
    if (a1.x2 < a2.x1 + hor_offset || a1.x1 > a2.x2 - hor_offset || // 横向无重叠
        a1.y2 < a2.y1 + ver_offset || a1.y1 > a2.y2 - ver_offset) { // 纵向无重叠
        return false;                                               // 不重叠
    }

    return true; // 重叠
}
