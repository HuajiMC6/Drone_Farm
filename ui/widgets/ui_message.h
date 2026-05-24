#ifndef __UI_MESSAGE_H
#define __UI_MESSAGE_H

#include "lvgl.h"

#define UI_MESSAGE_MAX 5               // 最多同时显示的消息数量
#define UI_MESSAGE_TOAST_DURATION 2500 // Toast消息显示时长（毫秒）

/* 消息样式 */
typedef enum {
    UI_MESSAGE_TYPE_INFO,    // 一般信息，灰色风格
    UI_MESSAGE_TYPE_WARNING, // 警告信息，黄色风格
    UI_MESSAGE_TYPE_SUCCESS, // 成功信息，蓝色风格
    UI_MESSAGE_TYPE_ERROR,   // 错误信息，红色风格
} ui_message_style_t;

/* 消息类型 */
typedef enum {
    UI_MESSAGE_TOAST,   // 短暂显示后自动消失
    UI_MESSAGE_CONFIRM, // 需要用户确认
} ui_message_type_t;

/* 消息颜色定义 */
#define UI_MESSAGE_COLOR_INFO lv_color_hex(0xDCDCDC)
#define UI_MESSAGE_COLOR_WARNING lv_color_hex(0xFFE7A8)
#define UI_MESSAGE_COLOR_SUCCESS lv_color_hex(0xD9EEFF)
#define UI_MESSAGE_COLOR_ERROR lv_color_hex(0xF0C1C7)

#define UI_MESSAGE_BORDER_COLOR_INFO lv_color_hex(0x969696)
#define UI_MESSAGE_BORDER_COLOR_WARNING lv_color_hex(0xC9A227)
#define UI_MESSAGE_BORDER_COLOR_SUCCESS lv_color_hex(0x3E95E8)
#define UI_MESSAGE_BORDER_COLOR_ERROR lv_color_hex(0xC94857)

#define UI_MESSAGE_TEXT_COLOR_INFO lv_color_hex(0x4A4A4A)
#define UI_MESSAGE_TEXT_COLOR_WARNING lv_color_hex(0x6B5600)
#define UI_MESSAGE_TEXT_COLOR_SUCCESS lv_color_hex(0x155A99)
#define UI_MESSAGE_TEXT_COLOR_ERROR lv_color_hex(0x7A1F2B)

/* 显示消息 */
void ui_message_show(const char *message, ui_message_style_t style, ui_message_type_t type);

#endif