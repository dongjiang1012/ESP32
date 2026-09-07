#include "ui_numeric_input.h"
#include "ui_common.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static struct {
    lv_obj_t *overlay;
    lv_obj_t *entry;
    lv_obj_t *error;
    const sw6306_numeric_spec_t *spec;
    ui_numeric_result_cb_t callback;
    void *context;
} s_input;

static const char *const s_keys[] = {"1", "2", "3", "\n", "4", "5", "6", "\n",
                                     "7", "8", "9", "\n", ".", "0", "退格", ""};

static void close_input(void)
{
    lv_obj_t *overlay = s_input.overlay;
    s_input.overlay = NULL;
    if (overlay != NULL) lv_obj_delete(overlay);
}

/* Fixed-point parsing: no float rounding, exponents, signs or silent snapping. */
static bool parse_number(const char *text, uint8_t decimals, int32_t *result)
{
    int64_t value = 0;
    unsigned fraction = 0;
    bool point = false, digit = false;
    for (const char *p = text; *p; ++p) {
        if (*p == '.' && !point) { point = true; continue; }
        if (*p < '0' || *p > '9') return false;
        digit = true;
        if (point && ++fraction > decimals) return false;
        value = value * 10 + (*p - '0');
        if (value > INT32_MAX) return false;
    }
    if (!digit) return false;
    while (fraction++ < decimals) {
        value *= 10;
        if (value > INT32_MAX) return false;
    }
    *result = (int32_t)value;
    return true;
}

static void accept_cb(lv_event_t *event)
{
    (void)event;
    int32_t value;
    if (!parse_number(lv_textarea_get_text(s_input.entry), s_input.spec->decimals, &value)) {
        lv_label_set_text_fmt(s_input.error, "请输入数字，最多保留 %u 位小数。",
                              (unsigned)s_input.spec->decimals);
        return;
    }
    if (!sw6306_numeric_valid(s_input.spec, value)) {
        lv_label_set_text(s_input.error, "数值须在允许范围内，并符合调整步进。");
        return;
    }
    ui_numeric_result_cb_t callback = s_input.callback;
    void *context = s_input.context;
    close_input();
    if (callback != NULL) callback(value, context);
}

static void cancel_cb(lv_event_t *event) { (void)event; close_input(); }

static void key_cb(lv_event_t *event)
{
    lv_obj_t *keys = lv_event_get_target_obj(event);
    uint32_t index = lv_buttonmatrix_get_selected_button(keys);
    const char *key = lv_buttonmatrix_get_button_text(keys, index);
    if (key == NULL) return;
    if (strcmp(key, "退格") == 0) lv_textarea_delete_char(s_input.entry);
    else lv_textarea_add_text(s_input.entry, key);
    lv_label_set_text(s_input.error, "");
}

static void clear_cb(lv_event_t *event) { (void)event; lv_textarea_set_text(s_input.entry, ""); }

bool ui_numeric_input_is_open(void) { return s_input.overlay != NULL; }

void ui_numeric_input_open(const sw6306_numeric_spec_t *spec, int32_t current,
                           ui_numeric_result_cb_t callback, void *context)
{
    if (spec == NULL || s_input.overlay != NULL) return;
    s_input.spec = spec;
    s_input.callback = callback;
    s_input.context = context;
    s_input.overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_input.overlay);
    lv_obj_set_size(s_input.overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_input.overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_input.overlay, LV_OPA_70, 0);
    lv_obj_add_flag(s_input.overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *panel = ui_card(s_input.overlay, 0, 0, 580, 552);
    lv_obj_center(panel);
    lv_obj_t *title = ui_label(panel, spec->title, 4, 0, 530, UI_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, 0);
    char minimum[24], maximum[24], step[24], current_text[24], hint[160];
    ui_format_number(minimum, sizeof(minimum), spec->minimum, spec->decimals);
    ui_format_number(maximum, sizeof(maximum), spec->maximum, spec->decimals);
    ui_format_number(step, sizeof(step), spec->step, spec->decimals);
    ui_format_number(current_text, sizeof(current_text), current, spec->decimals);
    snprintf(hint, sizeof(hint), "范围 %s～%s %s  |  步进 %s %s", minimum, maximum, spec->unit, step, spec->unit);
    ui_label(panel, hint, 4, 36, 535, UI_MUTED);
    s_input.entry = lv_textarea_create(panel);
    lv_obj_set_pos(s_input.entry, 4, 68);
    lv_obj_set_size(s_input.entry, 418, 58);
    lv_textarea_set_one_line(s_input.entry, true);
    lv_textarea_set_max_length(s_input.entry, 10);
    lv_textarea_set_accepted_chars(s_input.entry, "0123456789.");
    lv_textarea_set_text(s_input.entry, current_text);
    lv_obj_set_style_text_font(s_input.entry, UI_FONT_LARGE, 0);
    ui_button(panel, "清空", 434, 76, 106, clear_cb, NULL);
    lv_obj_t *keys = lv_buttonmatrix_create(panel);
    lv_obj_set_pos(keys, 4, 140);
    lv_obj_set_size(keys, 536, 278);
    lv_buttonmatrix_set_map(keys, s_keys);
    lv_obj_set_style_text_font(keys, UI_FONT_LARGE, LV_PART_ITEMS);
    lv_obj_add_event_cb(keys, key_cb, LV_EVENT_VALUE_CHANGED, NULL);
    s_input.error = ui_label(panel, "", 4, 426, 535, UI_WARNING);
    ui_button(panel, "取消", 4, 474, 258, cancel_cb, NULL);
    ui_button(panel, "确认数值", 278, 474, 262, accept_cb, NULL);
}
