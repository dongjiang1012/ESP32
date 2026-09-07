#include "ui_config_pages.h"
#include "ui_brightness_panel.h"

static const char *const s_roles[] = {"双向（优先供电）", "供电端", "受电端", ""};

static const char *role_name(sw6306_c1_role_t role)
{
    return (unsigned)role < SW6306_C1_ROLE_RESERVED ? s_roles[role] : "未知";
}

static void role_changed_cb(lv_event_t *event)
{
    ui_system_page_t *p = lv_event_get_user_data(event);
    uint32_t selected = lv_buttonmatrix_get_selected_button(p->roles);
    if (selected >= SW6306_C1_ROLE_RESERVED) return;
    sw6306_config_model_t *m = p->base.model;
    m->c1_role = (sw6306_c1_role_t)selected;
    m->c1_role_dirty = m->c1_role != m->c1_role_actual;
    lv_label_set_text(p->base.status, m->c1_role_dirty ? "C1 角色草稿已修改，尚未应用" : "与上次回读配置一致");
    lv_obj_set_style_text_color(p->base.status, lv_color_hex(UI_ACCENT), 0);
    ui_system_settings_refresh(p, false);
}

static void brightness_cb(lv_event_t *event)
{
    ui_system_page_t *p = lv_event_get_user_data(event);
    esp_err_t err = ui_brightness_panel_set_percent((int)lv_slider_get_value(p->brightness));
    if (err != ESP_OK) {
        lv_label_set_text_fmt(p->brightness_value, "亮度设置失败：%s", ui_error_text(err));
        lv_slider_set_value(p->brightness, ui_brightness_panel_get_percent(), LV_ANIM_OFF);
    } else lv_label_set_text_fmt(p->brightness_value, "%d%%", ui_brightness_panel_get_percent());
}

static void reload_confirmed(void *context)
{
    ui_system_page_t *p = context;
    p->base.request(SW6306_LOAD_ROLE);
}

static void reload_cb(lv_event_t *event)
{
    ui_system_page_t *p = lv_event_get_user_data(event);
    if (p->base.model->c1_role_dirty) ui_confirm_open("覆盖 C1 角色草稿？", "重新读取将用芯片配置覆盖尚未应用的 C1 角色。", reload_confirmed, p);
    else reload_confirmed(p);
}

static void apply_cb(lv_event_t *event)
{
    ui_system_page_t *p = lv_event_get_user_data(event);
    p->base.request(SW6306_APPLY_ROLE);
}

static void discard_cb(lv_event_t *event)
{
    ui_system_page_t *p = lv_event_get_user_data(event);
    p->base.model->c1_role = p->base.model->c1_role_actual;
    p->base.model->c1_role_dirty = false;
    lv_label_set_text(p->base.status, "已放弃 C1 修改，与上次回读一致");
    ui_system_settings_refresh(p, false);
}

void ui_system_settings_refresh(ui_system_page_t *p, bool busy)
{
    sw6306_config_model_t *m = p->base.model;
    bool editable = m->c1_role_valid && !busy;
    lv_buttonmatrix_clear_button_ctrl_all(p->roles, LV_BUTTONMATRIX_CTRL_CHECKED);
    if (m->c1_role_valid && (unsigned)m->c1_role < SW6306_C1_ROLE_RESERVED)
        lv_buttonmatrix_set_button_ctrl(p->roles, m->c1_role, LV_BUTTONMATRIX_CTRL_CHECKED);
    ui_set_enabled(p->roles, editable);
    ui_set_enabled(p->base.reload, !busy);
    ui_set_enabled(p->base.discard, editable && m->c1_role_dirty);
    ui_set_enabled(p->base.apply, editable && m->c1_role_dirty);
    lv_label_set_text_fmt(p->base.actual, "C1 角色：%s\n\n调整亮度立即生效。\n芯片配置通过读取、应用操作。\n\n本阶段不保存设置到闪存。\n重启后重新读取芯片配置。",
                          m->c1_role_valid ? role_name(m->c1_role_actual) : "尚未读取");
    int percent = ui_brightness_panel_get_percent();
    if (lv_slider_get_value(p->brightness) != percent) {
        lv_slider_set_value(p->brightness, percent, LV_ANIM_OFF);
        lv_label_set_text_fmt(p->brightness_value, "%d%%", percent);
    }
}

