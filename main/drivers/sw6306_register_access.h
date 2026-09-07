#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Call i2c_device_0x3c_init() before using these functions.
 *
 * The generic read/write functions below are transaction-safe: they lock the
 * SW6306 access mutex, select the required page, perform the I2C operation,
 * return to the low page, and unlock the device. Use begin/end when several
 * register operations must be kept atomic.
 */

/**
 * Acquire/release the SW6306 register access transaction lock.
 *
 * The lock is recursive. The legacy enter/exit page APIs also acquire and
 * release one level of this lock, so existing multi-register drivers remain
 * serialized while they are migrated to the generic APIs.
 */
esp_err_t sw6306_register_access_init(void);
esp_err_t sw6306_register_access_begin(void);
esp_err_t sw6306_register_access_end(void);

/** Read or write one 8-bit register using its 16-bit SW6306 address. */
esp_err_t sw6306_register_read(uint16_t address, uint8_t *value);
esp_err_t sw6306_register_write(uint16_t address, uint8_t value);

/** Update only the bits selected by mask in one register. */
esp_err_t sw6306_register_update_bits(uint16_t address,
                                      uint8_t mask,
                                      uint8_t value);

/**
 * @brief Enter the low register page and enable protected-register writes.
 *
 * On success, defined registers below 0x100 can be accessed.  The function
 * disables shutdown low-power mode and sends the complete SW6306V
 * write-unlock sequence.
 *
 * This function acquires one level of the access lock. Pair every successful
 * call with sw6306_register_access_exit().
 *
 * @return ESP_OK on success, or the first I2C error encountered.
 */
esp_err_t sw6306_register_access_enter_low_page(void);

/**
 * @brief Enter the high register page and enable protected-register writes.
 *
 * On success, registers 0x100 through 0x156 can be accessed by sending their
 * low eight address bits to the device.
 *
 * This function acquires one level of the access lock. Pair every successful
 * call with sw6306_register_access_exit().
 *
 * @return ESP_OK on success, or the first I2C error encountered.
 */
esp_err_t sw6306_register_access_enter_high_page(void);

/**
 * @brief Return to the low page and disable protected-register writes.
 *
 * This writes REG0x24 = 0x00 after returning from the high page when needed.
 * It intentionally leaves REG0x23 unchanged, so shutdown low-power mode is not
 * re-enabled by this function.
 *
 * This function returns to the low page, disables protected writes, and
 * releases the lock level acquired by the matching enter call.
 *
 * @return ESP_OK on success, or the first I2C error encountered.
 */
esp_err_t sw6306_register_access_exit(void);

#ifdef __cplusplus
}
#endif
