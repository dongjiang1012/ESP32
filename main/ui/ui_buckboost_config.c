#include "ui_config_pages.h"

static const char *const s_frequency[] = {"300 kHz", "200 kHz", "400 kHz", "500 kHz", ""};
static const char *const s_peak[] = {"12 A", "14 A", "16 A", "18 A", ""};
static const char *const s_temperature[] = {"120 ℃", "130 ℃", "140 ℃", "150 ℃", ""};
static const char *const s_rdson[] = {"2.5 mΩ", "5 mΩ", "7.5 mΩ", "10 mΩ", ""};

static void mark_dirty(ui_buck_page_t *p)
{
    p->base.model->buck_dirty = !sw6306_buck_equal(&p->base.model->buck, &p->base.model->buck_actual);
    lv_label_set_text(p->base.status, p->base.model->buck_dirty ? "草稿已修改，尚未应用" : "与上次回读配置一致");
    lv_obj_set_style_text_color(p->base.status, lv_color_hex(UI_ACCENT), 0);
    ui_buckboost_config_refresh(p, false);
}

static void selection_cb(lv_event_t *event)
{
    ui_buck_page_t *p = lv_event_get_user_data(event);
    lv_obj_t *target = lv_event_get_target_obj(event);
    sw6306_buck_config_t *c = &p->base.model->buck;
    if (target == p->pwm) c->force_pwm = lv_obj_has_state(target, LV_STATE_CHECKED);
    else {
        uint32_t index = lv_buttonmatrix_get_selected_button(target);
        if (index > 3) return;
        if (target == p->options[0]) c->frequency_khz = SW6306_FREQUENCIES_KHZ[index];
        if (target == p->options[1]) c->peak_limit_ma = SW6306_PEAK_LIMITS_MA[index];
        if (target == p->options[2]) c->die_temperature_c = SW6306_DIE_TEMPERATURES_C[index];
        if (target == p->options[3]) c->m2_rdson_uohm = SW6306_M2_RDSON_UOHM[index];
    }
    mark_dirty(p);
}

static void reload_confirmed(void *context)
{
    ui_buck_page_t *p = context;
    p->base.request(SW6306_LOAD_BUCK);
}

static void reload_cb(lv_event_t *event)
{
    ui_buck_page_t *p = lv_event_get_user_data(event);
    if (p->base.model->buck_dirty) ui_confirm_open("覆盖升降压配置草稿？", "重新读取将用芯片配置覆盖尚未应用的草稿。", reload_confirmed, p);
    else reload_confirmed(p);
}

static void apply_cb(lv_event_t *event)
{
    ui_buck_page_t *p = lv_event_get_user_data(event);
    p->base.request(SW6306_APPLY_BUCK);
}

static void discard_cb(lv_event_t *event)
{
    ui_buck_page_t *p = lv_event_get_user_data(event);
    p->base.model->buck = p->base.model->buck_actual;
    mark_dirty(p);
}

static void defaults_cb(lv_event_t *event)
{
    ui_buck_page_t *p = lv_event_get_user_data(event);
    /* V1.0.2 reset values, draft only. The installed M2 must still match. */
    p->base.model->buck = (sw6306_buck_config_t) {
        .frequency_khz = 300, .peak_limit_ma = 18000,
        .die_temperature_c = 120, .m2_rdson_uohm = 10000, .force_pwm = false,
    };
    mark_dirty(p);
}

void ui_buckboost_config_refresh(ui_buck_page_t *p, bool busy)
{
    sw6306_config_model_t *m = p->base.model;
    bool editable = m->buck_valid && !busy;
    const int indices[] = {
        sw6306_option_index(SW6306_FREQUENCIES_KHZ, m->buck.frequency_khz),
        sw6306_option_index(SW6306_PEAK_LIMITS_MA, m->buck.peak_limit_ma),
        sw6306_option_index(SW6306_DIE_TEMPERATURES_C, m->buck.die_temperature_c),
        sw6306_option_index(SW6306_M2_RDSON_UOHM, m->buck.m2_rdson_uohm),
    };
    for (int i = 0; i < 4; ++i) {
        lv_buttonmatrix_clear_button_ctrl_all(p->options[i], LV_BUTTONMATRIX_CTRL_CHECKED);
        if (m->buck_valid && indices[i] >= 0) lv_buttonmatrix_set_button_ctrl(p->options[i], indices[i], LV_BUTTONMATRIX_CTRL_CHECKED);
        ui_set_enabled(p->options[i], editable);
    }
    ui_set_checked(p->pwm, m->buck.force_pwm);
    ui_set_enabled(p->pwm, editable);
    ui_set_enabled(p->base.reload, !busy);
    ui_set_enabled(p->base.apply, editable && m->buck_dirty && sw6306_buck_valid(&m->buck));
    ui_set_enabled(p->base.discard, editable && m->buck_dirty);
    ui_set_enabled(p->defaults, editable);
    if (!m->buck_valid) lv_label_set_text(p->base.actual, "尚无有效的回读配置。\n请点击“重新读取”读取芯片。");
    else {
        char rdson[24];
        ui_format_number(rdson, sizeof(rdson), m->buck_actual.m2_rdson_uohm, 3);
        lv_label_set_text_fmt(p->base.actual,
            "开关频率  %ld kHz\n峰值限流  %ld A\n芯片过温门限  %ld ℃\nM2 导通电阻  %s mΩ\n轻载工作模式  %s",
            (long)m->buck_actual.frequency_khz, (long)(m->buck_actual.peak_limit_ma / 1000),
            (long)m->buck_actual.die_temperature_c, rdson, m->buck_actual.force_pwm ? "PWM" : "PFM");
    }
}

