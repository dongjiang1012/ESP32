#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 一个 ADC 通道的原始值和换算值。 */
typedef struct {
    uint16_t raw;
    int32_t value;
} sw6306_adc_channel_data_t;

/**
 * SW6306V 的 7 个 ADC 通道数据。
 *
 * 各成员 value 的单位由成员名给出；raw 为 REG0x31/REG0x32 组成的
 * 12 位原始 ADC 数据。
 */
typedef struct {
    sw6306_adc_channel_data_t bus_voltage_mv;
    sw6306_adc_channel_data_t bus_current_ma;
    sw6306_adc_channel_data_t battery_voltage_mv;
    sw6306_adc_channel_data_t battery_current_ma;
    sw6306_adc_channel_data_t ntc_temperature_mc;
    sw6306_adc_channel_data_t chip_temperature_mc;
    sw6306_adc_channel_data_t ntc_voltage_uv;
} sw6306_adc_data_t;

/** 最近一次成功读取的 7 通道 ADC 数据。 */
extern sw6306_adc_data_t g_sw6306_adc_data;

/**
 * @brief 读取一次全部 7 个 ADC 通道并刷新 g_sw6306_adc_data。
 *
 * 调用前必须先成功调用 i2c_device_0x3c_init()。本函数进入低页并解锁
 * 一次，然后依次选择 7 个 ADC 通道。每次从 REG0x31 开始连续读取两个
 * 字节，读取全部成功后更新全局数据，最后退出寄存器写使能。
 *
 * @return ESP_OK，或读取过程中遇到的 I2C/寄存器访问错误。
 */
esp_err_t sw6306_adc_read_all(void);

#ifdef __cplusplus
}
#endif
