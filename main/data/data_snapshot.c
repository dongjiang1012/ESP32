#include "data_snapshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_mutex;
static data_snapshot_t s_snapshot;

esp_err_t data_snapshot_init(void) { if (s_mutex != NULL) return ESP_OK; s_mutex = xSemaphoreCreateMutex(); return s_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM; }
static void publish_group(data_group_t group) { s_snapshot.state[group].valid = true; s_snapshot.state[group].last_error = ESP_OK; ++s_snapshot.state[group].update_count; }
void data_snapshot_publish_fast_charge(const sw6306_fast_charge_status_t *data) { if (s_mutex == NULL || data == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return; s_snapshot.fast_charge = *data; publish_group(DATA_GROUP_FAST_CHARGE); xSemaphoreGive(s_mutex); }
void data_snapshot_publish_adc(const sw6306_adc_data_t *data, int32_t power_mw) { if (s_mutex == NULL || data == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return; s_snapshot.adc = *data; s_snapshot.power_mw = power_mw; publish_group(DATA_GROUP_ADC); xSemaphoreGive(s_mutex); }
void data_snapshot_publish_fuel_gauge(const sw6306_fuel_gauge_data_t *data) { if (s_mutex == NULL || data == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return; s_snapshot.fuel_gauge = *data; publish_group(DATA_GROUP_FUEL_GAUGE); xSemaphoreGive(s_mutex); }
void data_snapshot_publish_fault_status(const sw6306_fault_status_t *data) { if (s_mutex == NULL || data == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return; s_snapshot.fault_status = *data; publish_group(DATA_GROUP_FAULT_STATUS); xSemaphoreGive(s_mutex); }
void data_snapshot_report_error(data_group_t group, esp_err_t error) { if (s_mutex == NULL || group >= DATA_GROUP_COUNT || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return; s_snapshot.state[group].last_error = error; xSemaphoreGive(s_mutex); }
esp_err_t data_snapshot_read(data_snapshot_t *out) { if (out == NULL) return ESP_ERR_INVALID_ARG; if (s_mutex == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_INVALID_STATE; *out = s_snapshot; xSemaphoreGive(s_mutex); return ESP_OK; }
