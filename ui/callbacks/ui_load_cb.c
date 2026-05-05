#include "ui_load_cb.h"
#include "player.h"
#include "ui.h"

void ui_load_test_cb(lv_event_t *e) {
    ui_screen_switch(UI_SCREEN_MAIN);
    player_t *player = player_get_instance();
    player->coins += 1000000;
}