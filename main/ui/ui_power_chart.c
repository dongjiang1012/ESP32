#include "ui_power_chart.h"
#include "ui_common.h"

#define POWER_CHART_POINT_COUNT 60
#define POWER_CHART_MAX_W 100

static void show_secondary_page(ui_power_chart_t *view, bool fuel)
{
    if (fuel) {
        lv_obj_add_flag(view->power_root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(view->fuel_root, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(view->power_root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->fuel_root, LV_OBJ_FLAG_HIDDEN);
    }
    ui_set_checked(view->power_tab, !fuel);
    ui_set_checked(view->fuel_tab, fuel);
}

static void secondary_tab_cb(lv_event_t *event)
{
    ui_power_chart_t *view = lv_event_get_user_data(event);
    show_secondary_page(view, lv_event_get_target_obj(event) == view->fuel_tab);
}

void ui_power_chart_create(ui_power_chart_t *view, lv_obj_t *parent)
{
    if (view == NULL || parent == NULL) return;
    lv_obj_t *title = ui_label(parent, "实时监控", 24, 12, 400, UI_TEXT);
    lv_obj_set_style_text_font(title, UI_FONT_LARGE, 0);
    ui_label(parent, "实时功率与 SW6306 库仑计数据", 24, 44, 500, UI_MUTED);
    view->power_tab = ui_button(parent, "功率监控", 650, 16, 150, secondary_tab_cb, view);
    view->fuel_tab = ui_button(parent, "电量计", 820, 16, 150, secondary_tab_cb, view);

    view->power_root = lv_obj_create(parent);
    lv_obj_remove_style_all(view->power_root);
    lv_obj_set_pos(view->power_root, 0, 78);
    lv_obj_set_size(view->power_root, lv_pct(100), 418);
    lv_obj_remove_flag(view->power_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *metrics = ui_card(view->power_root, 24, 0, 230, 418);
    ui_label(metrics, "总线功率", 0, 0, 196, UI_MUTED);
    view->power_label = ui_label(metrics, "-- W", 0, 34, 196, UI_ACCENT);
    lv_obj_set_style_text_font(view->power_label, UI_FONT_LARGE, 0);
    ui_label(metrics, "电压", 0, 104, 196, UI_MUTED);
    view->voltage_label = ui_label(metrics, "-- V", 0, 132, 196, UI_TEXT);
    lv_obj_set_style_text_font(view->voltage_label, UI_FONT_MEDIUM, 0);
    ui_label(metrics, "电流", 0, 208, 196, UI_MUTED);
    view->current_label = ui_label(metrics, "-- A", 0, 236, 196, UI_TEXT);
    lv_obj_set_style_text_font(view->current_label, UI_FONT_MEDIUM, 0);
    view->status_label = ui_label(metrics, "等待采样数据…", 0, 312, 196, UI_MUTED);

    lv_obj_t *panel = ui_card(view->power_root, 274, 0, 726, 418);
    ui_label(panel, "100 W", 0, 0, 120, UI_MUTED);
    ui_label(panel, "0 W", 0, 359, 100, UI_MUTED);
    view->chart = lv_chart_create(panel);
    lv_obj_set_pos(view->chart, 0, 28);
    lv_obj_set_size(view->chart, 692, 324);
    lv_chart_set_type(view->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_axis_range(view->chart, LV_CHART_AXIS_PRIMARY_Y, 0, POWER_CHART_MAX_W);
    lv_chart_set_point_count(view->chart, POWER_CHART_POINT_COUNT);
    lv_chart_set_update_mode(view->chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(view->chart, 6, 10);
    lv_obj_set_style_radius(view->chart, 0, 0);
    lv_obj_set_style_border_color(view->chart, lv_color_hex(UI_ACCENT), 0);
    lv_obj_set_style_border_width(view->chart, 1, 0);
    lv_obj_set_style_bg_color(view->chart, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_line_color(view->chart, lv_color_hex(0x354C59), LV_PART_MAIN);
    lv_obj_set_style_line_width(view->chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(view->chart, 0, 0, LV_PART_INDICATOR);
    view->series = lv_chart_add_series(view->chart, lv_color_hex(UI_ACCENT), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_values(view->chart, view->series, LV_CHART_POINT_NONE);

    view->fuel_root = lv_obj_create(parent);
    lv_obj_remove_style_all(view->fuel_root);
    lv_obj_set_pos(view->fuel_root, 0, 78);
    lv_obj_set_size(view->fuel_root, lv_pct(100), 418);
    lv_obj_remove_flag(view->fuel_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *summary = ui_card(view->fuel_root, 24, 0, 976, 108);
    ui_label(summary, "显示电量（REG0x99）", 0, 0, 220, UI_MUTED);
    view->fuel_soc = ui_label(summary, "-- %", 0, 28, 220, UI_ACCENT);
    lv_obj_set_style_text_font(view->fuel_soc, UI_FONT_LARGE, 0);
    view->fuel_status = ui_label(summary, "等待库仑计采样…", 250, 4, 680, UI_MUTED);
    view->fuel_capacity = ui_label(summary, "", 250, 36, 680, UI_TEXT);

    lv_obj_t *levels = ui_card(view->fuel_root, 24, 124, 456, 294);
    view->fuel_levels = ui_label(levels, "电量百分比", 0, 0, 416, UI_MUTED);
    lv_obj_t *raw = ui_card(view->fuel_root, 500, 124, 500, 294);
    view->fuel_raw = ui_label(raw, "容量寄存器", 0, 0, 460, UI_MUTED);
    show_secondary_page(view, false);
}

void ui_power_chart_update(ui_power_chart_t *view, int32_t power_mw,
                           int32_t voltage_mv, int32_t current_ma)
{
    if (view == NULL || view->chart == NULL || view->series == NULL) return;
    int32_t watts = power_mw <= 0 ? 0 : (power_mw + 500) / 1000;
    if (watts > POWER_CHART_MAX_W) watts = POWER_CHART_MAX_W;
    lv_chart_set_next_value(view->chart, view->series, watts);
    char power[24], voltage[24], current[24];
    ui_format_number(power, sizeof(power), power_mw / 10, 2);
    ui_format_number(voltage, sizeof(voltage), voltage_mv, 3);
    ui_format_number(current, sizeof(current), current_ma, 3);
    lv_label_set_text_fmt(view->power_label, "%s W", power);
    lv_label_set_text_fmt(view->voltage_label, "%s V", voltage);
    lv_label_set_text_fmt(view->current_label, "%s A", current);
    lv_label_set_text(view->status_label, "采样正常 | 实时更新");
    lv_obj_set_style_text_color(view->status_label, lv_color_hex(UI_ACCENT), 0);
}

void ui_power_chart_show_error(ui_power_chart_t *view, const char *message)
{
    if (view == NULL || view->status_label == NULL) return;
    lv_label_set_text_fmt(view->status_label, "采样数据不可用\n%s",
                          message != NULL ? message : "未知错误");
    lv_obj_set_style_text_color(view->status_label, lv_color_hex(UI_ERROR), 0);
}

void ui_power_chart_update_fuel_gauge(ui_power_chart_t *view,
                                      const sw6306_fuel_gauge_data_t *data)
{
    if (view == NULL || data == NULL) return;
    char full[24], current[24];
    ui_format_number(full, sizeof(full), (int32_t)data->maximum_capacity_uwh.value, 6);
    ui_format_number(current, sizeof(current), (int32_t)data->current_capacity_uwh.value, 6);
    lv_label_set_text_fmt(view->fuel_soc, "%lu %%", (unsigned long)data->display_level_percent.value);
    lv_label_set_text(view->fuel_status, "SW6306 库仑计原始读数 | 每 2 秒更新");
    lv_obj_set_style_text_color(view->fuel_status, lv_color_hex(UI_ACCENT), 0);
    lv_label_set_text_fmt(view->fuel_capacity,
        "当前容量  %s Wh      最大容量  %s Wh", current, full);
    lv_label_set_text_fmt(view->fuel_levels,
        "电量百分比\n\n当前电量（REG0x8B）  %lu %%\n"
        "可用电量（REG0x8C）  %lu %%\n"
        "最终处理（REG0x94）  %lu %%\n"
        "显示电量（REG0x99）  %lu %%\n\n"
        "这些值由芯片库仑计直接给出，不做软件估算。",
        (unsigned long)data->current_level_percent.value,
        (unsigned long)data->available_level_percent.value,
        (unsigned long)data->equalized_level_percent.value,
        (unsigned long)data->display_level_percent.value);
    lv_label_set_text_fmt(view->fuel_raw,
        "容量寄存器\n\n最大容量  REG0x86/0x87\n"
        "原始值  0x%03lX\n换算值  %s Wh\n\n"
        "当前容量  REG0x88–0x8A\n"
        "原始值  0x%05lX\n换算值  %s Wh",
        (unsigned long)data->maximum_capacity_uwh.raw, full,
        (unsigned long)data->current_capacity_uwh.raw, current);
}

void ui_power_chart_show_fuel_gauge_error(ui_power_chart_t *view, const char *message)
{
    if (view == NULL || view->fuel_status == NULL) return;
    lv_label_set_text_fmt(view->fuel_status, "库仑计数据不可用：%s",
                          message != NULL ? message : "未知错误");
    lv_obj_set_style_text_color(view->fuel_status, lv_color_hex(UI_ERROR), 0);
}
