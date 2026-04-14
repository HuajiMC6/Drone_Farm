#include "ui.h"
#include "ui_event.h"
#include "icon.h"
#include "sdram_malloc.h"
#include "enum.h"
#include "drone.h"
#include "joystick.h"

#define FARM_GRID_N farm_get_instance()->current_size
#define FARM_BLOCK_SIZE 80
#define DRONE_COORD_SCALNG_FACTOR (FARM_BLOCK_SIZE / 100.0)

static lv_obj_t *g_screen_load;
static lv_obj_t *g_screen_main;

static lv_obj_t *farm_grid = NULL;
static lv_obj_t *g_drone = NULL;

lv_obj_t *g_current_window = NULL;

static farm_block_t g_farm_blocks[10][10];

static void ui_screen_main_create();
static void ui_farm_grid_create(lv_obj_t *parent);
static void ui_field_update(int x, int y);
static lv_obj_t *ui_crop_grwoing_bar(lv_obj_t *parent);
static void ui_gold_bar_create(lv_obj_t *parent);
static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x, lv_coord_t y);
static lv_obj_t *ui_seed_table_create(lv_obj_t *parent);
static lv_obj_t *ui_plant_window_create();
static lv_obj_t *ui_drone_create(lv_obj_t *parent);
static lv_obj_t *drone_window_create();
static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y);
static void ui_update_100ms();
static void ui_update_1s();

void ui_init(void)
{
	ui_screen_main_create();

	ui_screen_switch(UI_SCREEN_MAIN);
}

void ui_screen_switch(ui_screen_type_t screen)
{
	switch (screen)
	{
	case UI_SCREEN_LOAD:
		lv_scr_load(g_screen_load);
		break;
	case UI_SCREEN_MAIN:
		lv_scr_load(g_screen_main);
		break;
	default:
		lv_scr_load(g_screen_main);
	}
}

void ui_event_handler(event_t *event)
{
	if (event == NULL)
		return;

	switch (event->type) // 后续细致处理
	{
	case EVENT_ON_FIELD_PLANTED:
	case EVENT_ON_FIELD_CLEARED:
	case EVENT_ON_FIELD_HARVESTED:
	case EVENT_ON_CROP_STAGE_CHANGE:
	case EVENT_ON_PEST_DETECTED:
	case EVENT_ON_PEST_COMMITTED:
	case EVENT_ON_PEST_CLEARED:
	case EVENT_ON_FIELD_UPGRADE:
		field_t *data = event->data;
		ui_field_update(data->x, data->y);
		break;
	case EVENT_ON_PLAYER_COIN_CHANGE:
	case EVENT_ON_PLAYER_EXPERIENCE_CHANGE:
	case EVENT_ON_PLAYER_LEVEL_UPGRADE:
		// Handle player-related events
		break;
	default:
		break;
	}
}

static void ui_screen_main_create()
{
	g_screen_main = lv_obj_create(NULL);
	lv_obj_set_style_bg_img_src(g_screen_main, &icon_farm_bg, 0);
	lv_obj_set_style_bg_img_tiled(g_screen_main, true, 0);

	/* 田地 */
	ui_farm_grid_create(g_screen_main);
	lv_obj_add_event_cb(g_screen_main, screen_main_click_cb, LV_EVENT_CLICKED, farm_grid);

	/* 金币显示 */
	ui_gold_bar_create(g_screen_main);

	/* 无人机 */
	g_drone = ui_drone_create(g_screen_main);

	/* 相关按钮 */
	lv_obj_t *shop_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_shop_btn, 40, 380);
	lv_obj_t *storage_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_storage_btn, 40, 460);
	lv_obj_t *plant_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_plant_btn, 920, 450);
	lv_obj_t *setting_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_setting_btn, 920, 40);

	/* 按钮回调 */
	lv_obj_add_event_cb(plant_btn, plant_btn_click_cb, LV_EVENT_CLICKED, ui_plant_window_create);

	lv_obj_add_event_cb(g_screen_main, screen_main_click_cb, LV_EVENT_CLICKED, g_current_window);
}

