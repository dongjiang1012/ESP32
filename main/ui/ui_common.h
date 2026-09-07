#pragma once
#include <stddef.h>
#include "lvgl.h"
#include "ui_fonts.h"
#include "esp_err.h"
#include "sw6306_power_types.h"

#define UI_BG       0x101820
#define UI_CARD     0x1B2833
#define UI_TEXT     0xE7F0F5
#define UI_MUTED    0x9FB2C2
#define UI_ACCENT   0x50D7AC
#define UI_WARNING  0xFFC66B
#define UI_ERROR    0xFF8888

lv_obj_t *ui_label(lv_obj_t *parent, const char *text, int x, int y, int width, uint32_t color);
lv_obj_t *ui_card(lv_obj_t *parent, int x, int y, int w, int h);
lv_obj_t *ui_button(lv_obj_t *parent, const char *text, int x, int y, int w,
                    lv_event_cb_t callback, void *user_data);
void ui_set_enabled(lv_obj_t *object, bool enabled);
void ui_set_checked(lv_obj_t *object, bool checked);
const char *ui_error_text(esp_err_t error);
void ui_format_number(char *buffer, size_t size, int32_t value, uint8_t decimals);
void ui_confirm_open(const char *title, const char *message,
                     void (*accept)(void *), void *context);
