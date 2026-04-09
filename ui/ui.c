#include "ui.h"
#include "ui_event.h"
#include "icon.h"
#include "sdram_malloc.h"
#include "enum.h"

#define FARM_GRID_N 5
#define FARM_BLOCK_SIZE 80

static lv_obj_t *g_screen_load;
static lv_obj_t *g_screen_main;

static lv_obj_t *farm_grid;

typedef struct {
	lv_obj_t *obj;
	lv_obj_t *window;
	bool is_planted;
	uint8_t x;
	uint8_t y;
	icon_pest_type_t pest_type;
} farm_block_t;

static farm_block_t g_farm_blocks[FARM_GRID_N][FARM_GRID_N];

static lv_obj_t *ui_div_create(lv_obj_t *parent);
	
static void ui_screen_main_create();
static void ui_farm_grid_create(lv_obj_t *parent);
static void ui_gold_bar_create(lv_obj_t *parent);
static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x, lv_coord_t y);
static lv_obj_t *ui_seed_table_create(lv_obj_t *parent);
static lv_obj_t *ui_plant_window_create();

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

static lv_obj_t *ui_div_create(lv_obj_t *parent) {
	lv_obj_t *div = lv_obj_create(parent);
	lv_obj_set_style_bg_opa(div, 0, 0);
	lv_obj_set_style_border_width(div, 0, 0);
	lv_obj_set_style_pad_all(div, 0, 0);
	
	return div;
}

static void ui_screen_main_create() {
	g_screen_main = lv_obj_create(NULL);
	lv_obj_set_style_bg_img_src(g_screen_main, &icon_farm_bg, 0);
	lv_obj_set_style_bg_img_tiled(g_screen_main, true, 0);
	
	/* 田地 */
	farm_grid = ui_div_create(g_screen_main);
	lv_obj_set_size(farm_grid, FARM_GRID_N * FARM_BLOCK_SIZE, FARM_GRID_N * FARM_BLOCK_SIZE);
	lv_obj_center(farm_grid);
	ui_farm_grid_create(farm_grid);
	lv_obj_add_event_cb(g_screen_main, screen_main_click_cb, LV_EVENT_CLICKED, farm_grid);
	
	/* 金币显示 */
	ui_gold_bar_create(g_screen_main);
	
	/* 相关按钮 */
	lv_obj_t *shop_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_shop_btn, 40, 380);
	lv_obj_t *storage_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_storage_btn, 40, 460);
	lv_obj_t *plant_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_plant_btn, 920, 450);
	lv_obj_t *setting_btn = ui_icon_btn_create(g_screen_main, 64, 64, &icon_setting_btn, 920, 40);
	
	/* 按钮回调 */
	lv_obj_add_event_cb(plant_btn, plant_btn_click_cb, LV_EVENT_CLICKED, ui_plant_window_create);
	
}

static void ui_farm_grid_create(lv_obj_t *parent) {
	for(int i=0; i<FARM_GRID_N; i++) {
		for(int j=0; j<FARM_GRID_N; j++) {
			farm_block_t *block = &g_farm_blocks[i][j];
			block->obj = ui_div_create(parent);
			block->window = NULL;
			block->is_planted = 0;
			block->x = i;
			block->y = j;
			block->pest_type = ICON_PEST_UNKNOWN;
			
			lv_obj_set_size(block->obj, FARM_BLOCK_SIZE, FARM_BLOCK_SIZE);
			lv_obj_set_pos(block->obj, FARM_BLOCK_SIZE * i, FARM_BLOCK_SIZE * j);
			lv_obj_set_style_bg_img_src(block->obj, &icon_field_bg, 0);
			lv_obj_add_flag(block->obj, LV_OBJ_FLAG_CHECKABLE);
			lv_obj_add_flag(block->obj, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_set_style_border_width(block->obj, 2, LV_STATE_CHECKED);
			lv_obj_add_event_cb(block->obj, farm_block_click_cb, LV_EVENT_CLICKED, block);
			
			/** for test **/
			lv_obj_t *crop = lv_img_create(block->obj);
			lv_obj_center(crop);
			if(i == j) 
				lv_img_set_src(crop, &icon_crop_corn_ripe);
			else
				lv_img_set_src(crop, &icon_crop_wheat_ripe);
		}
	}
}

static void ui_gold_bar_create(lv_obj_t *parent) {
	lv_obj_t *bar = ui_div_create(parent);
	lv_obj_set_size(bar, 120, 40);
	lv_obj_set_pos(bar, 200, 8);
	lv_obj_set_style_bg_img_src(bar, &icon_gold_bar_bg, 0);
}

static lv_obj_t *ui_icon_btn_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, const void *img, lv_coord_t x, lv_coord_t y) {
	lv_obj_t *btn = ui_div_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_style_bg_img_src(btn, img, 0);
	lv_obj_set_pos(btn, x, y);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	
	return btn;
}

