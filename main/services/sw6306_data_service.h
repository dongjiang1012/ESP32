#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "sw6306_adc.h"
#include "sw6306_fast_charge_status.h"
#include "sw6306_fuel_gauge.h"
#include "sw6306_fault_status.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sw6306_data_service_read_fast_charge(
    sw6306_fast_charge_status_t *out);

esp_err_t sw6306_data_service_read_adc(sw6306_adc_data_t *out,
                                       int32_t *power_mw);

esp_err_t sw6306_data_service_read_fuel_gauge(
    sw6306_fuel_gauge_data_t *out);

esp_err_t sw6306_data_service_read_fault_status(
    sw6306_fault_status_t *out);


#ifdef __cplusplus
}
#endif
