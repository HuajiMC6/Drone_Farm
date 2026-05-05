#include "ui.h"

lv_obj_t *ui_load_screen_create(void);
lv_obj_t *ui_main_screen_create(void);
void ui_main_handle_event(event_t *event);
void ui_main_update_timer_init(void);

static lv_obj_t *g_screen_load = NULL;
static lv_obj_t *g_screen_main = NULL;

void ui_init(void) {
    g_screen_load = ui_load_screen_create();
    g_screen_main = ui_main_screen_create();

    ui_screen_switch(UI_SCREEN_LOAD);
}

void ui_screen_switch(ui_screen_type_t screen) {
    switch (screen) {
        case UI_SCREEN_LOAD:
            if (g_screen_load) {
                lv_scr_load(g_screen_load);
            }
            break;
        case UI_SCREEN_MAIN:
            if (g_screen_main) {
                lv_scr_load(g_screen_main);
            }
            break;
        default:
            if (g_screen_main) {
                lv_scr_load(g_screen_main);
            }
            break;
    }
}

void ui_event_handler(event_t *event) {
    if (!event) {
        return;
    }

    ui_main_handle_event(event);
}

void ui_update_timer_init(void) {
    ui_main_update_timer_init();
}