static void ui_farm_grid_create(lv_obj_t *parent)
{
	if (farm_grid != NULL)
		return;

	farm_grid = ui_div_create(parent);
	lv_obj_set_size(farm_grid, FARM_GRID_N * FARM_BLOCK_SIZE, FARM_GRID_N * FARM_BLOCK_SIZE + 160);
	lv_obj_set_align(farm_grid, LV_ALIGN_TOP_MID);
	lv_obj_set_style_pad_ver(farm_grid, 80, 0);

	for (int i = 0; i < FARM_GRID_N; i++)
	{
		for (int j = 0; j < FARM_GRID_N; j++)
		{
			field_t *field = farm_get_instance()->fields[i][j];
			farm_block_t *block = &g_farm_blocks[i][j];
			block->field = field;
			block->is_planted = field->crop_type != CROP_TYPE_NONE;
			block->has_pest = field->damage != CROP_DAMAGE_NONE;
			block->is_detected = field->is_detected;
			block->x = i;
			block->y = j;

			/* 田地 */
			block->obj = ui_div_create(farm_grid);
			lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
			lv_obj_set_pos(block->obj, FARM_BLOCK_SIZE * i, FARM_BLOCK_SIZE * j);
			lv_obj_set_style_bg_img_src(block->obj, &icon_field_bg, 0);
			lv_obj_add_flag(block->obj, LV_OBJ_FLAG_CHECKABLE);
			lv_obj_add_flag(block->obj, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_set_style_border_width(block->obj, 2, LV_STATE_CHECKED);
			lv_obj_add_event_cb(block->obj, farm_block_click_cb, LV_EVENT_CLICKED, g_screen_main);
			lv_obj_set_user_data(block->obj, block);

			/* 生长进度条 */
			block->growing_bar = ui_crop_grwoing_bar(block->obj);
			lv_bar_set_range(block->growing_bar, 0, field->ready_time);
			lv_bar_set_value(block->growing_bar, field->growing_time, LV_ANIM_OFF);

			/* 作物贴图 */
			block->crop_img = lv_img_create(block->obj);
			lv_obj_center(block->crop_img);
			if (block->is_planted)
			{
				lv_img_set_src(block->crop_img, icon_get_crop(field->crop_type, field->stage));
			}
		}
	}
}

static void ui_field_update(int x, int y)
{
	farm_block_t *block = &g_farm_blocks[x][y];
	block->is_planted = block->field->crop_type != CROP_TYPE_NONE;
	block->has_pest = block->field->damage != CROP_DAMAGE_NONE;
	block->is_detected = block->field->is_detected;

	if (block->is_planted)
	{
		lv_img_set_src(block->crop_img, icon_get_crop(block->field->crop_type, block->field->stage));
	}
	else
	{
		lv_obj_clean(block->obj);
	}
}

static lv_obj_t *ui_drone_create(lv_obj_t *parent)
{
	lv_obj_t *drone = lv_btn_create(parent);
	lv_obj_add_event_cb(drone, drone_click_cb, LV_EVENT_CLICKED, drone_window_create);

	// for simulate
	lv_obj_t *label = lv_label_create(drone);
	lv_label_set_text(label, "Drone");
	lv_obj_set_style_pad_all(drone, 0, 0);

	lv_obj_set_size(drone, 40, 40);

	// lv_obj_set_pos(drone, -60, 20);

	return drone;
}

static void ui_drone_set_pos(lv_coord_t x, lv_coord_t y)
{
	lv_obj_align_to(g_drone, farm_grid, LV_ALIGN_TOP_LEFT, x - 20, y - 20);
}
void test_cb(lv_event_t *e)
{
	drone_state_t *state = lv_event_get_user_data(e);
	if (*state == DRONE_STATE_DETECTING)
	{
		drone_state_switch(DRONE_STATE_FREE);
	}
	else if (*state == DRONE_STATE_FREE)
	{
		drone_state_switch(DRONE_STATE_DETECTING);
	}
}
static lv_obj_t *drone_window_create()
{
	lv_obj_t *body = ui_div_create(g_screen_main);
	drone_t *drone = drone_get_instance();

	lv_obj_t *label = lv_label_create(body);
	lv_label_set_text_fmt(label, "Speed Level: %d\nStorage Level: %d", drone->speed_level, drone->storage_level);

	lv_obj_t *test_btn = lv_btn_create(body);
	lv_obj_add_event_cb(test_btn, test_cb, LV_EVENT_CLICKED, &drone->drone_state);

	lv_obj_t *div = ui_window_create("PLANT", body, NULL, NULL, NULL, NULL, NULL);
	lv_obj_align_to(div, g_drone, LV_ALIGN_OUT_RIGHT_TOP, 20, -40);
	lv_obj_set_size(div, 206, 220);

	return div;
}

static lv_obj_t *ui_crop_grwoing_bar(lv_obj_t *parent)
{
	lv_obj_t *bar = lv_bar_create(parent);
	lv_obj_add_event_cb(bar, crop_growing_bar_event, LV_EVENT_DRAW_PART_END, NULL);
	lv_obj_set_size(bar, 75, 10);
	lv_obj_set_align(bar, LV_ALIGN_BOTTOM_MID);
	lv_obj_set_pos(bar, 0, -2);
	lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bar, LV_OPA_50, LV_PART_MAIN);
	return bar;
}

