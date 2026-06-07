#include "ui_grid_list.h"

/* 应用配置并刷新网格布局 */
static bool ui_grid_list_apply_cfg(ui_grid_list_t *list, const ui_grid_list_cfg_t *cfg) {
    if (!list || !list->obj || !cfg || cfg->col_count == 0 || cfg->row_count == 0) {
        return false;
    }

    /* 先分配新描述数组，失败就直接返回 */
    lv_coord_t *new_col_dsc = (lv_coord_t *)lv_mem_alloc(sizeof(lv_coord_t) * (cfg->col_count + 1));
    lv_coord_t *new_row_dsc = (lv_coord_t *)lv_mem_alloc(sizeof(lv_coord_t) * (cfg->row_count + 1));
    if (!new_col_dsc || !new_row_dsc) {
        if (new_col_dsc) {
            lv_mem_free(new_col_dsc);
        }
        if (new_row_dsc) {
            lv_mem_free(new_row_dsc);
        }
        return false;
    }

    /* 每一列都使用同样的宽度 */
    for (uint16_t i = 0; i < cfg->col_count; ++i) {
        new_col_dsc[i] = cfg->item_w;
    }
    new_col_dsc[cfg->col_count] = LV_GRID_TEMPLATE_LAST;

    /* 每一行都使用同样的高度 */
    for (uint16_t i = 0; i < cfg->row_count; ++i) {
        new_row_dsc[i] = cfg->item_h;
    }
    new_row_dsc[cfg->row_count] = LV_GRID_TEMPLATE_LAST;

    /* 重建前先释放旧布局，避免重复分配 */
    if (list->col_dsc) {
        lv_mem_free(list->col_dsc);
    }
    if (list->row_dsc) {
        lv_mem_free(list->row_dsc);
    }

    list->col_dsc = new_col_dsc;
    list->row_dsc = new_row_dsc;
    list->cfg = *cfg;

    lv_obj_set_layout(list->obj, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(list->obj, list->col_dsc, list->row_dsc);
    lv_obj_set_size(list->obj,
                    (cfg->item_w * cfg->col_count) + (cfg->pad_col * (cfg->col_count - 1)) + (cfg->pad_all * 2),
                    (cfg->item_h * cfg->row_count) + (cfg->pad_row * (cfg->row_count - 1)) + (cfg->pad_all * 2));

    lv_obj_set_style_pad_column(list->obj, cfg->pad_col, 0);
    lv_obj_set_style_pad_row(list->obj, cfg->pad_row, 0);
    lv_obj_set_style_pad_all(list->obj, cfg->pad_all, 0);

    return true;
}

/* 删除时释放组件自己申请的内存 */
static void ui_grid_list_delete_cb(lv_event_t *e) {
    ui_grid_list_t *list = (ui_grid_list_t *)lv_event_get_user_data(e);
    if (list) {
        if (list->col_dsc) {
            lv_mem_free(list->col_dsc);
            list->col_dsc = NULL;
        }
        if (list->row_dsc) {
            lv_mem_free(list->row_dsc);
            list->row_dsc = NULL;
        }
        lv_mem_free(list);
    }
}

/* 初始化一套默认参数 */
void ui_grid_list_cfg_init(ui_grid_list_cfg_t *cfg) {
    if (!cfg) {
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

/* 快速构造配置：init 后自动填入宽高行列 */
ui_grid_list_cfg_t ui_grid_list_cfg_make(lv_coord_t item_w, lv_coord_t item_h, uint16_t cols, uint16_t rows) {
    ui_grid_list_cfg_t cfg;
    ui_grid_list_cfg_init(&cfg);
    cfg.item_w = item_w;
    cfg.item_h = item_h;
    cfg.col_count = cols;
    cfg.row_count = rows;
    return cfg;
}

/* 一行设好三个间距 */
void ui_grid_list_cfg_set_pad(ui_grid_list_cfg_t *cfg, lv_coord_t pad_col, lv_coord_t pad_row, lv_coord_t pad_all) {
    cfg->pad_col = pad_col;
    cfg->pad_row = pad_row;
    cfg->pad_all = pad_all;
}

/* 创建网格列表对象 */
ui_grid_list_t *ui_grid_list_create(lv_obj_t *parent, const ui_grid_list_cfg_t *cfg) {
    if (!parent || !cfg || cfg->col_count == 0 || cfg->row_count == 0) {
        return NULL;
    }

    /* 先建数据对象，再挂到 LVGL 容器上 */
    ui_grid_list_t *list = (ui_grid_list_t *)lv_mem_alloc(sizeof(ui_grid_list_t));
    if (!list) {
        return NULL;
    }

    list->col_dsc = NULL;
    list->row_dsc = NULL;
    list->item_count = 0;
    list->next_index = 0;

    list->obj = lv_obj_create(parent);

    if (!ui_grid_list_apply_cfg(list, cfg)) {
        lv_obj_del(list->obj);
        lv_mem_free(list);
        return NULL;
    }

    lv_obj_set_style_bg_opa(list->obj, 0, 0);
    lv_obj_set_style_border_width(list->obj, 0, 0);

    /* 容器删除时顺带释放网格描述 */
    lv_obj_add_event_cb(list->obj, ui_grid_list_delete_cb, LV_EVENT_DELETE, list);

    return list;
}

/* 按指定位置添加一个格子 */
lv_obj_t *ui_grid_list_add_item_at(ui_grid_list_t *list, uint16_t col, uint16_t row) {
    if (!list || !list->obj) {
        return NULL;
    }
    /* 越界就不创建 */
    if (col >= list->cfg.col_count || row >= list->cfg.row_count) {
        return NULL;
    }

    /* 让格子占满指定单元格 */
    lv_obj_t *item = lv_obj_create(list->obj);
    lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);

    lv_obj_set_style_pad_all(item, 0, 0);
    lv_obj_set_style_bg_color(item, list->cfg.item_bg_color, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(item, list->cfg.item_bg_color_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(item, list->cfg.item_border_color, 0);
    lv_obj_set_style_border_width(item, list->cfg.item_border_width, 0);

    /* 不需要滚动时关掉滚动标志 */
    if (!list->cfg.item_scrollable) {
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    }

    list->item_count++;

    return item;
}

/* 按顺序添加下一个格子 */
lv_obj_t *ui_grid_list_add_item(ui_grid_list_t *list) {
    if (!list) {
        return NULL;
    }

    /* 超出容量就停止添加 */
    uint16_t capacity = (uint16_t)(list->cfg.col_count * list->cfg.row_count);
    if (list->next_index >= capacity) {
        return NULL;
    }

    /* 行优先计算当前落点 */
    uint16_t col = (uint16_t)(list->next_index % list->cfg.col_count);
    uint16_t row = (uint16_t)(list->next_index / list->cfg.col_count);
    lv_obj_t *item = ui_grid_list_add_item_at(list, col, row);
    if (item) {
        list->next_index++;
    }

    return item;
}

/* 清空所有子项并重置游标 */
bool ui_grid_list_clear(ui_grid_list_t *list) {
    if (!list || !list->obj) {
        return false;
    }

    /* 删除所有子对象 */
    lv_obj_clean(list->obj);
    list->item_count = 0;
    list->next_index = 0;
    return true;
}

/* 先清空再应用新配置 */
bool ui_grid_list_reset(ui_grid_list_t *list, const ui_grid_list_cfg_t *cfg) {
    if (!list || !cfg) {
        return false;
    }

    /* 先清空内容，再刷新布局 */
    if (!ui_grid_list_clear(list)) {
        return false;
    }

    return ui_grid_list_apply_cfg(list, cfg);
}

/* 创建一个带图标的格子，并顺手绑点击事件 */
lv_obj_t *ui_grid_list_add_icon_item(ui_grid_list_t *list, const void *icon_src, lv_event_cb_t cb, void *user_data) {
    /* 先拿一个空位 */
    lv_obj_t *item = ui_grid_list_add_item(list);
    if (!item)
        return NULL;

    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_pad_all(item, 0, 0);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    /* 图标居中放在格子里 */
    lv_obj_t *icon = lv_img_create(item);
    lv_img_set_src(icon, icon_src);
    lv_obj_center(icon);

    /* 直接把点击事件绑到这个格子上 */
    ui_grid_list_bind_item_event(item, cb, LV_EVENT_CLICKED, user_data);
    return item;
}

/* 给单个格子绑定事件 */
bool ui_grid_list_bind_item_event(lv_obj_t *item, lv_event_cb_t cb, lv_event_code_t code, void *user_data) {
    if (!item || !cb) {
        return false;
    }

    lv_obj_add_event_cb(item, cb, code, user_data);
    return true;
}

/* 给整个列表容器绑定事件 */
bool ui_grid_list_bind_list_event(ui_grid_list_t *list, lv_event_cb_t cb, lv_event_code_t code, void *user_data) {
    if (!list || !list->obj || !cb) {
        return false;
    }

    lv_obj_add_event_cb(list->obj, cb, code, user_data);
    return true;
}

/* 获取底层对象 */
lv_obj_t *ui_grid_list_get_obj(ui_grid_list_t *list) {
    if (!list) {
        return NULL;
    }

    return list->obj;
}
