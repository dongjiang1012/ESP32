#include "data_scheduler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "data_snapshot.h"
#include "services/sw6306_data_service.h"

#define TICK_PERIOD_MS 50U
typedef esp_err_t (*collect_fn_t)(void);
typedef struct { uint32_t period_ms; uint32_t elapsed_ms; collect_fn_t collect; } collect_job_t;

static esp_err_t collect_fast_charge(void) {
    sw6306_fast_charge_status_t data;
    esp_err_t ret = sw6306_data_service_read_fast_charge(&data);
    if (ret != ESP_OK) data_snapshot_report_error(DATA_GROUP_FAST_CHARGE, ret);
    else data_snapshot_publish_fast_charge(&data);
    return ret;
}

static esp_err_t collect_adc(void) {
    sw6306_adc_data_t local;
    int32_t power_mw = 0;
    esp_err_t ret = sw6306_data_service_read_adc(&local, &power_mw);
    if (ret != ESP_OK) { data_snapshot_report_error(DATA_GROUP_ADC, ret); return ret; }
    data_snapshot_publish_adc(&local, power_mw);
    return ESP_OK;
}

static esp_err_t collect_fuel_gauge(void) {
    sw6306_fuel_gauge_data_t data;
    esp_err_t ret = sw6306_data_service_read_fuel_gauge(&data);
    if (ret != ESP_OK) data_snapshot_report_error(DATA_GROUP_FUEL_GAUGE, ret);
    else data_snapshot_publish_fuel_gauge(&data);
    return ret;
}

static esp_err_t collect_fault_status(void) {
    sw6306_fault_status_t data;
    esp_err_t ret = sw6306_data_service_read_fault_status(&data);
    if (ret != ESP_OK) data_snapshot_report_error(DATA_GROUP_FAULT_STATUS, ret);
    else data_snapshot_publish_fault_status(&data);
    return ret;
}

static collect_job_t s_jobs[] = {
    { 100U, 0, collect_fast_charge },
    { 500U, 0, collect_adc },
    { 2000U, 0, collect_fuel_gauge },
    { 200U, 0, collect_fault_status },
};

void data_collector_task(void *arg) {
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        for (size_t i = 0; i < sizeof(s_jobs) / sizeof(s_jobs[0]); ++i) {
            s_jobs[i].elapsed_ms += TICK_PERIOD_MS;
            if (s_jobs[i].elapsed_ms >= s_jobs[i].period_ms) {
                s_jobs[i].elapsed_ms = 0;
                (void)s_jobs[i].collect();
            }
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(TICK_PERIOD_MS));
    }
}
