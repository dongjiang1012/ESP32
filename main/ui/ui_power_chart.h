#pragma once

#include <stdint.h>

#include "lvgl.h"
#include "sw6306_fuel_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *power_root;
    lv_obj_t *fuel_root;
    lv_obj_t *power_tab;
    lv_obj_t *fuel_tab;
    lv_obj_t *chart;
    lv_chart_series_t *series;
    lv_obj_t *power_label;
    lv_obj_t *voltage_label;
    lv_obj_t *current_label;
    lv_obj_t *status_label;
    lv_obj_t *fuel_status;
    lv_obj_t *fuel_soc;
    lv_obj_t *fuel_capacity;
    lv_obj_t *fuel_levels;
    lv_obj_t *fuel_raw;
} ui_power_chart_t;

/** Create the 0-100 W bus-power dashboard inside a persistent page container. */
void ui_power_chart_create(ui_power_chart_t *view, lv_obj_t *parent);

/** Add one sample. power_mw is displayed precisely and charted in whole watts. */
void ui_power_chart_update(ui_power_chart_t *view,
                           int32_t power_mw,
                           int32_t voltage_mv,
                           int32_t current_ma);

/** Show an ADC acquisition error without destroying the existing trace. */
void ui_power_chart_show_error(ui_power_chart_t *view, const char *message);

/** Refresh the secondary "battery meter" view from the SW6306 fuel-gauge registers. */
void ui_power_chart_update_fuel_gauge(ui_power_chart_t *view,
                                      const sw6306_fuel_gauge_data_t *data);
void ui_power_chart_show_fuel_gauge_error(ui_power_chart_t *view, const char *message);

#ifdef __cplusplus
}
#endif
