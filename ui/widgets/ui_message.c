#include "ui_message.h"

/* 消息容器 */
static lv_obj_t *g_message_box = NULL;
/* 消息列表 */
static lv_obj_t *g_message_list[UI_MESSAGE_MAX] = {0};

/* ──────────────── 消息队列管理 ──────────────── */

/* 从队列中移除指定消息，并紧凑剩余元素 */
static void ui_message_queue_remove(lv_obj_t *msg_obj) {
    int idx = -1;
    for (int i = 0; i < UI_MESSAGE_MAX; i++) {
        if (g_message_list[i] == msg_obj) {
            idx = i;
            g_message_list[i] = NULL;
            break;
        }
    }
    if (idx < 0)
        return;
    /* 左移后续元素，保持队列无空洞 */
    for (int i = idx; i < UI_MESSAGE_MAX - 1; i++) {
        g_message_list[i] = g_message_list[i + 1];
    }
    g_message_list[UI_MESSAGE_MAX - 1] = NULL;
}

/* 对象删除回调：自动从消息队列中移除 */
static void ui_message_on_delete_cb(lv_event_t *e) {
    lv_obj_t *msg_obj = lv_event_get_target(e);
    ui_message_queue_remove(msg_obj);
}

/* 消息入队：加入第一个空槽；若队列满则移除最早的并插入 */
static void ui_message_queue_push(lv_obj_t *msg_obj) {
    /* 查找空槽 */
    for (int i = 0; i < UI_MESSAGE_MAX; i++) {
        if (!g_message_list[i] || !lv_obj_is_valid(g_message_list[i])) {
            g_message_list[i] = msg_obj;
            return;
        }
    }
    /* 队列满：手动移除最早消息再追加，避免依赖DELETE 回调时序 */
    lv_obj_t *oldest = g_message_list[0];
    ui_message_queue_remove(oldest);
    lv_obj_del(oldest);
    g_message_list[UI_MESSAGE_MAX - 1] = msg_obj;
}

/* ──────────────── 内部回调 ──────────────── */

static void ui_message_confirm_btn_click_cb(lv_event_t *e) {
    lv_obj_t *confirm = lv_event_get_user_data(e);
    if (confirm && lv_obj_is_valid(confirm)) {
        lv_obj_fade_out(confirm, 500, 0); // 立即开始淡出
        lv_obj_del_delayed(confirm, 500); // 淡出动画结束后删除对象
    }
}

/* ———————————————— 工具函数 ———————————————— */

static lv_color_t get_bg_color(ui_message_style_t style) {
    switch (style) {
        case UI_MESSAGE_TYPE_INFO:
            return UI_MESSAGE_COLOR_INFO;
        case UI_MESSAGE_TYPE_WARNING:
            return UI_MESSAGE_COLOR_WARNING;
        case UI_MESSAGE_TYPE_SUCCESS:
            return UI_MESSAGE_COLOR_SUCCESS;
        case UI_MESSAGE_TYPE_ERROR:
            return UI_MESSAGE_COLOR_ERROR;
        default:
            return lv_color_white();
    }
}

static lv_color_t get_border_color(ui_message_style_t style) {
    switch (style) {
        case UI_MESSAGE_TYPE_INFO:
            return UI_MESSAGE_BORDER_COLOR_INFO;
        case UI_MESSAGE_TYPE_WARNING:
            return UI_MESSAGE_BORDER_COLOR_WARNING;
        case UI_MESSAGE_TYPE_SUCCESS:
            return UI_MESSAGE_BORDER_COLOR_SUCCESS;
        case UI_MESSAGE_TYPE_ERROR:
            return UI_MESSAGE_BORDER_COLOR_ERROR;
        default:
            return lv_color_white();
    }
}

static lv_color_t get_text_color(ui_message_style_t style) {
    switch (style) {
        case UI_MESSAGE_TYPE_INFO:
            return UI_MESSAGE_TEXT_COLOR_INFO;
        case UI_MESSAGE_TYPE_WARNING:
            return UI_MESSAGE_TEXT_COLOR_WARNING;
        case UI_MESSAGE_TYPE_SUCCESS:
            return UI_MESSAGE_TEXT_COLOR_SUCCESS;
        case UI_MESSAGE_TYPE_ERROR:
            return UI_MESSAGE_TEXT_COLOR_ERROR;
        default:
            return lv_color_white();
    }
}

