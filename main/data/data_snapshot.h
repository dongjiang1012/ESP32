#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "sw6306_adc.h"
#include "sw6306_fast_charge_status.h"
#include "sw6306_fuel_gauge.h"
#include "sw6306_fault_status.h"

typedef enum { DATA_GROUP_FAST_CHARGE = 0, DATA_GROUP_ADC, DATA_GROUP_FUEL_GAUGE, DATA_GROUP_FAULT_STATUS, DATA_GROUP_COUNT } data_group_t;
typedef struct { bool valid; esp_err_t last_error; uint32_t update_count; } data_snapshot_group_state_t;
typedef struct { sw6306_fast_charge_status_t fast_charge; sw6306_adc_data_t adc; int32_t power_mw; sw6306_fuel_gauge_data_t fuel_gauge; sw6306_fault_status_t fault_status; data_snapshot_group_state_t state[DATA_GROUP_COUNT]; } data_snapshot_t;

esp_err_t data_snapshot_init(void);
void data_snapshot_publish_fast_charge(const sw6306_fast_charge_status_t *data);
void data_snapshot_publish_adc(const sw6306_adc_data_t *data, int32_t power_mw);
void data_snapshot_publish_fuel_gauge(const sw6306_fuel_gauge_data_t *data);
void data_snapshot_publish_fault_status(const sw6306_fault_status_t *data);
void data_snapshot_report_error(data_group_t group, esp_err_t error);
esp_err_t data_snapshot_read(data_snapshot_t *out);
