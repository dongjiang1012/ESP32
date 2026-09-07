#include "ui_config_pages.h"
#include "ui_numeric_input.h"
#include <stdio.h>

static unsigned s_edit_index;

static void mark_dirty(ui_output_page_t *p)
{
    p->base.model->output_dirty = !sw6306_output_equal(&p->base.model->output, &p->base.model->output_actual);
    lv_label_set_text(p->base.status, p->base.model->output_dirty ? "草稿已修改，尚未应用" : "与上次回读配置一致");
    lv_obj_set_style_text_color(p->base.status, lv_color_hex(UI_ACCENT), 0);
    ui_output_control_refresh(p, false);
}

static void value_changed(int32_t value, void *context)
{
    ui_output_page_t *p = context;
    sw6306_output_config_t *c = &p->base.model->output;
    if (s_edit_index == 0) c->voltage_mv = value;
    else if (s_edit_index == 1) c->output_limit_ma = value;
    else c->charge_limit_ma = value;
    mark_dirty(p);
}

static void edit_cb(lv_event_t *event)
{
    ui_output_page_t *p = lv_event_get_user_data(event);
    if (!p->base.model->output_valid) return;
    const sw6306_numeric_spec_t *specs[] = {&SW6306_VOLTAGE_SPEC, &SW6306_OUTPUT_CURRENT_SPEC, &SW6306_CHARGE_CURRENT_SPEC};
    int32_t values[] = {p->base.model->output.voltage_mv, p->base.model->output.output_limit_ma, p->base.model->output.charge_limit_ma};
    for (unsigned i = 0; i < 3; ++i) if (lv_event_get_target_obj(event) == p->values[i]) {
        s_edit_index = i;
        ui_numeric_input_open(specs[i], values[i], value_changed, p);
        break;
    }
}

static void switch_cb(lv_event_t *event)
{
    ui_output_page_t *p = lv_event_get_user_data(event);
    p->base.model->output.force_voltage = lv_obj_has_state(p->force_voltage, LV_STATE_CHECKED);
    p->base.model->output.force_ibus = lv_obj_has_state(p->force_ibus, LV_STATE_CHECKED);
    mark_dirty(p);
}

static void confirmed_apply(void *context)
{
    ui_output_page_t *p = context;
    p->base.request(SW6306_APPLY_OUTPUT);
}

static void preset_22v_cb(lv_event_t *event)
{
    ui_output_page_t *p = lv_event_get_user_data(event);
    if (!p->base.model->output_valid) return;
    p->base.model->output.voltage_mv = 22000;
    p->base.model->output.force_voltage = true;
    mark_dirty(p);
}

static void apply_cb(lv_event_t *event)
{
    ui_output_page_t *p = lv_event_get_user_data(event);
    char voltage[24], discharge[24], charge[24], message[1024];
    sw6306_output_config_t *c = &p->base.model->output;
    ui_format_number(voltage, sizeof(voltage), c->voltage_mv, 3);
    ui_format_number(discharge, sizeof(discharge), c->output_limit_ma, 3);
    ui_format_number(charge, sizeof(charge), c->charge_limit_ma, 3);
    snprintf(message, sizeof(message),
        "板内 VBUS 强制输出：%s   /   %s V\nIBUS 强制限流：%s\n输出限流：%s A   /   充电限流：%s A\n\n"
        "IBUS 强制限流同时影响充电和放电。\n"
        "开启时自动设 C1 为 Source，CC 需检测到受电端。\n"
        "C1 口也会输出设定高压，请使用专用测试连接。\n"
        "请断开充电输入；失败后输出可能保持关闭。\n"
        "关闭开关并应用后，物理输出保持关闭。",
        c->force_voltage ? "开启" : "关闭", voltage, c->force_ibus ? "开启" : "关闭", discharge, charge);
    ui_confirm_open("确认应用输出配置？", message, confirmed_apply, p);
}

static void reload_confirmed(void *context)
{
    ui_output_page_t *p = context;
    p->base.request(SW6306_LOAD_OUTPUT);
}

static void reload_cb(lv_event_t *event)
{
    ui_output_page_t *p = lv_event_get_user_data(event);
    if (p->base.model->output_dirty) ui_confirm_open("覆盖输出配置草稿？", "重新读取将用芯片配置覆盖尚未应用的草稿。", reload_confirmed, p);
    else reload_confirmed(p);
}

static void discard_cb(lv_event_t *event)
{
    ui_output_page_t *p = lv_event_get_user_data(event);
    p->base.model->output = p->base.model->output_actual;
    mark_dirty(p);
}

