#pragma once
#include "sw6306_power_types.h"

typedef void (*ui_numeric_result_cb_t)(int32_t value, void *context);
void ui_numeric_input_open(const sw6306_numeric_spec_t *spec, int32_t current,
                           ui_numeric_result_cb_t callback, void *context);
bool ui_numeric_input_is_open(void);
