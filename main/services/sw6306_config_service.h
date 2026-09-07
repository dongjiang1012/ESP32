#pragma once

#include "esp_err.h"
#include "sw6306_config_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Load the currently readable device configuration into the model. */
esp_err_t sw6306_config_service_load(sw6306_config_model_t *model);

/** Apply the editable configuration and verify it by reading it back. */
esp_err_t sw6306_config_service_apply(sw6306_config_model_t *model);

/** Read, toggle, apply, and verify the current C1 Source/Sink role. */
esp_err_t sw6306_config_service_toggle_c1_role(
    sw6306_config_model_t *model);

/** Apply the existing fixed startup configuration through the service layer. */
esp_err_t sw6306_config_service_apply_startup(void);

esp_err_t sw6306_config_service_load_output(sw6306_config_model_t *model);
esp_err_t sw6306_config_service_apply_output(sw6306_config_model_t *model);
esp_err_t sw6306_config_service_load_buck(sw6306_config_model_t *model);
esp_err_t sw6306_config_service_apply_buck(sw6306_config_model_t *model);

#ifdef __cplusplus
}
#endif
