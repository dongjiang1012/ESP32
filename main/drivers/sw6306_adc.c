#include "sw6306_adc.h"

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "i2c_device_0x3c.h"
#include "sw6306_register_access.h"

#define SW6306_REG_ADC_CONFIG        UINT8_C(0x30)
#define SW6306_REG_ADC_DATA_LOW      UINT8_C(0x31)
#define SW6306_ADC_HIGH_NIBBLE_MASK  UINT8_C(0x0F)
#define SW6306_I2C_TIMEOUT_MS        (100)

typedef enum {
    SW6306_ADC_BUS_VOLTAGE,
    SW6306_ADC_BUS_CURRENT,
    SW6306_ADC_BATTERY_VOLTAGE,
    SW6306_ADC_BATTERY_CURRENT,
    SW6306_ADC_NTC_TEMPERATURE,
    SW6306_ADC_CHIP_TEMPERATURE,
    SW6306_ADC_NTC_VOLTAGE,
    SW6306_ADC_CHANNEL_COUNT,
} sw6306_adc_channel_t;

static const uint8_t s_adc_selectors[SW6306_ADC_CHANNEL_COUNT] = {
    [SW6306_ADC_BUS_VOLTAGE]       = UINT8_C(0),
    [SW6306_ADC_BUS_CURRENT]       = UINT8_C(1),
    [SW6306_ADC_BATTERY_VOLTAGE]   = UINT8_C(2),
    [SW6306_ADC_BATTERY_CURRENT]   = UINT8_C(3),
    [SW6306_ADC_NTC_TEMPERATURE]   = UINT8_C(4),
    [SW6306_ADC_CHIP_TEMPERATURE]  = UINT8_C(9),
    [SW6306_ADC_NTC_VOLTAGE]       = UINT8_C(10),
};

sw6306_adc_data_t g_sw6306_adc_data;

static esp_err_t sw6306_adc_read_channel(i2c_master_dev_handle_t device,
                                         uint8_t selector,
                                         uint16_t *raw)
{
    if (device == NULL || raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t select_data[] = {
        SW6306_REG_ADC_CONFIG,
        selector,
    };

    esp_err_t ret = i2c_master_transmit(device,
                                        select_data,
                                        sizeof(select_data),
                                        SW6306_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        return ret;
    }

    const uint8_t register_address = SW6306_REG_ADC_DATA_LOW;
    uint8_t adc_data[2] = {0};

    ret = i2c_master_transmit_receive(device,
                                      &register_address,
                                      sizeof(register_address),
                                      adc_data,
                                      sizeof(adc_data),
                                      SW6306_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        return ret;
    }

    *raw = ((uint16_t)(adc_data[1] & SW6306_ADC_HIGH_NIBBLE_MASK) << 8)
           | adc_data[0];
    return ESP_OK;
}

esp_err_t sw6306_adc_read_all(void)
{
    esp_err_t ret = sw6306_register_access_enter_low_page();
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw[SW6306_ADC_CHANNEL_COUNT] = {0};
    i2c_master_dev_handle_t device = i2c_device_0x3c_get_handle();
    if (device == NULL) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit_register_access;
    }

    for (size_t index = 0; index < SW6306_ADC_CHANNEL_COUNT; ++index) {
        ret = sw6306_adc_read_channel(device, s_adc_selectors[index], &raw[index]);
        if (ret != ESP_OK) {
            goto exit_register_access;
        }
    }

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

    g_sw6306_adc_data = (sw6306_adc_data_t) {
        .bus_voltage_mv = {
            .raw = raw[SW6306_ADC_BUS_VOLTAGE],
            .value = (int32_t)raw[SW6306_ADC_BUS_VOLTAGE] * 8,
        },
        .bus_current_ma = {
            .raw = raw[SW6306_ADC_BUS_CURRENT],
            .value = (int32_t)raw[SW6306_ADC_BUS_CURRENT] * 4,
        },
        .battery_voltage_mv = {
            .raw = raw[SW6306_ADC_BATTERY_VOLTAGE],
            .value = (int32_t)raw[SW6306_ADC_BATTERY_VOLTAGE] * 7,
        },
        .battery_current_ma = {
            .raw = raw[SW6306_ADC_BATTERY_CURRENT],
            .value = (int32_t)raw[SW6306_ADC_BATTERY_CURRENT] * 5,
        },
        .ntc_temperature_mc = {
            .raw = raw[SW6306_ADC_NTC_TEMPERATURE],
            .value = ((int32_t)raw[SW6306_ADC_NTC_TEMPERATURE] - 16) * 5000,
        },
        .chip_temperature_mc = {
            .raw = raw[SW6306_ADC_CHIP_TEMPERATURE],
            .value = (((int32_t)raw[SW6306_ADC_CHIP_TEMPERATURE] - 1839) * 100000)
                     / 682,
        },
        .ntc_voltage_uv = {
            .raw = raw[SW6306_ADC_NTC_VOLTAGE],
            .value = (int32_t)raw[SW6306_ADC_NTC_VOLTAGE] * 1100,
        },
    };

    return ESP_OK;
}
