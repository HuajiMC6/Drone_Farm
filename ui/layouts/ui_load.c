#include "audio.h"
#include "ui.h"
#include "ui_load_cb.h"

lv_obj_t *ui_load_screen_create(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* 没有图片资源时将展示的低配版加载页 */

    lv_obj_t *no_img_title = lv_label_create(screen);
    lv_label_set_text(no_img_title, "Drone Farm");
    lv_obj_set_style_text_font(no_img_title, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(no_img_title, lv_color_hex(0x8a6333), 0);
    lv_obj_align_to(no_img_title, screen, LV_ALIGN_TOP_MID, 0, 65);

    lv_obj_t *no_img_description = lv_label_create(screen);
    lv_label_set_text(no_img_description,
                      "Since you don't have the image resources, here's a extremely simple loading screen for you!");
    lv_obj_center(no_img_description);
    lv_obj_align_to(no_img_description, screen, LV_ALIGN_TOP_MID, 0, 160);

    lv_obj_t *no_img_btn = lv_btn_create(screen);
    lv_obj_set_size(no_img_btn, 272, 85);
    lv_obj_align_to(no_img_btn, screen, LV_ALIGN_BOTTOM_MID, 0, -75);
    lv_obj_set_style_bg_color(no_img_btn, lv_color_hex(0xfac757), 0);
    lv_obj_set_style_border_color(no_img_btn, lv_color_hex(0x8a6333), 0);
    lv_obj_t *btn_label = lv_label_create(no_img_btn);
    lv_label_set_text(btn_label, "Start Game");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_30, 0);
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(no_img_btn, icon_btns_click_audio_play, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(no_img_btn, ui_load_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 有图片资源时展示的高脂加载页 */

    lv_obj_t *bg = lv_img_create(screen);
    lv_obj_set_size(bg, 1024, 600);
    lv_img_set_src(bg, img_load_bg);

    lv_obj_t *btn = lv_btn_create(bg);
    lv_obj_set_size(btn, 272, 85);
    lv_obj_set_style_bg_img_src(btn, img_load_btn, 0);
    lv_obj_set_style_bg_img_src(btn, img_load_btn_pressed, LV_STATE_PRESSED);
    lv_obj_align_to(btn, bg, LV_ALIGN_BOTTOM_MID, 0, -75);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(btn, icon_btns_click_audio_play, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn, ui_load_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 开发团队信息 */
    lv_obj_t *dev_info = lv_label_create(bg);
    lv_label_set_text(dev_info,
                      "Developed by Liu Junhui & Wu Tianyu. \nPowered by LVGL v8.2. \n2026 (c) All rights reserved.");
    lv_obj_set_style_text_font(dev_info, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(dev_info, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_color(dev_info, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_bg_opa(dev_info, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(dev_info, 6, 0);
    lv_obj_set_style_radius(dev_info, 5, 0);
    lv_obj_align_to(dev_info, bg, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    return screen;
}
