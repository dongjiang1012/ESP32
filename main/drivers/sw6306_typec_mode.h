#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C1 port Type-C role selection, REG0x132 bits 1-0 (high register page).
 */
typedef enum {
    SW6306_C1_ROLE_DRP_TRY_SRC = UINT8_C(0),
    SW6306_C1_ROLE_SOURCE      = UINT8_C(1),
    SW6306_C1_ROLE_SINK        = UINT8_C(2),
    SW6306_C1_ROLE_RESERVED    = UINT8_C(3),
} sw6306_c1_role_t;

/**
 * Read the current C1 port role from REG0x132[1:0].
 *
 * Call i2c_device_0x3c_init() before using this function.  All register page
 * selection is serialized by the caller, exactly as documented in
 * sw6306_register_access.h.
 *
 * @param role Receives the current C1 role on success.
 * @return ESP_OK on success, or the first I2C error encountered.
 */
esp_err_t sw6306_typec_mode_read_c1_role(sw6306_c1_role_t *role);

/**
 * Write the C1 port role into REG0x132[1:0].
 *
 * The remaining register bits (including the VCONN over-current detection
 * enable bit) are preserved by a read-modify-write sequence.
 *
 * @param role C1 role to apply.  SW6306_C1_ROLE_RESERVED is rejected.
 * @return ESP_OK on success, or the first I2C error encountered.
 */
esp_err_t sw6306_typec_mode_set_c1_role(sw6306_c1_role_t role);

/**
 * Toggle the C1 port between "only source" and "only sink".
 *
 * A current "only sink" setting switches to "only source"; every other value
 * (including the DRP default) switches to "only source".
 *
 * @param new_role Receives the role written to REG0x132 on success.
 * @return ESP_OK on success, or the first I2C error encountered.
 */
esp_err_t sw6306_typec_mode_toggle_c1_source_sink(sw6306_c1_role_t *new_role);

#ifdef __cplusplus
}
#endif
