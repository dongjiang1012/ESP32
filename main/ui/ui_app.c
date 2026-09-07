#include "ui_app.h"
#include "data_snapshot.h"
#include "ui_brightness_panel.h"
#include "ui_config_pages.h"
#include "ui_page_manager.h"
#include "ui_power_chart.h"
#include "ui_fault_analysis.h"

static ui_power_chart_t s_power_chart;
static ui_output_page_t s_output_page;
static ui_buck_page_t s_buck_page;
static ui_system_page_t s_system_page;
static ui_fault_analysis_t s_fault_analysis;
static sw6306_config_model_t s_model;
static lv_timer_t *s_snapshot_timer;
static bool s_busy;
static bool s_load_attempted[UI_PAGE_COUNT];
static uint32_t s_last_adc_count;
static uint32_t s_last_adc_tick;
static bool s_have_adc;

static void refresh_controls(void)
{
    bool disabled = s_busy || s_snapshot_timer == NULL;
    ui_output_control_refresh(&s_output_page, disabled);
    ui_buckboost_config_refresh(&s_buck_page, disabled);
    ui_system_settings_refresh(&s_system_page, disabled);
}

static ui_config_page_t *operation_page(sw6306_config_operation_t operation)
{
    if (operation == SW6306_LOAD_OUTPUT || operation == SW6306_APPLY_OUTPUT) return &s_output_page.base;
    if (operation == SW6306_LOAD_BUCK || operation == SW6306_APPLY_BUCK) return &s_buck_page.base;
    return &s_system_page.base;
}

static void request_config(sw6306_config_operation_t operation)
{
    ui_config_page_t *page = operation_page(operation);
    if (s_busy || s_snapshot_timer == NULL) return;
    esp_err_t err = sw6306_config_worker_submit(operation, &s_model);
    if (err != ESP_OK) {
        lv_label_set_text_fmt(page->status, "请求失败：%s", ui_error_text(err));
        lv_obj_set_style_text_color(page->status, lv_color_hex(UI_ERROR), 0);
        return;
    }
    s_busy = true;
    bool apply = operation == SW6306_APPLY_OUTPUT || operation == SW6306_APPLY_BUCK || operation == SW6306_APPLY_ROLE;
    lv_label_set_text(page->status, apply ? "正在应用并校验回读…" : "正在读取芯片配置…");
    lv_obj_set_style_text_color(page->status, lv_color_hex(UI_ACCENT), 0);
    refresh_controls();
}

static void load_visible_page(void)
{
    ui_page_id_t page = ui_page_manager_current();
    if (s_busy || s_snapshot_timer == NULL || page == UI_PAGE_MONITOR || page == UI_PAGE_FAULT_ANALYSIS || s_load_attempted[page]) return;
    sw6306_config_operation_t operation = page == UI_PAGE_OUTPUT ? SW6306_LOAD_OUTPUT :
        page == UI_PAGE_BUCKBOOST ? SW6306_LOAD_BUCK : SW6306_LOAD_ROLE;
    request_config(operation);
    /* A failed read is not retried automatically; the user can press Reload. */
    s_load_attempted[page] = true;
}

static void page_shown(ui_page_id_t page)
{
    (void)page;
    refresh_controls();
    load_visible_page();
}

static void accept_result(const sw6306_config_result_t *r)
{
    /* Merge only the completed group. Other pages may contain unsaved drafts. */
    switch (r->operation) {
    case SW6306_LOAD_OUTPUT:
    case SW6306_APPLY_OUTPUT:
        s_model.output = r->model.output;
        s_model.output_actual = r->model.output_actual;
        s_model.output_valid = r->error == ESP_OK && r->model.output_valid;
        s_model.output_dirty = r->model.output_dirty;
        if (r->operation == SW6306_APPLY_OUTPUT) {
            s_model.c1_role_actual = r->model.c1_role_actual;
            s_model.c1_role_valid = r->model.c1_role_valid;
            if (!s_model.c1_role_dirty && s_model.c1_role_valid)
                s_model.c1_role = s_model.c1_role_actual;
            if (s_model.c1_role_valid)
                s_model.c1_role_dirty = s_model.c1_role != s_model.c1_role_actual;
        }
        break;
    case SW6306_LOAD_BUCK:
    case SW6306_APPLY_BUCK:
        s_model.buck = r->model.buck;
        s_model.buck_actual = r->model.buck_actual;
        s_model.buck_valid = r->error == ESP_OK && r->model.buck_valid;
        s_model.buck_dirty = r->model.buck_dirty;
        break;
    case SW6306_LOAD_ROLE:
    case SW6306_APPLY_ROLE:
        s_model.c1_role = r->model.c1_role;
        s_model.c1_role_actual = r->model.c1_role_actual;
        s_model.c1_role_valid = r->error == ESP_OK && r->model.c1_role_valid;
        s_model.c1_role_dirty = r->model.c1_role_dirty;
        break;
    default: return;
    }
    s_busy = false;
    ui_config_page_t *page = operation_page(r->operation);
    bool apply = r->operation == SW6306_APPLY_OUTPUT || r->operation == SW6306_APPLY_BUCK || r->operation == SW6306_APPLY_ROLE;
    if (r->error == ESP_OK) {
        lv_label_set_text(page->status, apply ? "应用成功，寄存器回读一致" : "已读取芯片配置，可以编辑");
        lv_obj_set_style_text_color(page->status, lv_color_hex(UI_ACCENT), 0);
    } else if (r->operation == SW6306_APPLY_OUTPUT && r->error == ESP_ERR_NOT_FOUND) {
        lv_label_set_text(page->status, "C1 未连接为 Source：检查 CC/Rd，重新读取后再应用。");
        lv_obj_set_style_text_color(page->status, lv_color_hex(UI_ERROR), 0);
    } else {
        lv_label_set_text_fmt(page->status, "%s失败：%s。%s",
            apply ? "应用" : "读取", ui_error_text(r->error),
            apply ? "状态未知，请重新读取后再编辑。" : "请点击“重新读取”重试。");
        lv_obj_set_style_text_color(page->status, lv_color_hex(UI_ERROR), 0);
    }
    refresh_controls();
}