void ui_output_control_refresh(ui_output_page_t *p, bool busy)
{
    sw6306_config_model_t *m = p->base.model;
    bool editable = m->output_valid && !busy;
    ui_set_checked(p->force_voltage, m->output.force_voltage);
    ui_set_checked(p->force_ibus, m->output.force_ibus);
    ui_set_enabled(p->force_voltage, editable);
    ui_set_enabled(p->force_ibus, editable);
    ui_set_enabled(p->preset_22v, editable);
    int32_t values[] = {m->output.voltage_mv, m->output.output_limit_ma, m->output.charge_limit_ma};
    for (int i = 0; i < 3; ++i) {
        char number[24], text[48];
        ui_format_number(number, sizeof(number), values[i], 3);
        snprintf(text, sizeof(text), "%s %s  >", m->output_valid ? number : "--", i == 0 ? "V" : "A");
        lv_label_set_text(lv_obj_get_child(p->values[i], 0), text);
        ui_set_enabled(p->values[i], editable && (i == 0 || m->output.force_ibus));
    }
    ui_set_enabled(p->base.reload, !busy);
    ui_set_enabled(p->base.discard, editable && m->output_dirty);
    /* Apply is also an explicit on/off command, including after a failed apply
     * or when normal chip output differs from the force-enable register. */
    ui_set_enabled(p->base.apply, editable && sw6306_output_valid(&m->output));
    if (!m->output_valid) lv_label_set_text(p->base.actual, "尚无有效的回读配置。\n请点击“重新读取”读取芯片。");
    else {
        char v[24], d[24], c[24];
        ui_format_number(v, sizeof(v), m->output_actual.voltage_mv, 3);
        ui_format_number(d, sizeof(d), m->output_actual.output_limit_ma, 3);
        ui_format_number(c, sizeof(c), m->output_actual.charge_limit_ma, 3);
        lv_label_set_text_fmt(p->base.actual,
            "电压强制控制  %s\nIBUS 强制限流  %s\n\n电压设定  %s V\n输出限流  %s A\n充电限流  %s A",
            m->output_actual.force_voltage ? "开启" : "关闭", m->output_actual.force_ibus ? "开启" : "关闭", v, d, c);
    }
}

void ui_output_control_create(ui_output_page_t *p, lv_obj_t *parent,
                              sw6306_config_model_t *model, ui_config_request_cb_t request)
{
    p->base = (ui_config_page_t){.root = parent, .model = model, .request = request};
    lv_obj_t *title = ui_label(parent, "输出控制", 24, 12, 580, UI_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, 0);
    p->base.status = ui_label(parent, "请先读取芯片配置", 24, 44, 900, UI_ACCENT);
    p->base.form = ui_card(parent, 24, 78, 600, 354);
    lv_obj_t *form = p->base.form;
    ui_label(form, "强制 VBUS 输出使能", 0, 10, 370, UI_TEXT);
    p->force_voltage = lv_switch_create(form);
    lv_obj_set_pos(p->force_voltage, 486, 4);
    lv_obj_set_size(p->force_voltage, 72, 36);
    lv_obj_add_event_cb(p->force_voltage, switch_cb, LV_EVENT_VALUE_CHANGED, p);
    ui_label(form, "强制控制 IBUS 限流（充电及放电）", 0, 62, 470, UI_TEXT);
    p->force_ibus = lv_switch_create(form);
    lv_obj_set_pos(p->force_ibus, 486, 56);
    lv_obj_set_size(p->force_ibus, 72, 36);
    lv_obj_add_event_cb(p->force_ibus, switch_cb, LV_EVENT_VALUE_CHANGED, p);
    const char *names[] = {"输出电压", "输出 IBUS 限流", "充电 IBUS 限流"};
    const char *hints[] = {"3.300～27.300 V / 步进 0.010 V", "0.200～7.000 A / 步进 0.050 A", "共用使能开关，需分别设置限流值"};
    for (int i = 0; i < 3; ++i) {
        int y = 116 + i * 66;
        ui_label(form, names[i], 0, y, 350, UI_TEXT);
        ui_label(form, hints[i], 0, y + 25, 365, UI_MUTED);
        p->values[i] = ui_button(form, "--", 374, y + 1, 184, edit_cb, p);
    }
    lv_obj_t *right = ui_card(parent, 644, 78, 356, 354);
    ui_label(right, "上次回读配置", 0, 0, 320, UI_ACCENT);
    p->base.actual = ui_label(right, "--", 0, 34, 320, UI_TEXT);
    p->live = ui_label(right, "实时总线\n等待采样数据…", 0, 205, 320, UI_MUTED);
    ui_label(right, "板内 VBUS 由 C1 连接触发。\nCC 需有受电端，C1 口也带高压。\n回读设定值不代表实测电压。", 0, 256, 320, UI_WARNING);
    p->base.reload = ui_button(parent, "重新读取", 24, 452, 170, reload_cb, p);
    p->base.discard = ui_button(parent, "放弃修改", 210, 452, 190, discard_cb, p);
    p->base.apply = ui_button(parent, "应用输出配置", 420, 452, 204, apply_cb, p);
    p->preset_22v = ui_button(parent, "载入 22V", 644, 452, 164, preset_22v_cb, p);
    ui_label(parent, "仅载入强制电压草稿，\n点击应用后写入。", 824, 454, 176, UI_MUTED);
    ui_output_control_refresh(p, false);
}
