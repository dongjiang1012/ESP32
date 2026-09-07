#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** REG0x0F 充放电快充指示的最新原始数据。 */
typedef struct {
    uint8_t raw;
} sw6306_fast_charge_status_t;

/** 最近一次成功读取的 REG0x0F 原始数据。 */
extern sw6306_fast_charge_status_t g_sw6306_fast_charge_status;

/**
 * @brief 进入低页，读取 REG0x0F，并刷新全局结构体。
 *
 * 调用前必须先成功调用 i2c_device_0x3c_init()。
 *
 * @return ESP_OK，或读取过程中遇到的 I2C/寄存器访问错误。
 */
esp_err_t sw6306_fast_charge_status_read(void);

#ifdef __cplusplus
}
#endif
