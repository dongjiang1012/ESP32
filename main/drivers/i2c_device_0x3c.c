#include "i2c_device_0x3c.h"

#include "sdkconfig.h"
#include "bsp/esp-bsp.h"

#define I2C_DEVICE_0X3C_ADDRESS  (0x3C)  // 设置 SW6306 地址

static i2c_master_dev_handle_t dev = NULL;

// 初始化 0x3C 的设备
esp_err_t i2c_device_0x3c_init(void)
{
    if (dev != NULL) {
        return ESP_OK;
    }

    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (bus == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEVICE_0X3C_ADDRESS,
        .scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ,
    };

    return i2c_master_bus_add_device(bus, &dev_cfg, &dev);
}

i2c_master_dev_handle_t i2c_device_0x3c_get_handle(void)
{
    return dev;
}