static void ui_gold_bar_create(lv_obj_t *parent)
{
	lv_obj_t *bar = ui_div_create(parent);
	lv_obj_set_size(bar, 120, 40);
	lv_obj_set_pos(bar, 200, 8);
	lv_obj_set_style_bg_img_src(bar, &icon_gold_bar_bg, 0);
	lv_obj_add_flag(bar, LV_OBJ_FLAG_FLOATING);
}

static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x, lv_coord_t y)
{
	lv_obj_t *btn = ui_div_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_style_bg_img_src(btn, img, 0);
	lv_obj_set_pos(btn, x, y);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING);

	return btn;
}

static lv_obj_t *ui_seed_table_create(lv_obj_t *parent)
{
	static lv_coord_t col_dsc[] = {60, 60, 60, LV_GRID_TEMPLATE_LAST};
	static lv_coord_t row_dsc[] = {60, 60, 60, LV_GRID_TEMPLATE_LAST};

	lv_obj_t *grid = ui_div_create(parent);
	lv_obj_set_layout(grid, LV_LAYOUT_GRID);
	lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

	lv_obj_set_size(grid, 200, 300);
	lv_obj_set_style_pad_column(grid, 5, 0);
	lv_obj_set_style_pad_row(grid, 5, 0);
	lv_obj_set_style_pad_all(grid, 3, 0);

	// lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ON);

	static ui_drag_to_plant_desc_t seeds[CROP_TYPE_NONE];

	crop_type_t i;
	lv_obj_t *obj;
	lv_obj_t *label;
	for (i = 0; i < CROP_TYPE_NONE; i++)
	{
		uint8_t col = i % 3;
		uint8_t row = i / 3;
		obj = lv_obj_create(grid);
		lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1,
							 LV_GRID_ALIGN_STRETCH, row, 1);

		lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd88a), LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(obj, lv_color_make(241, 194, 125), LV_STATE_PRESSED);
		lv_obj_set_style_border_color(obj, lv_color_make(205, 133, 63), 0);
		lv_obj_set_style_border_width(obj, 1, 0);
		// lv_obj_add_event_cb(obj, item_click_cb, LV_EVENT_CLICKED, NULL);

		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

		lv_obj_t *item1_label = lv_label_create(obj);
		lv_label_set_text(item1_label, "Wheat");
		lv_obj_set_style_text_color(item1_label, lv_color_make(60, 42, 29), 0);
		lv_obj_set_style_text_align(item1_label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_center(item1_label);

		seeds[i] = (ui_drag_to_plant_desc_t){.type = i, .img = &icon_crop_wheat_ripe, .fields = farm_grid};
		lv_obj_add_event_cb(obj, drag_to_plant_cb, LV_EVENT_PRESSING, &seeds[i]);
		lv_obj_add_event_cb(obj, drag_to_plant_cb, LV_EVENT_RELEASED, &seeds[i]);
	}

	return grid;
}

static lv_obj_t *ui_plant_window_create()
{
	lv_obj_t *grid = ui_seed_table_create(g_screen_main);

	lv_obj_t *div = ui_window_create("PLANT", grid, NULL, NULL, NULL, NULL, NULL);
	lv_obj_set_align(div, LV_ALIGN_RIGHT_MID);
	lv_obj_set_pos(div, -20, -20);
	lv_obj_set_size(div, 206, 290);

	return div;
}

void ui_update_timer_init()
{
	lv_timer_create(ui_update_100ms, 100, NULL);
	lv_timer_create(ui_update_1s, 1000, NULL);
}

static void ui_update_100ms()
{
	/* UPDATE drone */
	drone_t *drone = drone_get_instance();
	if (drone->drone_state == DRONE_STATE_DETECTING)
	{
		pos_t vector = {
			.x = joystick_get_dir_x(),
			.y = joystick_get_dir_y()};
		drone_move(vector);
		pos_t pos = drone->current_pos;
		ui_drone_set_pos(pos.x * DRONE_COORD_SCALNG_FACTOR, pos.y * DRONE_COORD_SCALNG_FACTOR);
	}
	else
	{
		ui_drone_set_pos(-40, 40);
	}
}

static void ui_update_1s()
{
	/* UPDATE farm */
	for (int i = 0; i < FARM_GRID_N; i++)
	{
		for (int j = 0; j < FARM_GRID_N; j++)
		{
			farm_block_t *block = &g_farm_blocks[i][j];
			if (block->is_planted)
			{
				/* UPDATE growing bar */
				lv_bar_set_range(block->growing_bar, 0, block->field->ready_time);
				lv_bar_set_value(block->growing_bar, block->field->growing_time, LV_ANIM_OFF);
			}
		}
	}
}

