#include "sw6306_register_access.h"

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "i2c_device_0x3c.h"

#define SW6306_REG_LOW_POWER_CONTROL       UINT8_C(0x23)
#define SW6306_REG_I2C_WRITE_ENABLE        UINT8_C(0x24)
#define SW6306_HIGH_PAGE_EXIT_ADDRESS      UINT8_C(0xFF)

#define SW6306_LOW_POWER_DISABLED          UINT8_C(0x01)
#define SW6306_WRITE_UNLOCK_STEP_1         UINT8_C(0x20)
#define SW6306_WRITE_UNLOCK_STEP_2         UINT8_C(0x40)
#define SW6306_WRITE_UNLOCK_LOW_PAGE       UINT8_C(0x80)
#define SW6306_WRITE_UNLOCK_HIGH_PAGE      UINT8_C(0x81)
#define SW6306_WRITE_ACCESS_DISABLED       UINT8_C(0x00)

#define SW6306_I2C_TIMEOUT_MS              (100)

typedef enum {
    SW6306_REGISTER_PAGE_LOW,
    SW6306_REGISTER_PAGE_HIGH,
    SW6306_REGISTER_PAGE_UNKNOWN,
} sw6306_register_page_t;

/* REG0x24 bit 0 defaults to zero after an SW6306V reset. */
static sw6306_register_page_t s_current_page = SW6306_REGISTER_PAGE_LOW;
static SemaphoreHandle_t s_access_mutex;

