#include "sw6306_fuel_gauge.h"

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "i2c_device_0x3c.h"
#include "sw6306_register_access.h"

#define SW6306_REG_MAXIMUM_CAPACITY_LOW  UINT8_C(0x86)
#define SW6306_REG_EQUALIZED_LEVEL       UINT8_C(0x94)
#define SW6306_REG_DISPLAY_LEVEL         UINT8_C(0x99)

#define SW6306_REG_0X86_TO_0X8C_COUNT    (7)
#define SW6306_MAXIMUM_CAPACITY_HIGH_MASK UINT8_C(0x0F)
#define SW6306_I2C_TIMEOUT_MS             (100)

sw6306_fuel_gauge_data_t g_sw6306_fuel_gauge_data;

static esp_err_t sw6306_fuel_gauge_read_registers(
    i2c_master_dev_handle_t device,
    uint8_t start_register,
    uint8_t *data,
    size_t data_size)
{
    if (device == NULL || data == NULL || data_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit_receive(device,
                                       &start_register,
                                       sizeof(start_register),
                                       data,
                                       data_size,
                                       SW6306_I2C_TIMEOUT_MS);
}

esp_err_t sw6306_fuel_gauge_read_all(void)
{
    esp_err_t ret = sw6306_register_access_enter_low_page();
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t registers_0x86_to_0x8c[SW6306_REG_0X86_TO_0X8C_COUNT] = {0};
    uint8_t equalized_level = 0;
    uint8_t display_level = 0;

    i2c_master_dev_handle_t device = i2c_device_0x3c_get_handle();
    if (device == NULL) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit_register_access;
    }

    ret = sw6306_fuel_gauge_read_registers(
        device,
        SW6306_REG_MAXIMUM_CAPACITY_LOW,
        registers_0x86_to_0x8c,
        sizeof(registers_0x86_to_0x8c));
    if (ret != ESP_OK) {
        goto exit_register_access;
    }

    ret = sw6306_fuel_gauge_read_registers(device,
                                            SW6306_REG_EQUALIZED_LEVEL,
                                            &equalized_level,
                                            sizeof(equalized_level));
    if (ret != ESP_OK) {
        goto exit_register_access;
    }

    ret = sw6306_fuel_gauge_read_registers(device,
                                            SW6306_REG_DISPLAY_LEVEL,
                                            &display_level,
                                            sizeof(display_level));

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

    const uint16_t maximum_capacity_raw =
        ((uint16_t)(registers_0x86_to_0x8c[1] &
                    SW6306_MAXIMUM_CAPACITY_HIGH_MASK) << 8) |
        registers_0x86_to_0x8c[0];

    const uint32_t current_capacity_raw =
        ((uint32_t)registers_0x86_to_0x8c[4] << 16) |
        ((uint32_t)registers_0x86_to_0x8c[3] << 8) |
        registers_0x86_to_0x8c[2];

    g_sw6306_fuel_gauge_data = (sw6306_fuel_gauge_data_t) {
        .maximum_capacity_uwh = {
            .raw = maximum_capacity_raw,
            .value = (uint32_t)(((uint64_t)maximum_capacity_raw *
                                 UINT64_C(3262236) + UINT64_C(5)) /
                                UINT64_C(10)),
        },
        .current_capacity_uwh = {
            .raw = current_capacity_raw,
            .value = (uint32_t)(((uint64_t)current_capacity_raw *
                                 UINT64_C(7964) + UINT64_C(50)) /
                                UINT64_C(100)),
        },
        .current_level_percent = {
            .raw = registers_0x86_to_0x8c[5],
            .value = registers_0x86_to_0x8c[5],
        },
        .available_level_percent = {
            .raw = registers_0x86_to_0x8c[6],
            .value = registers_0x86_to_0x8c[6],
        },
        .equalized_level_percent = {
            .raw = equalized_level,
            .value = equalized_level,
        },
        .display_level_percent = {
            .raw = display_level,
            .value = display_level,
        },
    };

    return ESP_OK;
}
