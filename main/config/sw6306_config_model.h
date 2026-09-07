#pragma once

#include <stdbool.h>

#include "sw6306_typec_mode.h"
#include "sw6306_power_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Device configuration currently exposed by the application.
 *
 * This is the editable application-side model. It deliberately contains no
 * register addresses or I2C details.
 */
typedef struct {
    sw6306_c1_role_t c1_role;
    sw6306_c1_role_t c1_role_actual;
    bool c1_role_valid;
    bool c1_role_dirty;
    sw6306_output_config_t output;
    sw6306_output_config_t output_actual;
    bool output_valid;
    bool output_dirty;
    sw6306_buck_config_t buck;
    sw6306_buck_config_t buck_actual;
    bool buck_valid;
    bool buck_dirty;
} sw6306_config_model_t;

void sw6306_config_model_init(sw6306_config_model_t *model);

void sw6306_config_model_set_c1_role(sw6306_config_model_t *model,
                                     sw6306_c1_role_t role,
                                     bool mark_dirty);

bool sw6306_config_model_is_ready(const sw6306_config_model_t *model);

#ifdef __cplusplus
}
#endif
