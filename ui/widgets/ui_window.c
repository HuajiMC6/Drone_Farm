#include "ui_window.h"
#include "ui_common.h"

/* 当前窗口对象 */
static lv_obj_t *g_current_window = NULL;
/* 窗口遮罩对象 */
static lv_obj_t *g_window_mask = NULL;

/* 窗口元信息管理 */
#define UI_WINDOW_META_MAX 12 // 最多同时存在的窗口数量
typedef struct {
    lv_obj_t *window;        // 窗口对象
    bool use_mask;           // 是否使用遮罩
    lv_obj_t *follow_target; // 窗口需要跟随移动的对象（可选）
    bool keep_alive;         // 窗口是否需要保持存活（即隐藏时只是添加HIDDEN标记而不删除窗口对象）
} ui_window_meta_t;

static ui_window_meta_t g_window_meta[UI_WINDOW_META_MAX];

/* 设置窗口元信息 */
static void ui_window_meta_set(lv_obj_t *window, bool use_mask) {
    if (!window) {
        return;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window || g_window_meta[i].window == NULL) {
            g_window_meta[i].window = window;
            g_window_meta[i].use_mask = use_mask;
            g_window_meta[i].follow_target = NULL;
            g_window_meta[i].keep_alive = true;
            return;
        }
    }
}

/* 获取窗口是否使用遮罩 */
static bool ui_window_meta_get_use_mask(lv_obj_t *window) {
    if (!window) {
        return false;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window) {
            return g_window_meta[i].use_mask;
        }
    }

    return true;
}

/* 获取窗口是否保持存活 */
static bool ui_window_meta_get_keep_alive(lv_obj_t *window) {
    if (!window) {
        return false;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window) {
            return g_window_meta[i].keep_alive;
        }
    }

    return false;
}

/* 移除窗口元信息（当窗口被删除时） */
static void ui_window_meta_remove(lv_obj_t *window) {
    if (!window) {
        return;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window) {
            g_window_meta[i].window = NULL;
            return;
        }
    }
}

/* 遮罩点击回调，隐藏窗口 */
static void ui_window_mask_click_cb(lv_event_t *e) {
    lv_event_stop_bubbling(e);
    ui_window_hide_current();
}

/* 获取遮罩对象，若不存在则创建 */
static lv_obj_t *ui_window_mask_ensure(void) {
    if (!g_window_mask || !lv_obj_is_valid(g_window_mask)) {
        g_window_mask = lv_obj_create(lv_layer_top());
        lv_obj_set_size(g_window_mask, 1024, 600);
        lv_obj_set_style_radius(g_window_mask, 0, 0);
        lv_obj_set_style_border_width(g_window_mask, 0, 0);
        lv_obj_set_style_bg_color(g_window_mask, lv_color_make(20, 20, 20), 0);
        lv_obj_set_style_bg_opa(g_window_mask, LV_OPA_40, 0);
        lv_obj_set_style_pad_all(g_window_mask, 0, 0);
        lv_obj_clear_flag(g_window_mask, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_FLOATING); // 不随屏幕滚动而滚动，保证显示正常
        lv_obj_add_event_cb(g_window_mask, ui_window_mask_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_HIDDEN);
    }

    return g_window_mask;
}

