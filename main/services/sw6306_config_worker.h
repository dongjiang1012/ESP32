#pragma once
#include "sw6306_config_model.h"
#include "esp_err.h"

typedef enum {
    SW6306_LOAD_OUTPUT,
    SW6306_APPLY_OUTPUT,
    SW6306_LOAD_BUCK,
    SW6306_APPLY_BUCK,
    SW6306_LOAD_ROLE,
    SW6306_APPLY_ROLE,
} sw6306_config_operation_t;

typedef struct {
    sw6306_config_operation_t operation;
    sw6306_config_model_t model;
    esp_err_t error;
} sw6306_config_result_t;

esp_err_t sw6306_config_worker_init(void);
/* Call submit/poll from the LVGL task only. Copies values, never UI pointers. */
esp_err_t sw6306_config_worker_submit(sw6306_config_operation_t operation,
                                    const sw6306_config_model_t *model);
bool sw6306_config_worker_poll(sw6306_config_result_t *result);
