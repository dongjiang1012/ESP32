#include "sw6306_startup_config.h"

#include <stdint.h>

#include "driver/i2c_master.h"
#include "i2c_device_0x3c.h"
#include "sw6306_register_access.h"

/* 高页已选中时，REG0x100 在线路上的低 8 位地址为 0x00。 */
#define SW6306_REG_DISCHARGE_CONFIG_0_LOW_ADDRESS UINT8_C(0x00)
#define SW6306_DISCHARGE_MAX_POWER_100W           UINT8_C(0x0E)

/*
 * 高页已选中时，REG0x107 在线路上的低 8 位地址为 0x07。
 * bit3 = 1：输入最高功率由 REG0x107[2:0] 设置；
 * bit2:0 = 6：输入最高功率为 100 W。
 */
#define SW6306_REG_CHARGE_CONFIG_0_LOW_ADDRESS    UINT8_C(0x07)
#define SW6306_INPUT_POWER_FROM_REGISTER          UINT8_C(0x08)
#define SW6306_INPUT_MAX_POWER_100W                UINT8_C(0x06)
#define SW6306_CHARGE_CONFIG_0_100W                \
    (SW6306_INPUT_POWER_FROM_REGISTER | SW6306_INPUT_MAX_POWER_100W)

/*
 * 高页已选中时，REG0x108 在线路上的低 8 位地址为 0x08。
 * bit7 = 1：电池类型由 REG0x108[6:4] 设置；
 * bit6:4 = 0：4.2V 锂电池；
 * bit3 = 1：电池节数由 REG0x108[2:0] 设置；
 * bit2:0 = 4：4S。
 */
#define SW6306_REG_CHARGE_CONFIG_1_LOW_ADDRESS    UINT8_C(0x08)
#define SW6306_BATTERY_TYPE_FROM_REGISTER         UINT8_C(0x80)
#define SW6306_BATTERY_TYPE_4V2                   UINT8_C(0x00)
#define SW6306_CELL_COUNT_FROM_REGISTER           UINT8_C(0x08)
#define SW6306_CELL_COUNT_4S                      UINT8_C(0x04)
#define SW6306_CHARGE_CONFIG_1_4S_4V2             \
    (SW6306_BATTERY_TYPE_FROM_REGISTER | SW6306_BATTERY_TYPE_4V2 | \
     SW6306_CELL_COUNT_FROM_REGISTER | SW6306_CELL_COUNT_4S)

#define SW6306_I2C_TIMEOUT_MS                     (100)

esp_err_t sw6306_startup_config_apply(void)
{
    esp_err_t ret = sw6306_register_access_enter_high_page();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_dev_handle_t device = i2c_device_0x3c_get_handle();
    if (device == NULL) {
        ret = ESP_ERR_INVALID_STATE;
    } else {
        const uint8_t discharge_config[] = {
            SW6306_REG_DISCHARGE_CONFIG_0_LOW_ADDRESS,
            SW6306_DISCHARGE_MAX_POWER_100W,
        };

        ret = i2c_master_transmit(device,
                                  discharge_config,
                                  sizeof(discharge_config),
                                  SW6306_I2C_TIMEOUT_MS);

        if (ret == ESP_OK) {
            const uint8_t charge_config[] = {
                SW6306_REG_CHARGE_CONFIG_0_LOW_ADDRESS,
                SW6306_CHARGE_CONFIG_0_100W,
            };

            ret = i2c_master_transmit(device,
                                      charge_config,
                                      sizeof(charge_config),
                                      SW6306_I2C_TIMEOUT_MS);

            if (ret == ESP_OK) {
                const uint8_t battery_config[] = {
                    SW6306_REG_CHARGE_CONFIG_1_LOW_ADDRESS,
                    SW6306_CHARGE_CONFIG_1_4S_4V2,
                };

                ret = i2c_master_transmit(device,
                                          battery_config,
                                          sizeof(battery_config),
                                          SW6306_I2C_TIMEOUT_MS);
            }
        }
    }

    const esp_err_t exit_ret = sw6306_register_access_exit();
    if (ret != ESP_OK) {
        return ret;
    }

    return exit_ret;
}
