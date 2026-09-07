#include "ui_fault_analysis.h"

#include <stdio.h>
#include <string.h>

#include "ui_common.h"

static bool has_fault(const sw6306_fault_status_t *status)
{
    return (status->event_flags & 0x1c) != 0 || status->discharge_faults != 0 ||
           status->charge_faults != 0 || (status->other_faults & 0x1f) != 0;
}

static void append_text(char *text, size_t size, const char *description)
{
    size_t used = strlen(text);
    if (used >= size - 1) return;
    (void)snprintf(text + used, size - used, "- %s\n", description);
}

static void append_fault(char *text, size_t size, uint8_t raw, uint8_t mask,
                         const char *description)
{
    if ((raw & mask) != 0) append_text(text, size, description);
}

static void set_list(lv_obj_t *label, const char *title, char *text, size_t size)
{
    if (text[0] == '\0') (void)snprintf(text, size, "正常\n未发现对应异常记录");
    lv_label_set_text_fmt(label, "%s\n%s", title, text);
}

void ui_fault_analysis_create(ui_fault_analysis_t *view, lv_obj_t *parent)
{
    if (view == NULL || parent == NULL) return;
    *view = (ui_fault_analysis_t){0};

    lv_obj_t *title = ui_label(parent, "异常分析", 24, 12, 300, UI_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, 0);
    ui_label(parent, "只读诊断：事件位不会被本页面自动清除", 24, 44, 700, UI_MUTED);

    lv_obj_t *overview = ui_card(parent, 24, 78, 976, 78);
    view->summary = ui_label(overview, "等待异常寄存器采样…", 0, 0, 500, UI_MUTED);
    view->raw = ui_label(overview, "REG 15/2A/2B/2C: -- / -- / -- / --", 0, 30, 700, UI_MUTED);

    lv_obj_t *event_card = ui_card(parent, 24, 170, 304, 326);
    view->events = ui_label(event_card, "事件", 0, 0, 272, UI_MUTED);
    lv_obj_t *discharge_card = ui_card(parent, 348, 170, 304, 326);
    view->discharge = ui_label(discharge_card, "放电异常", 0, 0, 272, UI_MUTED);
    lv_obj_t *charge_card = ui_card(parent, 672, 170, 328, 326);
    view->charge = ui_label(charge_card, "充电异常", 0, 0, 296, UI_MUTED);

    /* Other fault information is compact enough to share the event card. */
    view->other = ui_label(event_card, "", 0, 178, 272, UI_MUTED);
}

void ui_fault_analysis_refresh(ui_fault_analysis_t *view,
                               const data_snapshot_t *snapshot)
{
    if (view == NULL || snapshot == NULL) return;
    const data_snapshot_group_state_t *state = &snapshot->state[DATA_GROUP_FAULT_STATUS];
    if (state->last_error != ESP_OK) {
        lv_label_set_text_fmt(view->summary, "异常寄存器读取失败：%s", ui_error_text(state->last_error));
        lv_obj_set_style_text_color(view->summary, lv_color_hex(UI_ERROR), 0);
        return;
    }
    if (!state->valid) return;
    if (view->has_sample && view->last_update_count == state->update_count) return;
    view->has_sample = true;
    view->last_update_count = state->update_count;

    const sw6306_fault_status_t *s = &snapshot->fault_status;
    const bool fault = has_fault(s);
    lv_label_set_text(view->summary, fault ? "发现异常事件或历史保护记录，请检查下方详情" : "未发现充电或放电异常记录");
    lv_obj_set_style_text_color(view->summary, lv_color_hex(fault ? UI_ERROR : UI_ACCENT), 0);
    lv_label_set_text_fmt(view->raw, "原始寄存器  0x15=%02X   0x2A=%02X   0x2B=%02X   0x2C=%02X",
                          s->event_flags, s->discharge_faults, s->charge_faults, s->other_faults);

    char events[420] = {0};
    append_fault(events, sizeof(events), s->event_flags, 0x10, "电池欠压 UVLO 事件");
    append_fault(events, sizeof(events), s->event_flags, 0x08, "充电异常事件");
    append_fault(events, sizeof(events), s->event_flags, 0x04, "放电异常事件");
    append_fault(events, sizeof(events), s->event_flags, 0x02, "按键事件");
    append_fault(events, sizeof(events), s->event_flags, 0x01, "场景变化事件");
    set_list(view->events, "0x15 事件指示", events, sizeof(events));

    char discharge[420] = {0};
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x40, "电池过压");
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x20, "芯片过温");
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x10, "NTC 过温");
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x08, "VBUS 过载");
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x04, "VBUS 短路");
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x02, "VBUS 慢速过压");
    append_fault(discharge, sizeof(discharge), s->discharge_faults, 0x01, "VBUS 快速过压");
    set_list(view->discharge, "0x2A 放电异常历史", discharge, sizeof(discharge));

    char charge[440] = {0};
    append_fault(charge, sizeof(charge), s->charge_faults, 0x80, "电池电压低于 1.5V");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x40, "充电超时");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x20, "充满事件");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x10, "电池过压");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x08, "芯片过温");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x04, "NTC 过温");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x02, "VBUS 慢速过压");
    append_fault(charge, sizeof(charge), s->charge_faults, 0x01, "VBUS 快速过压");
    set_list(view->charge, "0x2B 充电异常历史", charge, sizeof(charge));

    char other[320] = {0};
    if ((s->other_faults & 0xc0) != 0) {
        const char *key = (s->other_faults & 0xc0) == 0x40 ? "短按" :
                          (s->other_faults & 0xc0) == 0x80 ? "双击" : "长按";
        char key_event[32];
        (void)snprintf(key_event, sizeof(key_event), "按键事件：%s", key);
        append_text(other, sizeof(other), key_event);
    }
    append_fault(other, sizeof(other), s->other_faults, 0x10, "放电 62368 低温");
    append_fault(other, sizeof(other), s->other_faults, 0x08, "充电 62368 低温");
    append_fault(other, sizeof(other), s->other_faults, 0x04, "充电 62368 过温");
    append_fault(other, sizeof(other), s->other_faults, 0x02, "放电 DPDM 5.5V 过压");
    append_fault(other, sizeof(other), s->other_faults, 0x01, "放电 CC 5.5V 过压");
    set_list(view->other, "0x2C 其他状态", other, sizeof(other));
}
