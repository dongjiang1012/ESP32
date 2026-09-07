#include "sw6306_typec_mode.h"

#include <stddef.h>

#include "driver/i2c_master.h"
#include "i2c_device_0x3c.h"
#include "sw6306_register_access.h"

/*
 * REG0x132 lives in the high register page.  While the high page is selected,
 * its low eight address bits (0x32) are sent on the wire.
 */
#define SW6306_REG_TYPEC_CONFIG_LOW_ADDRESS UINT8_C(0x32)

/* REG0x132 bits 1-0 select the C1 port Type-C role. */
#define SW6306_TYPEC_C1_ROLE_MASK   UINT8_C(0x03)
#define SW6306_TYPEC_C1_ROLE_SHIFT  UINT8_C(0)

#define SW6306_I2C_TIMEOUT_MS       (100)

static esp_err_t sw6306_typec_mode_get_device(i2c_master_dev_handle_t *device)
{
    if (device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *device = i2c_device_0x3c_get_handle();
    if (*device == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

static esp_err_t sw6306_typec_mode_read_register(i2c_master_dev_handle_t device,
                                                 uint8_t *value)
{
    const uint8_t register_address = SW6306_REG_TYPEC_CONFIG_LOW_ADDRESS;

    return i2c_master_transmit_receive(device,
                                       &register_address,
                                       sizeof(register_address),
                                       value,
                                       sizeof(*value),
                                       SW6306_I2C_TIMEOUT_MS);
}

static esp_err_t sw6306_typec_mode_write_register(i2c_master_dev_handle_t device,
                                                  uint8_t value)
{
    const uint8_t transaction[] = {
        SW6306_REG_TYPEC_CONFIG_LOW_ADDRESS,
        value,
    };

    return i2c_master_transmit(device,
                               transaction,
                               sizeof(transaction),
                               SW6306_I2C_TIMEOUT_MS);
}

esp_err_t sw6306_typec_mode_read_c1_role(sw6306_c1_role_t *role)
{
    i2c_master_dev_handle_t device = NULL;
    if (role == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = sw6306_typec_mode_get_device(&device);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sw6306_register_access_enter_high_page();
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t value = 0;
    ret = sw6306_typec_mode_read_register(device, &value);

    const esp_err_t exit_ret = sw6306_register_access_exit();
    if (ret != ESP_OK) {
        return ret;
    }

    *role = (sw6306_c1_role_t)((value >> SW6306_TYPEC_C1_ROLE_SHIFT) &
                               SW6306_TYPEC_C1_ROLE_MASK);
    return exit_ret;
}

esp_err_t sw6306_typec_mode_set_c1_role(sw6306_c1_role_t role)
{
    if (role == SW6306_C1_ROLE_RESERVED) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_dev_handle_t device = NULL;
    esp_err_t ret = sw6306_typec_mode_get_device(&device);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sw6306_register_access_enter_high_page();
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t value = 0;
    ret = sw6306_typec_mode_read_register(device, &value);
    if (ret == ESP_OK) {
        value = (uint8_t)((value & ~SW6306_TYPEC_C1_ROLE_MASK) |
                          ((uint8_t)role << SW6306_TYPEC_C1_ROLE_SHIFT));
        ret = sw6306_typec_mode_write_register(device, value);
    }

    const esp_err_t exit_ret = sw6306_register_access_exit();
    if (ret != ESP_OK) {
        return ret;
    }

    return exit_ret;
}

esp_err_t sw6306_typec_mode_toggle_c1_source_sink(sw6306_c1_role_t *new_role)
{
    sw6306_c1_role_t role = SW6306_C1_ROLE_SOURCE;
    esp_err_t ret = sw6306_typec_mode_read_c1_role(&role);
    if (ret != ESP_OK) {
        return ret;
    }

    const sw6306_c1_role_t target = role == SW6306_C1_ROLE_SINK
                                        ? SW6306_C1_ROLE_SOURCE
                                        : SW6306_C1_ROLE_SINK;

    ret = sw6306_typec_mode_set_c1_role(target);
    if (ret != ESP_OK) {
        return ret;
    }

    if (new_role != NULL) {
        *new_role = target;
    }

    return ESP_OK;
}