void ui_buckboost_config_create(ui_buck_page_t *p, lv_obj_t *parent,
                               sw6306_config_model_t *model, ui_config_request_cb_t request)
{
    p->base = (ui_config_page_t){.root = parent, .model = model, .request = request};
    lv_obj_t *title = ui_label(parent, "升降压配置", 24, 12, 580, UI_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, 0);
    p->base.status = ui_label(parent, "请先读取芯片配置", 24, 44, 950, UI_ACCENT);
    p->base.form = ui_card(parent, 24, 78, 600, 354);
    const char *titles[] = {"开关工作频率", "充放电峰值限流（共用档位）", "芯片内部过温门限", "M2 导通电阻（与峰值电流检测联动）"};
    const char *const *maps[] = {s_frequency, s_peak, s_temperature, s_rdson};
    for (int i = 0; i < 4; ++i) {
        ui_label(p->base.form, titles[i], 0, i * 68, 560, UI_TEXT);
        p->options[i] = lv_buttonmatrix_create(p->base.form);
        lv_obj_set_pos(p->options[i], 0, i * 68 + 22);
        lv_obj_set_size(p->options[i], 566, 42);
        lv_buttonmatrix_set_map(p->options[i], maps[i]);
        lv_buttonmatrix_set_button_ctrl_all(p->options[i], LV_BUTTONMATRIX_CTRL_CHECKABLE);
        lv_buttonmatrix_set_one_checked(p->options[i], true);
        lv_obj_set_style_pad_all(p->options[i], 0, 0);
        lv_obj_set_style_bg_color(p->options[i], lv_color_hex(UI_CARD), 0);
        lv_obj_set_style_border_width(p->options[i], 0, 0);
        lv_obj_set_style_bg_color(p->options[i], lv_color_hex(0x2D4353), LV_PART_ITEMS);
        lv_obj_set_style_bg_color(p->options[i], lv_color_hex(0x26775E), LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(p->options[i], lv_color_hex(UI_TEXT), LV_PART_ITEMS);
        lv_obj_set_style_text_font(p->options[i], UI_FONT_BODY, LV_PART_ITEMS);
        lv_obj_add_event_cb(p->options[i], selection_cb, LV_EVENT_VALUE_CHANGED, p);
    }
    ui_label(p->base.form, "轻载时强制采用 PWM 模式", 0, 294, 410, UI_TEXT);
    p->pwm = lv_switch_create(p->base.form);
    lv_obj_set_pos(p->pwm, 486, 282);
    lv_obj_set_size(p->pwm, 72, 36);
    lv_obj_add_event_cb(p->pwm, selection_cb, LV_EVENT_VALUE_CHANGED, p);
    lv_obj_t *right = ui_card(parent, 644, 78, 356, 354);
    ui_label(right, "上次回读配置", 0, 0, 320, UI_ACCENT);
    p->base.actual = ui_label(right, "--", 0, 34, 320, UI_TEXT);
    ui_label(right, "充电和放电共用一个峰值档位。\n导通电阻应与实际 M2 参数匹配。\n\n这里显示芯片寄存器设定值，\n不是实测开关频率或峰值电流。", 0, 172, 320, UI_WARNING);
    p->base.reload = ui_button(parent, "重新读取", 24, 452, 170, reload_cb, p);
    p->base.discard = ui_button(parent, "放弃修改", 210, 452, 190, discard_cb, p);
    p->base.apply = ui_button(parent, "应用升降压配置", 420, 452, 204, apply_cb, p);
    p->defaults = ui_button(parent, "载入芯片复位值到草稿", 650, 452, 350, defaults_cb, p);
    ui_buckboost_config_refresh(p, false);
}