/* ──────────────── 消息对象创建 ──────────────── */

/* Toast 消息对象创建 */
static lv_obj_t *toast_create(const char *message, ui_message_style_t style) {
    lv_obj_t *toast = lv_obj_create(g_message_box);
    lv_obj_set_size(toast, LV_PCT(40), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(toast, 5, 0);
    lv_obj_set_style_pad_all(toast, 10, 0);
    lv_obj_set_style_border_width(toast, 1, 0);

    lv_obj_t *label = lv_label_create(toast);
    lv_label_set_text(label, message);
    lv_obj_set_align(label, LV_ALIGN_LEFT_MID);

    // 上色
    lv_obj_set_style_bg_color(toast, get_bg_color(style), 0);
    lv_obj_set_style_border_color(toast, get_border_color(style), 0);
    lv_obj_set_style_text_color(label, get_text_color(style), 0);

    return toast;
}

/* Confirm 消息对象创建 */
static lv_obj_t *confirm_create(const char *message, ui_message_style_t style) {
    lv_obj_t *confirm = toast_create(message, style);

    // 在Toast的基础上添加一个确认按钮，点击后淡出并删除消息对象
    lv_obj_t *confirm_btn = lv_btn_create(confirm);
    lv_obj_set_size(confirm_btn, 30, 20);
    lv_obj_set_align(confirm_btn, LV_ALIGN_RIGHT_MID);
    lv_obj_set_style_bg_color(confirm_btn, get_border_color(style), 0);
    lv_obj_set_style_radius(confirm_btn, 5, 0);
    lv_obj_add_event_cb(confirm_btn, ui_message_confirm_btn_click_cb, LV_EVENT_CLICKED, confirm);

    lv_obj_t *btn_label = lv_label_create(confirm_btn);
    lv_label_set_text(btn_label, "OK");
    lv_obj_center(btn_label);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);

    return confirm;
}

/* 创建消息对象 */
static lv_obj_t *message_create(const char *message, ui_message_style_t style, ui_message_type_t type) {
    if (!g_message_box || !lv_obj_is_valid(g_message_box)) {
        g_message_box = lv_obj_create(lv_layer_top());
        lv_obj_set_size(g_message_box, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_flow(g_message_box, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(g_message_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(g_message_box, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_message_box, 0, 0);
        lv_obj_set_style_pad_all(g_message_box, 25, 0);
        lv_obj_clear_flag(g_message_box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(g_message_box, LV_OBJ_FLAG_CLICKABLE); // 让事件穿透到下层，防止阻碍点击
    }
    lv_obj_move_foreground(g_message_box); // 确保消息容器在最前面

    // 创建消息对象
    lv_obj_t *msg;
    switch (type) {
        case UI_MESSAGE_TOAST:
            msg = toast_create(message, style);
            lv_obj_fade_out(msg, 500, UI_MESSAGE_TOAST_DURATION);     // UI_MESSAGE_TOAST_DURATION 毫秒后开始淡出
            lv_obj_del_delayed(msg, UI_MESSAGE_TOAST_DURATION + 500); // 淡出动画结束后删除对象
            break;
        case UI_MESSAGE_CONFIRM:
            msg = confirm_create(message, style);
            break;
        default:
            return NULL;
    }

    /* 绑定删除事件，消息销毁时自动出队 */
    lv_obj_add_event_cb(msg, ui_message_on_delete_cb, LV_EVENT_DELETE, NULL);

    return msg;
}

/* ──────────────── 公开接口 ──────────────── */

void ui_message_show(const char *message, ui_message_style_t style, ui_message_type_t type) {
    if (!message)
        return;

    lv_obj_t *msg = message_create(message, style, type);
    if (!msg)
        return;

    ui_message_queue_push(msg);
}
