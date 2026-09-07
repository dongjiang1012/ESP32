#include "ui_brightness_panel.h"
#include "ui_common.h"
#include "ui_page_manager.h"
#include "bsp/display.h"

#define PANEL_HEIGHT 220
#define PANEL_ANIMATION_MS 180

static struct {
    lv_obj_t *overlay;
    lv_obj_t *panel;
    lv_obj_t *slider;
    lv_obj_t *value;
    int percent;
    int32_t start_y;
    bool tracking;
} s_panel = {.percent = 50};

int ui_brightness_panel_get_percent(void) { return s_panel.percent; }

esp_err_t ui_brightness_panel_set_percent(int percent)
{
    if (percent < 5 || percent > 100) return ESP_ERR_INVALID_ARG;
    esp_err_t err = bsp_display_brightness_set(percent);
    if (err == ESP_OK) {
        s_panel.percent = percent;
        if (s_panel.slider != NULL) lv_slider_set_value(s_panel.slider, percent, LV_ANIM_OFF);
        if (s_panel.value != NULL) lv_label_set_text_fmt(s_panel.value, "%d%%", percent);
    }
    return err;
}

static void panel_set_y(void *obj, int32_t y) { lv_obj_set_y(obj, y); }

static void close_panel(void)
{
    lv_anim_delete(s_panel.panel, panel_set_y);
    lv_obj_add_flag(s_panel.overlay, LV_OBJ_FLAG_HIDDEN);
}

static void open_panel(void)
{
    lv_obj_remove_flag(s_panel.overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_panel.overlay);
    lv_anim_delete(s_panel.panel, panel_set_y);
    lv_obj_set_y(s_panel.panel, -PANEL_HEIGHT);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, s_panel.panel);
    lv_anim_set_exec_cb(&animation, panel_set_y);
    lv_anim_set_values(&animation, -PANEL_HEIGHT, 0);
    lv_anim_set_duration(&animation, PANEL_ANIMATION_MS);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_start(&animation);
}

static void pull_cb(lv_event_t *event)
{
    lv_indev_t *input = lv_indev_active();
    if (input == NULL) return;
    lv_point_t point;
    lv_indev_get_point(input, &point);
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
        s_panel.start_y = point.y;
        s_panel.tracking = true;
    } else if (code == LV_EVENT_PRESSING && s_panel.tracking && point.y - s_panel.start_y >= 30) {
        s_panel.tracking = false;
        open_panel();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_panel.tracking = false;
    }
}

static void close_cb(lv_event_t *event) { (void)event; close_panel(); }

static void overlay_cb(lv_event_t *event)
{
    if (lv_event_get_target_obj(event) == s_panel.overlay) close_panel();
}

static void settings_cb(lv_event_t *event)
{
    (void)event;
    close_panel();
    ui_page_manager_show(UI_PAGE_SYSTEM);
}

static void slider_cb(lv_event_t *event)
{
    int percent = (int)lv_slider_get_value(lv_event_get_target_obj(event));
    if (ui_brightness_panel_set_percent(percent) != ESP_OK) {
        lv_slider_set_value(s_panel.slider, s_panel.percent, LV_ANIM_OFF);
        lv_label_set_text(s_panel.value, "亮度设置失败");
    }
}

void ui_brightness_panel_create(lv_obj_t *screen)
{
    if (screen == NULL) return;
    s_panel.overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(s_panel.overlay);
    lv_obj_set_size(s_panel.overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_panel.overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_panel.overlay, LV_OPA_50, 0);
    lv_obj_add_flag(s_panel.overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_panel.overlay, overlay_cb, LV_EVENT_CLICKED, NULL);
    s_panel.panel = ui_card(s_panel.overlay, 0, 0, 1024, PANEL_HEIGHT);
    lv_obj_set_style_radius(s_panel.panel, 0, 0);
    ui_label(s_panel.panel, "屏幕亮度", 24, 12, 650, UI_TEXT);
    s_panel.value = ui_label(s_panel.panel, "50%", 826, 12, 142, UI_ACCENT);
    s_panel.slider = lv_slider_create(s_panel.panel);
    lv_obj_set_pos(s_panel.slider, 40, 70);
    lv_obj_set_size(s_panel.slider, 902, 26);
    lv_slider_set_range(s_panel.slider, 5, 100);
    lv_slider_set_value(s_panel.slider, s_panel.percent, LV_ANIM_OFF);
    lv_obj_add_event_cb(s_panel.slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    ui_button(s_panel.panel, "系统设置", 24, 132, 240, settings_cb, NULL);
    ui_button(s_panel.panel, "关闭", 780, 132, 184, close_cb, NULL);
    if (ui_brightness_panel_set_percent(s_panel.percent) != ESP_OK) lv_label_set_text(s_panel.value, "亮度设置失败");
    lv_obj_add_flag(s_panel.overlay, LV_OBJ_FLAG_HIDDEN);

    /* Only the top edge captures the pull gesture; navigation stays clickable. */
    lv_obj_t *pull = lv_obj_create(screen);
    lv_obj_remove_style_all(pull);
    lv_obj_set_size(pull, lv_pct(100), 12);
    lv_obj_add_flag(pull, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(pull, pull_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(pull, pull_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(pull, pull_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(pull, pull_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_t *handle = lv_obj_create(pull);
    lv_obj_remove_style_all(handle);
    lv_obj_set_size(handle, 64, 3);
    lv_obj_align(handle, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(handle, lv_color_hex(UI_MUTED), 0);
    lv_obj_set_style_bg_opa(handle, LV_OPA_COVER, 0);
    lv_obj_remove_flag(handle, LV_OBJ_FLAG_CLICKABLE);
}
