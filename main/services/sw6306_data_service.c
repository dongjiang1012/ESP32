#include "sw6306_data_service.h"

#include "sw6306_adc.h"
#include "sw6306_fast_charge_status.h"
#include "sw6306_fuel_gauge.h"
#include "sw6306_fault_status.h"

esp_err_t sw6306_data_service_read_fast_charge(
    sw6306_fast_charge_status_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t ret = sw6306_fast_charge_status_read();
    if (ret == ESP_OK) {
        *out = g_sw6306_fast_charge_status;
    }
    return ret;
}

esp_err_t sw6306_data_service_read_adc(sw6306_adc_data_t *out,
                                       int32_t *power_mw)
{
    if (out == NULL || power_mw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t ret = sw6306_adc_read_all();
    if (ret != ESP_OK) {
        return ret;
    }

    *out = g_sw6306_adc_data;
    const int32_t current_ma = out->bus_current_ma.value < 0
                                    ? -out->bus_current_ma.value
                                    : out->bus_current_ma.value;
    *power_mw = (int32_t)(((int64_t)out->bus_voltage_mv.value * current_ma) /
                          1000);
    return ESP_OK;
}

esp_err_t sw6306_data_service_read_fuel_gauge(
    sw6306_fuel_gauge_data_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t ret = sw6306_fuel_gauge_read_all();
    if (ret == ESP_OK) {
        *out = g_sw6306_fuel_gauge_data;
    }
    return ret;
}

esp_err_t sw6306_data_service_read_fault_status(
    sw6306_fault_status_t *out)
{
    return sw6306_fault_status_read(out);
}
