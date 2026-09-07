#include "sw6306_config_worker.h"
#include "sw6306_config_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static QueueHandle_t s_requests;
static QueueHandle_t s_results;
static bool s_pending;

static void worker_task(void *argument)
{
    (void)argument;
    sw6306_config_result_t r;
    for (;;) {
        if (xQueueReceive(s_requests, &r, portMAX_DELAY) != pdTRUE) continue;
        switch (r.operation) {
        case SW6306_LOAD_OUTPUT: r.error = sw6306_config_service_load_output(&r.model); break;
        case SW6306_APPLY_OUTPUT: r.error = sw6306_config_service_apply_output(&r.model); break;
        case SW6306_LOAD_BUCK: r.error = sw6306_config_service_load_buck(&r.model); break;
        case SW6306_APPLY_BUCK: r.error = sw6306_config_service_apply_buck(&r.model); break;
        case SW6306_LOAD_ROLE: r.error = sw6306_config_service_load(&r.model); break;
        case SW6306_APPLY_ROLE: r.error = sw6306_config_service_apply(&r.model); break;
        default: r.error = ESP_ERR_INVALID_ARG; break;
        }
        xQueueSend(s_results, &r, portMAX_DELAY);
    }
}

esp_err_t sw6306_config_worker_init(void)
{
    if (s_requests != NULL) return ESP_OK;
    s_requests = xQueueCreate(1, sizeof(sw6306_config_result_t));
    s_results = xQueueCreate(1, sizeof(sw6306_config_result_t));
    if (s_requests == NULL || s_results == NULL ||
        xTaskCreate(worker_task, "sw6306_config", 4096, NULL, 3, NULL) != pdPASS) {
        if (s_requests != NULL) vQueueDelete(s_requests);
        if (s_results != NULL) vQueueDelete(s_results);
        s_requests = NULL;
        s_results = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t sw6306_config_worker_submit(sw6306_config_operation_t operation,
                                    const sw6306_config_model_t *model)
{
    if (model == NULL || (unsigned)operation > SW6306_APPLY_ROLE) return ESP_ERR_INVALID_ARG;
    if (s_requests == NULL || s_pending) return ESP_ERR_INVALID_STATE;
    sw6306_config_result_t r = {.operation = operation, .model = *model, .error = ESP_OK};
    if (xQueueSend(s_requests, &r, 0) != pdTRUE) return ESP_ERR_TIMEOUT;
    s_pending = true;
    return ESP_OK;
}

bool sw6306_config_worker_poll(sw6306_config_result_t *result)
{
    if (s_results == NULL || result == NULL || xQueueReceive(s_results, result, 0) != pdTRUE) return false;
    s_pending = false;
    return true;
}