static void ui_snapshot_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    sw6306_config_result_t result;
    if (sw6306_config_worker_poll(&result)) accept_result(&result);
    load_visible_page();

    int brightness = ui_brightness_panel_get_percent();
    if (lv_slider_get_value(s_system_page.brightness) != brightness) {
        lv_slider_set_value(s_system_page.brightness, brightness, LV_ANIM_OFF);
        lv_label_set_text_fmt(s_system_page.brightness_value, "%d%%", brightness);
    }

    data_snapshot_t snapshot;
    if (data_snapshot_read(&snapshot) != ESP_OK) return;
    ui_fault_analysis_refresh(&s_fault_analysis, &snapshot);
    const data_snapshot_group_state_t *fuel_gauge = &snapshot.state[DATA_GROUP_FUEL_GAUGE];
    if (fuel_gauge->last_error != ESP_OK)
        ui_power_chart_show_fuel_gauge_error(&s_power_chart, ui_error_text(fuel_gauge->last_error));
    else if (fuel_gauge->valid)
        ui_power_chart_update_fuel_gauge(&s_power_chart, &snapshot.fuel_gauge);
    const data_snapshot_group_state_t *adc = &snapshot.state[DATA_GROUP_ADC];
    if (adc->last_error != ESP_OK) {
        ui_power_chart_show_error(&s_power_chart, ui_error_text(adc->last_error));
        lv_label_set_text(s_output_page.live, "实时总线\n采样读取失败，数据不可用");
        return;
    }
    if (!adc->valid) return;
    if (!s_have_adc || adc->update_count != s_last_adc_count) {
        s_have_adc = true;
        s_last_adc_count = adc->update_count;
        s_last_adc_tick = lv_tick_get();
        ui_power_chart_update(&s_power_chart, snapshot.power_mw,
                               snapshot.adc.bus_voltage_mv.value, snapshot.adc.bus_current_ma.value);
        char voltage[24], current[24], temperature[24];
        ui_format_number(voltage, sizeof(voltage), snapshot.adc.bus_voltage_mv.value, 3);
        ui_format_number(current, sizeof(current), snapshot.adc.bus_current_ma.value, 3);
        ui_format_number(temperature, sizeof(temperature), snapshot.adc.chip_temperature_mc.value / 100, 1);
        lv_label_set_text_fmt(s_output_page.live, "实时总线  %s V / %s A\n芯片温度  %s ℃", voltage, current, temperature);
    } else if (lv_tick_elaps(s_last_adc_tick) > 2000) {
        ui_power_chart_show_error(&s_power_chart, "采样数据未更新");
        lv_label_set_text(s_output_page.live, "实时总线\n采样未更新，数据已过期");
    }
}

void ui_app_create(void)
{
    ui_fonts_init();
    sw6306_config_model_init(&s_model);
    ui_page_manager_create(lv_screen_active(), page_shown);
    ui_power_chart_create(&s_power_chart, ui_page_manager_root(UI_PAGE_MONITOR));
    ui_output_control_create(&s_output_page, ui_page_manager_root(UI_PAGE_OUTPUT), &s_model, request_config);
    ui_buckboost_config_create(&s_buck_page, ui_page_manager_root(UI_PAGE_BUCKBOOST), &s_model, request_config);
    ui_system_settings_create(&s_system_page, ui_page_manager_root(UI_PAGE_SYSTEM), &s_model, request_config);
    ui_fault_analysis_create(&s_fault_analysis, ui_page_manager_root(UI_PAGE_FAULT_ANALYSIS));
    ui_brightness_panel_create(lv_screen_active());

    s_snapshot_timer = lv_timer_create(ui_snapshot_timer_cb, 100, NULL);
    if (s_snapshot_timer == NULL) {
        ui_power_chart_show_error(&s_power_chart, "界面定时器不可用");
        lv_label_set_text(s_output_page.base.status, "界面定时器不可用，配置功能已禁用");
        lv_label_set_text(s_buck_page.base.status, "界面定时器不可用，配置功能已禁用");
        lv_label_set_text(s_system_page.base.status, "界面定时器不可用，配置功能已禁用");
    }
    ui_page_manager_show(UI_PAGE_MONITOR);
}
