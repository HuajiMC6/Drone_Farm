#ifndef __UI_GRID_LIST_H
#define __UI_GRID_LIST_H

#include "lv_port_disp_template.h"

typedef struct {
    lv_coord_t item_w;
    lv_coord_t item_h;
    uint16_t col_count;
    uint16_t row_count;
    lv_coord_t pad_col;
    lv_coord_t pad_row;
    lv_coord_t pad_all;
    bool item_scrollable;

    /* Default visual style for each item. */
    lv_color_t item_bg_color;
    lv_color_t item_bg_color_pressed;
    lv_color_t item_border_color;
    lv_coord_t item_border_width;
} ui_grid_list_cfg_t;

typedef struct {
    lv_obj_t *obj;
    ui_grid_list_cfg_t cfg;
    lv_coord_t *col_dsc;
    lv_coord_t *row_dsc;
    uint16_t item_count;
    uint16_t next_index;
} ui_grid_list_t;

/* Initialize with a seed-table-like default style. */
void ui_grid_list_cfg_init(ui_grid_list_cfg_t *cfg);

/* Create a reusable grid list container. */
ui_grid_list_t *ui_grid_list_create(lv_obj_t *parent, const ui_grid_list_cfg_t *cfg);

/* Add an item by absolute cell position. */
lv_obj_t *ui_grid_list_add_item_at(ui_grid_list_t *list, uint16_t col, uint16_t row);

/* Add an item to the next free cell in row-major order. */
lv_obj_t *ui_grid_list_add_item(ui_grid_list_t *list);

/* Remove all item children and reset insertion state. */
bool ui_grid_list_clear(ui_grid_list_t *list);

/* Clear then update item layout settings for rebuild scenarios. */
bool ui_grid_list_reset(ui_grid_list_t *list, const ui_grid_list_cfg_t *cfg);

/* Bind callback to a specific item object. */
bool ui_grid_list_bind_item_event(lv_obj_t *item, lv_event_cb_t cb, lv_event_code_t code, void *user_data);

/* Bind callback to the grid container. */
bool ui_grid_list_bind_list_event(ui_grid_list_t *list, lv_event_cb_t cb, lv_event_code_t code, void *user_data);

/* Access underlying LVGL object. */
lv_obj_t *ui_grid_list_get_obj(ui_grid_list_t *list);

#endif
