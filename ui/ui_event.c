#include "ui_event.h"

void farm_block_click_cb(lv_event_t *e) {
	lv_obj_t *btn = lv_event_get_target(e);
    if (!lv_obj_has_state(btn, LV_STATE_CHECKED)) return;

    lv_obj_t *parent = lv_obj_get_parent(btn);
    lv_obj_t *child;
    uint8_t idx = 0;
    while ((child = lv_obj_get_child(parent, idx++)) != NULL) {
        if (child != btn && lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE)) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }
}

void screen_main_click_cb(lv_event_t *e) {
	lv_obj_t *target = lv_event_get_target(e);
	lv_obj_t *farm_field = lv_event_get_user_data(e);

    // 如果点击的目标是父容器或其内部子对象，则不处理
    if (target == farm_field || lv_obj_get_parent(target) == farm_field) {
        return;
    }

    // 点击外部，清除所有选中状态
    lv_obj_t *child;
    uint8_t idx = 0;
    while ((child = lv_obj_get_child(farm_field, idx++)) != NULL) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
    }
}

void plant_btn_click_cb(lv_event_t *e) {
	static lv_obj_t *window;
	if(!window) {
		window = ((lv_obj_t * (*)(void))lv_event_get_user_data(e))();
	} else {
		lv_obj_del(window);
		window = NULL;
	}
}

void drag_to_plant_cb(lv_event_t *e) {
	lv_obj_t *cell = lv_event_get_current_target(e);
	ui_drag_to_plant_desc_t *desc = lv_event_get_user_data(e);
	
	static crop_type_t type = CROP_TYPE_NONE;
	static lv_obj_t *img;
	
	switch(lv_event_get_code(e)) {
        case LV_EVENT_PRESSING:
            if(type != desc->type) {
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
            break;
        case LV_EVENT_RELEASED:
            lv_obj_del(img);
			img = NULL;
			type = CROP_TYPE_NONE;
            break;
		default:
			break;
    }
	
	
	
	
}