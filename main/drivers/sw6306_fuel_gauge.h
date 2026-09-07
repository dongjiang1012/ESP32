#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 一个电量计数据项的寄存器原始值和换算值。 */
typedef struct {
    uint32_t raw;
    uint32_t value;
} sw6306_fuel_gauge_value_t;

/**
 * SW6306V 电量计数据。
 *
 * 各成员 value 的单位由成员名给出。两个容量值换算为 uWh，并四舍五入
 * 到最接近的整数；四个电量值的单位为百分比。
 */
typedef struct {
    sw6306_fuel_gauge_value_t maximum_capacity_uwh;
    sw6306_fuel_gauge_value_t current_capacity_uwh;
    sw6306_fuel_gauge_value_t current_level_percent;
    sw6306_fuel_gauge_value_t available_level_percent;
    sw6306_fuel_gauge_value_t equalized_level_percent;
    sw6306_fuel_gauge_value_t display_level_percent;
} sw6306_fuel_gauge_data_t;

/** 最近一次成功读取的电量计数据。 */
extern sw6306_fuel_gauge_data_t g_sw6306_fuel_gauge_data;

/**
 * @brief 读取一次电量计相关寄存器并刷新全局数据。
 *
 * 调用前必须先成功调用 i2c_device_0x3c_init()。本函数进入低页并解锁
 * 一次，连续读取 REG0x86~REG0x8C，再读取 REG0x94 和 REG0x99。全部
 * 读取成功后更新 g_sw6306_fuel_gauge_data，最后退出寄存器写使能。
 *
 * @return ESP_OK，或读取过程中遇到的 I2C/寄存器访问错误。
 */
esp_err_t sw6306_fuel_gauge_read_all(void);

#ifdef __cplusplus
}
#endif