/* 为指定窗口显示遮罩 */
static void ui_window_mask_show_for(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    lv_obj_t *mask = ui_window_mask_ensure();

    lv_obj_clear_flag(mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(mask);
    lv_obj_move_foreground(window);
}

/* 隐藏遮罩 */
static void ui_window_mask_hide(void) {
    if (g_window_mask && lv_obj_is_valid(g_window_mask)) {
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 窗口对象删除回调 */
static void ui_window_delete_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);

    // 删除窗口元信息
    ui_window_meta_remove(target);

    // 如果当前窗口被删除了，隐藏遮罩并清除当前窗口引用
    if (target == g_current_window) {
        g_current_window = NULL;
        ui_window_mask_hide();
    }
}

/**
 * 创建窗口
 * @param title 窗口标题
 * @param body 窗口主体内容对象
 * @param enable_mask 是否启用遮罩
 * @return 窗口对象
 */
lv_obj_t *ui_window_create(const char *title, lv_obj_t *body, bool enable_mask) {
    // lv_obj_t *div = lv_obj_create(parent);

    // 窗口对象置于最顶层
    lv_obj_t *div = lv_obj_create(lv_layer_top());
    static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t row_dsc[] = {40, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_style_border_width(div, 3, 0);
    lv_obj_set_style_border_color(div, lv_color_make(139, 69, 19), 0);
    lv_obj_set_style_radius(div, 8, 0);
    lv_obj_set_style_shadow_width(div, 10, 0);
    lv_obj_set_style_pad_all(div, 0, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(div, lv_color_make(245, 232, 200), 0);
    lv_obj_set_layout(div, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(div, col_dsc, row_dsc);
    lv_obj_set_style_pad_row(div, 0, 0);

    lv_obj_move_foreground(div); // 将弹窗置于最上层

    lv_obj_add_event_cb(div, ui_window_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *header = lv_obj_create(div);
    lv_obj_set_grid_cell(header, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_size(header, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_make(139, 69, 19), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t *title_label = lv_label_create(header);
    lv_label_set_text(title_label, title ? title : "");
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_center(title_label);

    lv_obj_t *body_cont = ui_div_create(div);
    lv_obj_set_grid_cell(body_cont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_size(body_cont, LV_PCT(100), LV_PCT(100));
    if (body) {
        lv_obj_set_parent(body, body_cont);
        lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(body, lv_color_hex(0xf3e2a1), 0);

        lv_obj_set_style_radius(body, 0, 0);
        lv_obj_set_style_border_color(body, lv_color_hex(0x8b4513), 0);
        lv_obj_set_style_border_width(body, 2, 0);

        lv_obj_set_size(body, lv_pct(100), lv_pct(100));
    }

    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_flag(div, LV_OBJ_FLAG_FLOATING); // 默认：不随屏幕滚动而滚动，保证显示正常

    ui_window_meta_set(div, enable_mask);

    ui_window_show(div);
    return div;
}

/**
 * 设置窗口跟随滚动
 * @param window 窗口对象
 * @param target 目标对象
 */
void ui_window_follow_scroll(lv_obj_t *window, lv_obj_t *target) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }
    if (!target || !lv_obj_is_valid(target)) {
        return;
    }

    // 更新窗口元信息中的跟随目标
    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window) {
            g_window_meta[i].follow_target = target;
            break;
        }
    }

    // 将窗口与目标设置在同一个父对象下，以实现跟随滚动效果
    lv_obj_t *target_parent = lv_obj_get_parent(target);
    lv_obj_set_parent(window, target_parent);

    // 取消窗口的浮动属性，使其随父对象滚动
    lv_obj_clear_flag(window, LV_OBJ_FLAG_FLOATING);

    // 将窗口置于最前面
    lv_obj_move_foreground(window);
}

/* 禁用窗口保持存活功能 */
void ui_window_disable_keep_alive(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window) {
            g_window_meta[i].keep_alive = false;
            break;
        }
    }
}

/* 显示窗口 */
void ui_window_show(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    // 先隐藏当前窗口（保证同一时间只有一个窗口显示）
    lv_obj_t *current = ui_window_get_current();
    if (current && current != window) {
        ui_window_hide(current);
    }

    // 显示遮罩
    if (ui_window_meta_get_use_mask(window)) {
        ui_window_mask_show_for(window);
    } else {
        ui_window_mask_hide();
    }

    lv_obj_clear_flag(window, LV_OBJ_FLAG_HIDDEN);
    g_current_window = window;
}

/* 隐藏窗口 */
void ui_window_hide(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    // 如果窗口关闭了保持存活，则直接删除对象
    if (!ui_window_meta_get_keep_alive(window)) {
        lv_obj_del(window);
        return;
    }

    lv_obj_add_flag(window, LV_OBJ_FLAG_HIDDEN);
    if (g_current_window == window) {
        g_current_window = NULL;
        ui_window_mask_hide();
    }
}

/* 隐藏当前窗口 */
void ui_window_hide_current(void) {
    ui_window_hide(g_current_window);
}

/* 获取当前显示的窗口 */
lv_obj_t *ui_window_get_current(void) {
    if (g_current_window && lv_obj_is_valid(g_current_window)) {
        return g_current_window;
    }

    g_current_window = NULL;
    return NULL;
}

/* 检查窗口是否可见 */
bool ui_window_is_visible(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return false;
    }

    return !lv_obj_has_flag(window, LV_OBJ_FLAG_HIDDEN);
}