static esp_err_t sw6306_register_access_lock(void)
{
    if (s_access_mutex == NULL) {
        s_access_mutex = xSemaphoreCreateRecursiveMutex();
        if (s_access_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    return xSemaphoreTakeRecursive(s_access_mutex, portMAX_DELAY) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

static void sw6306_register_access_unlock(void)
{
    if (s_access_mutex != NULL) {
        xSemaphoreGiveRecursive(s_access_mutex);
    }
}

esp_err_t sw6306_register_access_init(void)
{
    if (s_access_mutex != NULL) {
        return ESP_OK;
    }

    s_access_mutex = xSemaphoreCreateRecursiveMutex();
    return s_access_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t sw6306_get_device(i2c_master_dev_handle_t *device)
{
    if (device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *device = i2c_device_0x3c_get_handle();
    if (*device == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

static esp_err_t sw6306_write_register(i2c_master_dev_handle_t device,
                                       uint8_t register_address,
                                       uint8_t value)
{
    const uint8_t transaction[] = { register_address, value };

    return i2c_master_transmit(device,
                               transaction,
                               sizeof(transaction),
                               SW6306_I2C_TIMEOUT_MS);
}

static esp_err_t sw6306_read_register(i2c_master_dev_handle_t device,
                                      uint8_t register_address,
                                      uint8_t *value)
{
    if (device == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit_receive(device,
                                       &register_address,
                                       sizeof(register_address),
                                       value,
                                       sizeof(*value),
                                       SW6306_I2C_TIMEOUT_MS);
}

static esp_err_t sw6306_return_to_low_page(i2c_master_dev_handle_t device)
{
    if (s_current_page == SW6306_REGISTER_PAGE_LOW) {
        return ESP_OK;
    }

    if (s_current_page == SW6306_REGISTER_PAGE_UNKNOWN) {
        /*
         * With an unknown page, writing address 0xFF is unsafe: it may mean
         * REG0x0FF instead of REG0x1FF.  Reset both the SW6306V and this MCU
         * to restore the documented default low-page state and page tracking.
         */
        return ESP_ERR_INVALID_STATE;
    }

    /* While bit 8 is one, address byte 0xFF selects REG0x1FF. */
    esp_err_t ret = sw6306_write_register(device,
                                          SW6306_HIGH_PAGE_EXIT_ADDRESS,
                                          SW6306_WRITE_ACCESS_DISABLED);
    if (ret != ESP_OK) {
        s_current_page = SW6306_REGISTER_PAGE_UNKNOWN;
        return ret;
    }

    s_current_page = SW6306_REGISTER_PAGE_LOW;
    return ESP_OK;
}

static esp_err_t sw6306_unlock_low_page(i2c_master_dev_handle_t device)
{
    esp_err_t ret = sw6306_write_register(device,
                                          SW6306_REG_LOW_POWER_CONTROL,
                                          SW6306_LOW_POWER_DISABLED);
    if (ret != ESP_OK) {
        return ret;
    }

    static const uint8_t unlock_sequence[] = {
        SW6306_WRITE_UNLOCK_STEP_1,
        SW6306_WRITE_UNLOCK_STEP_2,
        SW6306_WRITE_UNLOCK_LOW_PAGE,
    };

    for (size_t index = 0; index < sizeof(unlock_sequence); ++index) {
        ret = sw6306_write_register(device,
                                    SW6306_REG_I2C_WRITE_ENABLE,
                                    unlock_sequence[index]);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t sw6306_register_access_enter_low_page(void)
{
    esp_err_t ret = sw6306_register_access_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_dev_handle_t device = NULL;
    ret = sw6306_get_device(&device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_return_to_low_page(device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_unlock_low_page(device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
    }
    return ret;
}

esp_err_t sw6306_register_access_enter_high_page(void)
{
    esp_err_t ret = sw6306_register_access_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_dev_handle_t device = NULL;
    ret = sw6306_get_device(&device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_return_to_low_page(device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_unlock_low_page(device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_write_register(device,
                                SW6306_REG_I2C_WRITE_ENABLE,
                                SW6306_WRITE_UNLOCK_HIGH_PAGE);
    if (ret != ESP_OK) {
        /* The transfer may have reached the device before the error surfaced. */
        s_current_page = SW6306_REGISTER_PAGE_UNKNOWN;
        sw6306_register_access_unlock();
        return ret;
    }

    s_current_page = SW6306_REGISTER_PAGE_HIGH;
    return ESP_OK;
}

esp_err_t sw6306_register_access_exit(void)
{
    i2c_master_dev_handle_t device = NULL;
    esp_err_t ret = sw6306_get_device(&device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_return_to_low_page(device);
    if (ret != ESP_OK) {
        sw6306_register_access_unlock();
        return ret;
    }

    ret = sw6306_write_register(device,
                                SW6306_REG_I2C_WRITE_ENABLE,
                                SW6306_WRITE_ACCESS_DISABLED);
    sw6306_register_access_unlock();
    return ret;
}

esp_err_t sw6306_register_access_begin(void)
{
    return sw6306_register_access_lock();
}

esp_err_t sw6306_register_access_end(void)
{
    if (s_access_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    sw6306_register_access_unlock();
    return ESP_OK;
}

static esp_err_t sw6306_register_validate_address(uint16_t address)
{
    return address <= UINT16_C(0x1FF) ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static esp_err_t sw6306_register_read_locked(uint16_t address, uint8_t *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = sw6306_register_validate_address(address);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = (address & UINT16_C(0x100)) != 0
              ? sw6306_register_access_enter_high_page()
              : sw6306_register_access_enter_low_page();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_dev_handle_t device = NULL;
    ret = sw6306_get_device(&device);
    if (ret == ESP_OK) {
        ret = sw6306_read_register(device, (uint8_t)address, value);
    }

    const esp_err_t exit_ret = sw6306_register_access_exit();
    if (ret == ESP_OK && exit_ret != ESP_OK) {
        ret = exit_ret;
    }
    return ret;
}

static esp_err_t sw6306_register_write_locked(uint16_t address, uint8_t value)
{
    esp_err_t ret = sw6306_register_validate_address(address);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = (address & UINT16_C(0x100)) != 0
              ? sw6306_register_access_enter_high_page()
              : sw6306_register_access_enter_low_page();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_dev_handle_t device = NULL;
    ret = sw6306_get_device(&device);
    if (ret == ESP_OK) {
        ret = sw6306_write_register(device, (uint8_t)address, value);
    }

    const esp_err_t exit_ret = sw6306_register_access_exit();
    if (ret == ESP_OK && exit_ret != ESP_OK) {
        ret = exit_ret;
    }
    return ret;
}

esp_err_t sw6306_register_read(uint16_t address, uint8_t *value)
{
    esp_err_t ret = sw6306_register_access_begin();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sw6306_register_read_locked(address, value);
    (void)sw6306_register_access_end();
    return ret;
}

esp_err_t sw6306_register_write(uint16_t address, uint8_t value)
{
    esp_err_t ret = sw6306_register_access_begin();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sw6306_register_write_locked(address, value);
    (void)sw6306_register_access_end();
    return ret;
}

esp_err_t sw6306_register_update_bits(uint16_t address,
                                      uint8_t mask,
                                      uint8_t value)
{
    esp_err_t ret = sw6306_register_access_begin();
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t current = 0;
    ret = sw6306_register_read_locked(address, &current);
    if (ret == ESP_OK) {
        const uint8_t updated = (uint8_t)((current & (uint8_t)~mask) |
                                          (value & mask));
        if (updated != current) {
            ret = sw6306_register_write_locked(address, updated);
        }
    }

    (void)sw6306_register_access_end();
    return ret;
}