void ui_system_settings_create(ui_system_page_t *p, lv_obj_t *parent,
                               sw6306_config_model_t *model, ui_config_request_cb_t request)
{
    p->base = (ui_config_page_t){.root = parent, .model = model, .request = request};
    lv_obj_t *title = ui_label(parent, "系统设置", 24, 12, 580, UI_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, 0);
    p->base.status = ui_label(parent, "读取芯片后可配置 C1 角色", 24, 44, 950, UI_ACCENT);
    p->base.form = ui_card(parent, 24, 78, 600, 354);
    ui_label(p->base.form, "屏幕亮度", 0, 0, 300, UI_TEXT);
    p->brightness_value = ui_label(p->base.form, "50%", 310, 0, 252, UI_ACCENT);
    p->brightness = lv_slider_create(p->base.form);
    lv_obj_set_pos(p->brightness, 20, 56);
    lv_obj_set_size(p->brightness, 524, 26);
    lv_slider_set_range(p->brightness, 5, 100);
    lv_slider_set_value(p->brightness, ui_brightness_panel_get_percent(), LV_ANIM_OFF);
    lv_obj_add_event_cb(p->brightness, brightness_cb, LV_EVENT_VALUE_CHANGED, p);
    ui_label(p->base.form, "Type-C / C1 端口角色", 0, 124, 560, UI_TEXT);
    p->roles = lv_buttonmatrix_create(p->base.form);
    lv_obj_set_pos(p->roles, 0, 158);
    lv_obj_set_size(p->roles, 566, 60);
    lv_buttonmatrix_set_map(p->roles, s_roles);
    lv_buttonmatrix_set_button_ctrl_all(p->roles, LV_BUTTONMATRIX_CTRL_CHECKABLE);
    lv_buttonmatrix_set_one_checked(p->roles, true);
    lv_obj_set_style_pad_all(p->roles, 0, 0);
    lv_obj_set_style_border_width(p->roles, 0, 0);
    lv_obj_set_style_bg_color(p->roles, lv_color_hex(UI_CARD), 0);
    lv_obj_set_style_bg_color(p->roles, lv_color_hex(0x2D4353), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(p->roles, lv_color_hex(0x26775E), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(p->roles, lv_color_hex(UI_TEXT), LV_PART_ITEMS);
    lv_obj_set_style_text_font(p->roles, UI_FONT_BODY, LV_PART_ITEMS);
    lv_obj_add_event_cb(p->roles, role_changed_cb, LV_EVENT_VALUE_CHANGED, p);
    ui_label(p->base.form, "双向模式优先尝试供电；供电端输出电能，受电端接收电能。\n修改 C1 角色可能触发端口重新协商。", 0, 248, 564, UI_MUTED);
    lv_obj_t *right = ui_card(parent, 644, 78, 356, 354);
    ui_label(right, "设备与本次设置", 0, 0, 320, UI_ACCENT);
    p->base.actual = ui_label(right, "--", 0, 34, 320, UI_TEXT);
    ui_label(right, "SW6306V / 寄存器手册 V1.0.2\n1024 × 600 / LVGL\n从顶部边缘下拉可调节亮度。", 0, 252, 320, UI_MUTED);
    p->base.reload = ui_button(parent, "读取 C1 配置", 24, 452, 170, reload_cb, p);
    p->base.discard = ui_button(parent, "放弃修改", 210, 452, 190, discard_cb, p);
    p->base.apply = ui_button(parent, "应用 C1 角色", 420, 452, 204, apply_cb, p);
    ui_system_settings_refresh(p, false);
}
