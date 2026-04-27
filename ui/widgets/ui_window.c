#include "ui_window.h"
#include "ui_common.h"
#include "ui_window_cb.h"

static lv_obj_t *g_current_window = NULL;
static lv_obj_t *g_window_mask = NULL;

#define UI_WINDOW_META_MAX 12
typedef struct {
    lv_obj_t *window;
    bool use_mask;
} ui_window_meta_t;

static ui_window_meta_t g_window_meta[UI_WINDOW_META_MAX];

static void ui_window_meta_set(lv_obj_t *window, bool use_mask) {
    if (!window) {
        return;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window || g_window_meta[i].window == NULL) {
            g_window_meta[i].window = window;
            g_window_meta[i].use_mask = use_mask;
            return;
        }
    }
}

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

static void ui_window_meta_remove(lv_obj_t *window) {
    if (!window) {
        return;
    }

    for (int i = 0; i < UI_WINDOW_META_MAX; i++) {
        if (g_window_meta[i].window == window) {
            g_window_meta[i].window = NULL;
            g_window_meta[i].use_mask = false;
            return;
        }
    }
}

void ui_window_mask_click_handle(void) {
    ui_window_hide_current();
}

static lv_obj_t *ui_window_mask_ensure(lv_obj_t *parent) {
    if (!parent || !lv_obj_is_valid(parent)) {
        return NULL;
    }

    if (g_window_mask && !lv_obj_is_valid(g_window_mask)) {
        g_window_mask = NULL;
    }

    if (!g_window_mask || lv_obj_get_parent(g_window_mask) != parent) {
        if (g_window_mask && lv_obj_is_valid(g_window_mask)) {
            lv_obj_del(g_window_mask);
            g_window_mask = NULL;
        }

        g_window_mask = lv_obj_create(parent);
        lv_obj_set_size(g_window_mask, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_radius(g_window_mask, 0, 0);
        lv_obj_set_style_border_width(g_window_mask, 0, 0);
        lv_obj_set_style_bg_color(g_window_mask, lv_color_make(20, 20, 20), 0);
        lv_obj_set_style_bg_opa(g_window_mask, LV_OPA_40, 0);
        lv_obj_set_style_pad_all(g_window_mask, 0, 0);
        lv_obj_clear_flag(g_window_mask, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(g_window_mask, ui_window_mask_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_HIDDEN);
    }

    return g_window_mask;
}

static void ui_window_mask_show_for(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    lv_obj_t *parent = lv_obj_get_parent(window);
    lv_obj_t *mask = ui_window_mask_ensure(parent);
    if (!mask) {
        return;
    }

    lv_obj_clear_flag(mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(mask);
    lv_obj_move_foreground(window);
}

static void ui_window_mask_hide(void) {
    if (g_window_mask && lv_obj_is_valid(g_window_mask)) {
        lv_obj_add_flag(g_window_mask, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_window_lifecycle_delete_handle(lv_obj_t *target) {
    ui_window_meta_remove(target);

    if (target == g_current_window) {
        g_current_window = NULL;
        ui_window_mask_hide();
    }
}

lv_obj_t *ui_window_create(lv_obj_t *parent, const char *title, lv_obj_t *body, bool enable_mask) {
    lv_obj_t *div = lv_obj_create(parent);
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

    lv_obj_add_event_cb(div, ui_window_lifecycle_delete_cb, LV_EVENT_DELETE, NULL);

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

    ui_window_meta_set(div, enable_mask);

    ui_window_show(div);
    return div;
}

void ui_window_show(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    lv_obj_t *current = ui_window_get_current();
    if (current && current != window) {
        ui_window_hide(current);
    }

    if (ui_window_meta_get_use_mask(window)) {
        ui_window_mask_show_for(window);
    } else {
        ui_window_mask_hide();
    }

    lv_obj_clear_flag(window, LV_OBJ_FLAG_HIDDEN);
    g_current_window = window;
}

void ui_window_hide(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return;
    }

    lv_obj_add_flag(window, LV_OBJ_FLAG_HIDDEN);
    if (g_current_window == window) {
        g_current_window = NULL;
        ui_window_mask_hide();
    }
}

void ui_window_hide_current(void) {
    ui_window_hide(g_current_window);
}

lv_obj_t *ui_window_get_current(void) {
    if (g_current_window && lv_obj_is_valid(g_current_window)) {
        return g_current_window;
    }

    g_current_window = NULL;
    return NULL;
}

bool ui_window_is_visible(lv_obj_t *window) {
    if (!window || !lv_obj_is_valid(window)) {
        return false;
    }

    return !lv_obj_has_flag(window, LV_OBJ_FLAG_HIDDEN);
}
