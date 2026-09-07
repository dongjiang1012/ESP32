#include "sw6306_fast_charge_status.h"

#include <stdint.h>

#include "driver/i2c_master.h"
#include "i2c_device_0x3c.h"
#include "sw6306_register_access.h"

#define SW6306_REG_FAST_CHARGE_STATUS  UINT8_C(0x0F)
#define SW6306_I2C_TIMEOUT_MS          (100)

sw6306_fast_charge_status_t g_sw6306_fast_charge_status;

esp_err_t sw6306_fast_charge_status_read(void)
{
    esp_err_t ret = sw6306_register_access_enter_low_page();
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t raw = 0;
    i2c_master_dev_handle_t device = i2c_device_0x3c_get_handle();
    if (device == NULL) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit_register_access;
    }

    const uint8_t register_address = SW6306_REG_FAST_CHARGE_STATUS;
    ret = i2c_master_transmit_receive(device,
                                      &register_address,
                                      sizeof(register_address),
                                      &raw,
                                      sizeof(raw),
                                      SW6306_I2C_TIMEOUT_MS);

exit_register_access:
    {
        const esp_err_t exit_ret = sw6306_register_access_exit();
        if (ret == ESP_OK && exit_ret != ESP_OK) {
            ret = exit_ret;
        }
    }

    if (ret != ESP_OK) {
        return ret;
    }

    g_sw6306_fast_charge_status.raw = raw;
    return ESP_OK;
}