lv_obj_t *ui_div_create(lv_obj_t *parent)
{
	lv_obj_t *div = lv_obj_create(parent);
	lv_obj_set_style_bg_opa(div, 0, 0);
	lv_obj_set_style_border_width(div, 0, 0);
	lv_obj_set_style_pad_all(div, 0, 0);

	return div;
}

lv_obj_t *ui_window_create(const char *title, lv_obj_t *body, const void *btn1_text, void (*btn1_cb)(lv_event_t *), const void *btn2_text, void (*btn2_cb)(lv_event_t *), void *user_data)
{
	if (g_current_window)
		lv_obj_del(g_current_window); // 只允许同时显示一个弹窗

	lv_obj_t *div = lv_obj_create(g_screen_main);

	lv_obj_set_style_border_width(div, 3, 0);
	lv_obj_set_style_border_color(div, lv_color_make(139, 69, 19), 0);
	lv_obj_set_style_radius(div, 8, 0);
	lv_obj_set_style_shadow_width(div, 10, 0);
	lv_obj_set_style_pad_all(div, 0, 0);
	lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(div, lv_color_make(245, 232, 200), 0);

	lv_obj_add_event_cb(div, window_delete_cb, LV_EVENT_DELETE, NULL);

	lv_obj_set_parent(body, div);
	lv_obj_set_pos(body, 0, 40);

	lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(body, lv_color_hex(0xf3e2a1), 0);

	lv_obj_set_style_radius(body, 0, 0);
	lv_obj_set_style_border_color(body, lv_color_hex(0x8b4513), 0);
	lv_obj_set_style_border_width(body, 2, 0);

	lv_obj_set_size(body, lv_pct(100), lv_pct(100));

	lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *header = lv_obj_create(div);
	lv_obj_set_size(header, 200, 40);
	lv_obj_set_style_pad_all(header, 0, 0);
	lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(header, lv_color_make(139, 69, 19), 0);
	lv_obj_set_style_border_width(header, 0, 0);
	lv_obj_set_style_radius(header, 0, 0);

	lv_obj_t *title_label = lv_label_create(header);
	lv_label_set_text(title_label, title);
	lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
	lv_obj_center(title_label);

	if (btn1_text)
	{
		lv_obj_t *footer = lv_obj_create(div);
		lv_obj_set_size(footer, 200, 50);
		lv_obj_set_style_pad_all(footer, 0, 0);
		lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
		lv_obj_set_style_bg_color(footer, lv_color_make(230, 207, 160), 0);
		lv_obj_set_style_border_width(footer, 2, 0);
		lv_obj_set_style_border_color(footer, lv_color_hex(0x8b4513), 0);
		lv_obj_set_style_radius(footer, 0, 0);

		lv_obj_t *btn1 = lv_btn_create(footer);
		lv_obj_set_size(btn1, 86, 34);
		lv_obj_align(btn1, LV_ALIGN_LEFT_MID, 5, 0);
		lv_obj_set_style_bg_color(btn1, lv_color_make(205, 133, 63), LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(btn1, lv_color_make(166, 105, 46), LV_STATE_PRESSED);
		lv_obj_add_event_cb(btn1, btn1_cb, LV_EVENT_CLICKED, user_data);

		lv_obj_t *btn1_label = lv_label_create(btn1);
		lv_label_set_text(btn1_label, btn1_text);
		lv_obj_set_style_text_color(btn1_label, lv_color_white(), 0);
		lv_obj_center(btn1_label);

		if (btn2_text)
		{
			lv_obj_t *btn2 = lv_btn_create(footer);
			lv_obj_set_size(btn2, 86, 34);
			lv_obj_align(btn2, LV_ALIGN_RIGHT_MID, -5, 0);
			lv_obj_set_style_bg_color(btn2, lv_color_make(160, 82, 45), LV_STATE_DEFAULT);
			lv_obj_set_style_bg_color(btn2, lv_color_make(139, 66, 31), LV_STATE_PRESSED);
			lv_obj_add_event_cb(btn2, btn2_cb, LV_EVENT_CLICKED, user_data);

			lv_obj_t *btn2_label = lv_label_create(btn2);
			lv_label_set_text(btn2_label, btn2_text);
			lv_obj_set_style_text_color(btn2_label, lv_color_white(), 0);
			lv_obj_center(btn2_label);
		}
	}

	return div;
}
