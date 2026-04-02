#include "ui.h"
#include "ui_event.h"
#include "icon.h"

#define FARM_GRID_N 5
#define FARM_BLOCK_SIZE 80

static lv_obj_t *g_screen_load;
static lv_obj_t *g_screen_main;

typedef struct {
	lv_obj_t *obj;
	lv_obj_t *window;
	bool is_planted;
	uint8_t x;
	uint8_t y;
	icon_pest_type_t pest_type;
} farm_block_t;

static farm_block_t g_farm_blocks[FARM_GRID_N][FARM_GRID_N];

static void ui_screen_main_create();
static void ui_farm_grid_create(lv_obj_t *parent);

void ui_init(void) {
	ui_screen_main_create();
	
	ui_screen_switch(UI_SCREEN_MAIN);
}

void ui_screen_switch(ui_screen_type_t screen) {
	switch(screen) {
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

static void ui_screen_main_create() {
	g_screen_main = lv_obj_create(NULL);
	
	lv_obj_t *farm_grid = lv_obj_create(g_screen_main);
	lv_obj_set_size(farm_grid, FARM_GRID_N * FARM_BLOCK_SIZE, FARM_GRID_N * FARM_BLOCK_SIZE);
	lv_obj_center(farm_grid);
	lv_obj_set_style_bg_opa(farm_grid, 0, 0);
	lv_obj_set_style_border_width(farm_grid, 0, 0);
	lv_obj_set_style_pad_all(farm_grid, 0, 0);
	ui_farm_grid_create(farm_grid);
	
	lv_obj_add_event_cb(g_screen_main, screen_main_click_cb, LV_EVENT_CLICKED, farm_grid);
}

static void ui_farm_grid_create(lv_obj_t *parent) {
	for(int i=0; i<FARM_GRID_N; i++) {
		for(int j=0; j<FARM_GRID_N; j++) {
			farm_block_t *block = &g_farm_blocks[i][j];
			block->obj = lv_btn_create(parent);
			block->window = NULL;
			block->is_planted = 0;
			block->x = i;
			block->y = j;
			block->pest_type = ICON_PEST_UNKNOWN;
			
			lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
			lv_obj_set_pos(block->obj, FARM_BLOCK_SIZE * i, FARM_BLOCK_SIZE * j);
			lv_obj_set_style_bg_img_src(block->obj, &icon_field_bg, 0);
			lv_obj_add_flag(block->obj, LV_OBJ_FLAG_CHECKABLE);
			lv_obj_set_style_border_width(block->obj, 2, LV_STATE_CHECKED);
			lv_obj_add_event_cb(block->obj, farm_block_click_cb, LV_EVENT_CLICKED, block);
		}
	}
}