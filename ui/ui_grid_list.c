#include "ui_grid_list.h"

static bool ui_grid_list_apply_cfg(ui_grid_list_t *list, const ui_grid_list_cfg_t *cfg)
{
    if (!list || !list->obj || !cfg || cfg->col_count == 0 || cfg->row_count == 0)
    {
        return false;
    }

    lv_coord_t *new_col_dsc = (lv_coord_t *)lv_mem_alloc(sizeof(lv_coord_t) * (cfg->col_count + 1));
    lv_coord_t *new_row_dsc = (lv_coord_t *)lv_mem_alloc(sizeof(lv_coord_t) * (cfg->row_count + 1));
    if (!new_col_dsc || !new_row_dsc)
    {
        if (new_col_dsc)
        {
            lv_mem_free(new_col_dsc);
        }
        if (new_row_dsc)
        {
            lv_mem_free(new_row_dsc);
        }
        return false;
    }

    for (uint16_t i = 0; i < cfg->col_count; ++i)
    {
        new_col_dsc[i] = cfg->item_w;
    }
    new_col_dsc[cfg->col_count] = LV_GRID_TEMPLATE_LAST;

    for (uint16_t i = 0; i < cfg->row_count; ++i)
    {
        new_row_dsc[i] = cfg->item_h;
    }
    new_row_dsc[cfg->row_count] = LV_GRID_TEMPLATE_LAST;

    if (list->col_dsc)
    {
        lv_mem_free(list->col_dsc);
    }
    if (list->row_dsc)
    {
        lv_mem_free(list->row_dsc);
    }

    list->col_dsc = new_col_dsc;
    list->row_dsc = new_row_dsc;
    list->cfg = *cfg;

    lv_obj_set_layout(list->obj, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(list->obj, list->col_dsc, list->row_dsc);
    lv_obj_set_size(
        list->obj,
        (cfg->item_w * cfg->col_count) + (cfg->pad_col * (cfg->col_count - 1)) + (cfg->pad_all * 2),
        (cfg->item_h * cfg->row_count) + (cfg->pad_row * (cfg->row_count - 1)) + (cfg->pad_all * 2));

    lv_obj_set_style_pad_column(list->obj, cfg->pad_col, 0);
    lv_obj_set_style_pad_row(list->obj, cfg->pad_row, 0);
    lv_obj_set_style_pad_all(list->obj, cfg->pad_all, 0);

    return true;
}

static void ui_grid_list_delete_cb(lv_event_t *e)
{
    ui_grid_list_t *list = (ui_grid_list_t *)lv_event_get_user_data(e);
    if (list)
    {
        if (list->col_dsc)
        {
            lv_mem_free(list->col_dsc);
            list->col_dsc = NULL;
        }
        if (list->row_dsc)
        {
            lv_mem_free(list->row_dsc);
            list->row_dsc = NULL;
        }
        lv_mem_free(list);
    }
}

void ui_grid_list_cfg_init(ui_grid_list_cfg_t *cfg)
{
    if (!cfg)
    {
        return;
    }

    cfg->item_w = 60;
    cfg->item_h = 60;
    cfg->col_count = 3;
    cfg->row_count = 3;
    cfg->pad_col = 5;
    cfg->pad_row = 5;
    cfg->pad_all = 3;
    cfg->item_scrollable = false;

    cfg->item_bg_color = lv_color_hex(0xffd88a);
    cfg->item_bg_color_pressed = lv_color_make(241, 194, 125);
    cfg->item_border_color = lv_color_make(205, 133, 63);
    cfg->item_border_width = 1;
}

ui_grid_list_t *ui_grid_list_create(lv_obj_t *parent, const ui_grid_list_cfg_t *cfg)
{
    if (!parent || !cfg || cfg->col_count == 0 || cfg->row_count == 0)
    {
        return NULL;
    }

    ui_grid_list_t *list = (ui_grid_list_t *)lv_mem_alloc(sizeof(ui_grid_list_t));
    if (!list)
    {
        return NULL;
    }

    list->col_dsc = NULL;
    list->row_dsc = NULL;
    list->item_count = 0;
    list->next_index = 0;

    list->obj = lv_obj_create(parent);

    if (!ui_grid_list_apply_cfg(list, cfg))
    {
        lv_obj_del(list->obj);
        lv_mem_free(list);
        return NULL;
    }

    lv_obj_set_style_bg_opa(list->obj, 0, 0);
    lv_obj_set_style_border_width(list->obj, 0, 0);

    lv_obj_add_event_cb(list->obj, ui_grid_list_delete_cb, LV_EVENT_DELETE, list);

    return list;
}

lv_obj_t *ui_grid_list_add_item_at(ui_grid_list_t *list, uint16_t col, uint16_t row)
{
    if (!list || !list->obj)
    {
        return NULL;
    }
    if (col >= list->cfg.col_count || row >= list->cfg.row_count)
    {
        return NULL;
    }

    lv_obj_t *item = lv_obj_create(list->obj);
    lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col, 1,
                         LV_GRID_ALIGN_STRETCH, row, 1);

    lv_obj_set_style_bg_color(item, list->cfg.item_bg_color, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(item, list->cfg.item_bg_color_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(item, list->cfg.item_border_color, 0);
    lv_obj_set_style_border_width(item, list->cfg.item_border_width, 0);

    if (!list->cfg.item_scrollable)
    {
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    }

    list->item_count++;

    return item;
}

lv_obj_t *ui_grid_list_add_item(ui_grid_list_t *list)
{
    if (!list)
    {
        return NULL;
    }

    uint16_t capacity = (uint16_t)(list->cfg.col_count * list->cfg.row_count);
    if (list->next_index >= capacity)
    {
        return NULL;
    }

    uint16_t col = (uint16_t)(list->next_index % list->cfg.col_count);
    uint16_t row = (uint16_t)(list->next_index / list->cfg.col_count);
    lv_obj_t *item = ui_grid_list_add_item_at(list, col, row);
    if (item)
    {
        list->next_index++;
    }

    return item;
}

bool ui_grid_list_clear(ui_grid_list_t *list)
{
    if (!list || !list->obj)
    {
        return false;
    }

    lv_obj_clean(list->obj);
    list->item_count = 0;
    list->next_index = 0;
    return true;
}

bool ui_grid_list_reset(ui_grid_list_t *list, const ui_grid_list_cfg_t *cfg)
{
    if (!list || !cfg)
    {
        return false;
    }

    if (!ui_grid_list_clear(list))
    {
        return false;
    }

    return ui_grid_list_apply_cfg(list, cfg);
}

bool ui_grid_list_bind_item_event(lv_obj_t *item, lv_event_cb_t cb, lv_event_code_t code, void *user_data)
{
    if (!item || !cb)
    {
        return false;
    }

    lv_obj_add_event_cb(item, cb, code, user_data);
    return true;
}

bool ui_grid_list_bind_list_event(ui_grid_list_t *list, lv_event_cb_t cb, lv_event_code_t code, void *user_data)
{
    if (!list || !list->obj || !cb)
    {
        return false;
    }

    lv_obj_add_event_cb(list->obj, cb, code, user_data);
    return true;
}

lv_obj_t *ui_grid_list_get_obj(ui_grid_list_t *list)
{
    if (!list)
    {
        return NULL;
    }

    return list->obj;
}
