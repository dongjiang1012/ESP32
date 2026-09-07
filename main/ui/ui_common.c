#include "ui_common.h"
#include <stdio.h>

const char *ui_error_text(esp_err_t error)
{
    switch (error) {
    case ESP_OK: return "正常";
    case ESP_FAIL: return "操作失败";
    case ESP_ERR_NO_MEM: return "内存不足";
    case ESP_ERR_INVALID_ARG: return "参数无效";
    case ESP_ERR_INVALID_STATE: return "当前状态不可用";
    case ESP_ERR_INVALID_SIZE: return "数据长度无效";
    case ESP_ERR_NOT_FOUND: return "设备或资源未找到";
    case ESP_ERR_NOT_SUPPORTED: return "不支持此操作";
    case ESP_ERR_TIMEOUT: return "通信超时";
    case ESP_ERR_INVALID_RESPONSE: return "回读结果不一致";
    default: return "未识别的设备错误";
    }
}

lv_obj_t *ui_label(lv_obj_t *parent, const char *text, int x, int y, int width, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    if (width > 0) lv_obj_set_width(label, width);
    lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

lv_obj_t *ui_card(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, lv_color_hex(UI_CARD), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

lv_obj_t *ui_button(lv_obj_t *parent, const char *text, int x, int y, int w,
                    lv_event_cb_t callback, void *user_data)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, w, 44);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2D4353), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x26775E), LV_STATE_CHECKED);
    lv_obj_set_style_opa(button, LV_OPA_40, LV_STATE_DISABLED);
    lv_obj_t *label = ui_label(button, text, 0, 0, 0, UI_TEXT);
    lv_obj_center(label);
    if (callback != NULL) lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);
    return button;
}

void ui_set_enabled(lv_obj_t *obj, bool enabled)
{
    if (enabled) lv_obj_remove_state(obj, LV_STATE_DISABLED);
    else lv_obj_add_state(obj, LV_STATE_DISABLED);
}

void ui_set_checked(lv_obj_t *obj, bool checked)
{
    if (checked) lv_obj_add_state(obj, LV_STATE_CHECKED);
    else lv_obj_remove_state(obj, LV_STATE_CHECKED);
}

void ui_format_number(char *buffer, size_t size, int32_t value, uint8_t decimals)
{
    int32_t scale = 1;
    for (uint8_t i = 0; i < decimals; ++i) scale *= 10;
    int64_t magnitude = value < 0 ? -(int64_t)value : value;
    if (decimals == 0) snprintf(buffer, size, "%ld", (long)value);
    else snprintf(buffer, size, "%s%ld.%0*ld", value < 0 ? "-" : "",
                  (long)(magnitude / scale), decimals, (long)(magnitude % scale));
}

static lv_obj_t *s_confirm;
static void (*s_accept)(void *);
static void *s_accept_context;

static void confirm_event(lv_event_t *event)
{
    bool accepted = lv_event_get_user_data(event) != NULL;
    lv_obj_t *overlay = s_confirm;
    s_confirm = NULL;
    lv_obj_delete(overlay);
    if (accepted && s_accept != NULL) s_accept(s_accept_context);
}

void ui_confirm_open(const char *title, const char *message,
                     void (*accept)(void *), void *context)
{
    if (s_confirm != NULL) return;
    s_accept = accept;
    s_accept_context = context;
    s_confirm = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_confirm);
    lv_obj_set_size(s_confirm, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_confirm, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_confirm, LV_OPA_70, 0);
    lv_obj_add_flag(s_confirm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *card = ui_card(s_confirm, 0, 0, 640, 420);
    lv_obj_center(card);
    lv_obj_t *heading = ui_label(card, title, 8, 8, 580, UI_TEXT);
    lv_obj_set_style_text_font(heading, UI_FONT_LARGE, 0);
    ui_label(card, message, 8, 54, 580, UI_MUTED);
    ui_button(card, "取消", 8, 334, 280, confirm_event, NULL);
    ui_button(card, "确认", 310, 334, 280, confirm_event, card);
}
