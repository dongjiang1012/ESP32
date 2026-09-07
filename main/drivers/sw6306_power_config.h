#pragma once
#include "esp_err.h"
#include "sw6306_power_types.h"

esp_err_t sw6306_power_read_output(sw6306_output_config_t *config);
esp_err_t sw6306_power_write_output(const sw6306_output_config_t *config);
esp_err_t sw6306_power_read_buck(sw6306_buck_config_t *config);
esp_err_t sw6306_power_write_buck(const sw6306_buck_config_t *config);