static lv_obj_t *ui_seed_table_create(lv_obj_t *parent) {
	static lv_coord_t col_dsc[] = {60, 60, 60, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {60, 60, 60, LV_GRID_TEMPLATE_LAST};
	
	lv_obj_t *grid = ui_div_create(parent);
	lv_obj_set_layout(grid, LV_LAYOUT_GRID);
	lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
	
	lv_obj_set_size(grid, 200, 300);
	lv_obj_set_style_pad_column(grid, 5, 0);
	lv_obj_set_style_pad_row(grid, 5, 0);
	lv_obj_set_style_pad_all(grid, 3, 0);
	
	lv_obj_set_style_bg_opa(grid, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(grid, lv_color_hex(0xf3e2a1), 0);
	
	lv_obj_set_style_radius(grid, 0, 0);
	lv_obj_set_style_border_color(grid, lv_color_hex(0x8b4513), 0);
	lv_obj_set_style_border_width(grid, 2, 0);
	
	//lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ON);
	
	static ui_drag_to_plant_desc_t seeds[CROP_TYPE_NONE];
	
	crop_type_t i;
	lv_obj_t *obj;
	lv_obj_t * label;
	for(i = 0; i < CROP_TYPE_NONE; i++) {
		uint8_t col = i % 3;
        uint8_t row = i / 3;
		obj = lv_obj_create(grid);
		lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1,
                                  LV_GRID_ALIGN_STRETCH, row, 1);
		
			
		lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd88a), LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(obj, lv_color_make(241, 194, 125), LV_STATE_PRESSED);
		lv_obj_set_style_border_color(obj, lv_color_make(205, 133, 63), 0);
		lv_obj_set_style_border_width(obj, 1, 0);
		//lv_obj_add_event_cb(obj, item_click_cb, LV_EVENT_CLICKED, NULL);
		
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

static lv_obj_t *ui_plant_window_create() {
	lv_obj_t *div = lv_obj_create(g_screen_main);
	lv_obj_set_align(div, LV_ALIGN_RIGHT_MID);
	lv_obj_set_pos(div, -20, -20);
	lv_obj_set_size(div, 206, 290);
	lv_obj_set_style_border_width(div, 3, 0);
	lv_obj_set_style_border_color(div, lv_color_make(139, 69, 19), 0);
	lv_obj_set_style_radius(div, 8, 0);
	lv_obj_set_style_shadow_width(div, 10, 0);
	lv_obj_set_style_pad_all(div, 0, 0);
	lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(div, lv_color_make(245, 232, 200), 0);
	
	
	
	lv_obj_t *grid = ui_seed_table_create(div);
	lv_obj_set_pos(grid, 0, 40);
	
	
	
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);

    
    lv_obj_t *header = lv_obj_create(div);
    lv_obj_set_size(header, 200, 40);
	lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_make(139, 69, 19), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
	

    
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "PLANT");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    //lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_center(title);


	return div;
}