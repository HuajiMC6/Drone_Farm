#include "ui_window.h"
#include "ui_common.h"

static lv_obj_t *g_current_window = NULL;

static void ui_window_delete_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target == g_current_window) {
        g_current_window = NULL;
    }
}

lv_obj_t *ui_window_create(lv_obj_t *parent, const char *title, lv_obj_t *body) {
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
