#include "ui_setting.h"
#include "ui_common.h"
#include "ui_setting_cb.h"
#include "ui_window.h"

static lv_obj_t *g_speed_btns[4]; // 游戏速度按钮数组，供事件回调使用

// 设置窗口创建
lv_obj_t *ui_setting_window_create(void) { 
    /* body: 可滚动 flex 列布局，供各分区堆叠 */
    lv_obj_t *body = ui_div_create(lv_scr_act());
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(body, 12, 0);
    lv_obj_set_style_pad_row(body, 10, 0);
    lv_obj_set_size(body, LV_PCT(100), LV_SIZE_CONTENT);

    /* ======== 音频分区 ======== */
    lv_obj_t *audio_card = ui_card_create_with_flex(body, LV_PCT(100), LV_SIZE_CONTENT);

    /* 分区标题 */
    lv_obj_t *audio_title = ui_label_colored(audio_card, "Audio", lv_color_hex(0x8B4513));
    lv_obj_set_style_text_font(audio_title, &lv_font_montserrat_14, 0);

    /* 音量行：图标符号 + 滑块 + 百分比标签 */
    lv_obj_t *vol_row = ui_transparent_cont_create(audio_card, LV_PCT(100), 36);
    lv_obj_set_flex_flow(vol_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(vol_row, 16, 0);

    lv_obj_t *vol_icon = lv_label_create(vol_row);
    lv_label_set_text(vol_icon, LV_SYMBOL_VOLUME_MID);
    lv_obj_set_style_text_color(vol_icon, lv_color_hex(0x8B4513), 0);

    lv_obj_t *vol_slider = lv_slider_create(vol_row);
    lv_obj_set_size(vol_slider, 160, 8);
    lv_slider_set_range(vol_slider, 0, 100);
    lv_slider_set_value(vol_slider, audio_get_volume(), LV_ANIM_OFF);

    lv_obj_t *vol_label = lv_label_create(vol_row);
    lv_label_set_text_fmt(vol_label, "%d%%", audio_get_volume());
    lv_obj_set_style_text_color(vol_label, lv_color_hex(0x8B4513), 0);

    lv_obj_add_event_cb(vol_slider, ui_setting_volume_slider_cb, LV_EVENT_VALUE_CHANGED, vol_label);

    /* ======== 游戏分区 ======== */
    lv_obj_t *game_card = ui_card_create_with_flex(body, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *game_title = ui_label_colored(game_card, "Game", lv_color_hex(0x8B4513));
    lv_obj_set_style_text_font(game_title, &lv_font_montserrat_14, 0);

    /* 游戏速度：4 个预设按钮 */
    lv_obj_t *speed_label = lv_label_create(game_card);
    lv_label_set_text(speed_label, "Game Speed");
    lv_obj_set_style_text_color(speed_label, lv_color_hex(0x8B4513), 0);

    lv_obj_t *speed_row = ui_transparent_cont_create(game_card, LV_PCT(100), 36);
    lv_obj_set_flex_flow(speed_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(speed_row, 6, 0);

    static const char *speed_labels[] = {"0.5x", "1x", "2x", "5x"};
    for (int i = 0; i < 4; i++) {
        g_speed_btns[i] = lv_btn_create(speed_row);
        lv_obj_set_size(g_speed_btns[i], 52, 30);
        lv_obj_set_style_border_width(g_speed_btns[i], 1, 0);
        lv_obj_set_style_border_color(g_speed_btns[i], lv_color_hex(0x86653a), 0);
        lv_obj_set_style_radius(g_speed_btns[i], 6, 0);
        lv_obj_set_flex_grow(g_speed_btns[i], 1);

        lv_obj_t *btn_label = lv_label_create(g_speed_btns[i]);
        lv_label_set_text(btn_label, speed_labels[i]);
        lv_obj_center(btn_label);

        lv_obj_add_event_cb(g_speed_btns[i], ui_setting_game_speed_cb, LV_EVENT_CLICKED, g_speed_btns);
    }
    /* 默认 1x 高亮 */
    lv_obj_add_style(g_speed_btns[1], &ui_style_btn_yellow, 0);

    /* 重置游戏 */
    lv_obj_t *reset_btn = ui_btn_factory(game_card, LV_PCT(100), 36, "Reset Game", lv_color_hex(0xf4cdca),
                                         lv_color_hex(0xb66258), ui_setting_reset_game_cb, NULL);

    /* ======== 调试分区 ======== */
    lv_obj_t *debug_card = ui_card_create_with_flex(body, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t *debug_title = ui_label_colored(debug_card, "Debug", lv_color_hex(0x8B4513));
    lv_obj_set_style_text_font(debug_title, &lv_font_montserrat_14, 0);

    lv_obj_t *debug_row = ui_transparent_cont_create(debug_card, LV_PCT(100), 36);
    lv_obj_set_flex_flow(debug_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(debug_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(debug_row, 8, 0);

    ui_btn_factory(debug_row, 100, 32, "Add Coins", lv_color_hex(0xefcd76), lv_color_hex(0x8a6333),
                   ui_setting_add_coins_cb, NULL);

    ui_btn_factory(debug_row, 100, 32, "Add Level", lv_color_hex(0xefcd76), lv_color_hex(0x8a6333),
                   ui_setting_add_level_cb, NULL);

    /* ======== 关于分区 ======== */
    lv_obj_t *about_card = ui_card_create_with_flex(body, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_align(about_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *about_title = ui_label_colored(about_card, "About", lv_color_hex(0x8B4513));
    lv_obj_set_style_text_font(about_title, &lv_font_montserrat_14, 0);

    lv_obj_t *name_label = ui_label_colored(about_card, "Drone Farm", lv_color_hex(0x8B4513));
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_20, 0);

    lv_obj_t *ver_label = ui_label_colored(about_card, "v1.0.0", lv_color_hex(0xA0522D));
    lv_obj_set_style_text_font(ver_label, &lv_font_montserrat_14, 0);

    lv_obj_t *copyright_label = ui_label_colored(about_card, "(C) 2026 All Rights Reserved", lv_color_hex(0xA0522D));
    lv_obj_set_style_text_font(copyright_label, &lv_font_montserrat_12, 0);

    lv_obj_t *divider = ui_transparent_cont_create(about_card, LV_PCT(80), 1);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x86653a), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_50, 0);

    lv_obj_t *dev_title = ui_label_colored(about_card, "Developers", lv_color_hex(0x8B4513));
    lv_obj_set_style_text_font(dev_title, &lv_font_montserrat_14, 0);

    lv_obj_t *dev1 = ui_label_colored(about_card, "Liu Junhui  -  UI Design", lv_color_hex(0x6B4226));
    lv_obj_set_style_text_font(dev1, &lv_font_montserrat_12, 0);

    lv_obj_t *dev2 = ui_label_colored(about_card, "Wu Tianyu  -  Game Logic", lv_color_hex(0x6B4226));
    lv_obj_set_style_text_font(dev2, &lv_font_montserrat_12, 0);

    lv_obj_t *div = ui_window_create("SETTING", body, true);
    lv_obj_set_size(div, 300, 400);
    lv_obj_center(div);

    return div;
}
