#pragma once

#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the I2C device at 7-bit address 0x3C. */
esp_err_t i2c_device_0x3c_init(void);

/** Return the handle for the 7-bit I2C device at address 0x3C. */
i2c_master_dev_handle_t i2c_device_0x3c_get_handle(void);

#ifdef __cplusplus
}
#endif
