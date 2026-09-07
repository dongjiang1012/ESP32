#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 应用 SW6306V 上电预配置。
 *
 * 调用前必须先成功调用 i2c_device_0x3c_init()。当前配置会将
 * REG0x100 写为 0x0E，把最大输出功率设置为 100W，
 * 并将 REG0x108 写为 4S、4.2V 锂电池配置。
 *
 * @return ESP_OK，或配置过程中遇到的 I2C/寄存器访问错误。
 */
esp_err_t sw6306_startup_config_apply(void);

#ifdef __cplusplus
}
#endif
