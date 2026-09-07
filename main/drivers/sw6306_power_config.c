#include "sw6306_power_config.h"
#include "sw6306_register_access.h"
#include <stddef.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* SW6306V manual V1.0.2 pp.5-11,16-19,25,35-37. Preserve all unrelated bits. */
#define REG_FORCE 0x040
#define REG_VOLTAGE_LOW 0x041
#define REG_VOLTAGE_HIGH 0x042
#define REG_OUTPUT_IBUS 0x043
#define REG_CHARGE_IBUS 0x049
#define REG_BUCK0 0x114
#define REG_BUCK1 0x115
#define REG_BUCK2 0x116
#define FORCE_VOLTAGE 0x01
#define FORCE_IBUS 0x40
#define REG_SYSTEM_STATUS 0x018
#define REG_PORT_STATUS 0x01D
#define REG_TYPEC_STATUS 0x019
#define REG_C1_ROLE 0x132
#define REG_PORT_EVENT 0x022
#define REG_NO_LOAD 0x118
#define REG_VOLTAGE_COMPENSATION 0x101
#define REG_MODE 0x028
#define OUTPUT_OFF 0x08
#define DISCHARGE_AND_PATHS 0x1F
#define CHARGING 0x20
#define SINK_CONNECTIONS 0x50

static const char *TAG = "sw6306_output";

/* Capture faults before the next insertion event can clear their history. */
static void log_output_registers(void)
{
    const uint16_t registers[] = {0x18, 0x19, 0x1D, 0x28, 0x40, 0x41, 0x42,
                                  0x2A, 0x2B, 0x2C};
    for (unsigned i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i) {
        uint8_t value = 0;
        esp_err_t err = sw6306_register_read(registers[i], &value);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Diagnostic REG0x%02X read failed: %s",
                     (unsigned)registers[i], esp_err_to_name(err));
            break;
        }
        ESP_LOGI(TAG, "REG0x%02X=0x%02X", (unsigned)registers[i], (unsigned)value);
    }
}

/* These are software polling deadlines, not timing guarantees from the IC. */
static esp_err_t wait_status(uint16_t reg, uint8_t mask, uint8_t expected,
                             unsigned timeout_ms, esp_err_t timeout_error)
{
    const TickType_t start = xTaskGetTickCount();
    for (;;) {
        uint8_t value = 0;
        esp_err_t err = sw6306_register_read(reg, &value);
        if (err != ESP_OK) return err;
        if ((value & mask) == expected) return ESP_OK;
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms))
            return timeout_error;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static esp_err_t update_checked(uint16_t address, uint8_t mask, uint8_t value)
{
    esp_err_t err = sw6306_register_update_bits(address, mask, value);
    uint8_t actual = 0;
    if (err == ESP_OK) err = sw6306_register_read(address, &actual);
    if (err == ESP_OK && (actual & mask) != (value & mask)) err = ESP_ERR_INVALID_RESPONSE;
    return err;
}

static int32_t clamp(int32_t v, int32_t low, int32_t high)
{
    return v < low ? low : (v > high ? high : v);
}

esp_err_t sw6306_power_read_output(sw6306_output_config_t *out)
{
    if (out == NULL) return ESP_ERR_INVALID_ARG;
    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;
    uint8_t force = 0, lo = 0, hi = 0, discharge = 0, charge = 0;
    err = sw6306_register_read(REG_FORCE, &force);
    if (err == ESP_OK) err = sw6306_register_read(REG_VOLTAGE_LOW, &lo);
    if (err == ESP_OK) err = sw6306_register_read(REG_VOLTAGE_HIGH, &hi);
    if (err == ESP_OK) err = sw6306_register_read(REG_OUTPUT_IBUS, &discharge);
    if (err == ESP_OK) err = sw6306_register_read(REG_CHARGE_IBUS, &charge);
    if (err == ESP_OK) {
        /* Manual specifies clamping, including raw reset values of zero. */
        *out = (sw6306_output_config_t) {
            /* The legacy implementation treats the output-power source
             * selector (bit7) independently from the VBUS/IBUS enables. */
            .force_voltage = (force & FORCE_VOLTAGE) != 0,
            .force_ibus = (force & FORCE_IBUS) != 0,
            .voltage_mv = clamp((((hi & 0x0f) << 8) | lo) * 10, 3300, 27300),
            .output_limit_ma = clamp(discharge * 50, 200, 7000),
            .charge_limit_ma = clamp(charge * 50, 200, 7000),
        };
    }
    (void)sw6306_register_access_end();
    return err;
}

esp_err_t sw6306_power_write_output(const sw6306_output_config_t *c)
{
    if (!sw6306_output_valid(c)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;
    sw6306_output_config_t before;
    err = sw6306_power_read_output(&before);
    /* This page controls a load wired directly to the internal VBUS rail.
     * Both A ports are absent on this board. C1 must be configured only-source
     * and actually detect a sink through CC before REG22[6] is valid.
     * C1's connector also becomes live at the forced voltage. */
    uint8_t ports = 0, status = 0;
    bool output_off_requested = false;
    const char *stage = "read configuration";
    if (err == ESP_OK) {
        stage = "check input/output state";
        err = sw6306_register_read(REG_PORT_STATUS, &ports);
        if (err == ESP_OK) err = sw6306_register_read(REG_SYSTEM_STATUS, &status);
        if (err == ESP_OK && ((ports & SINK_CONNECTIONS) || (status & CHARGING))) {
            ESP_LOGE(TAG, "Disconnect charging input before forcing VBUS");
            err = ESP_ERR_INVALID_STATE;
        }
        if (err == ESP_OK && c->force_voltage && (ports & 0x2C)) {
            ESP_LOGE(TAG, "Only C1 may be connected for internal-VBUS output");
            err = ESP_ERR_INVALID_STATE;
        }
        if (err == ESP_OK) {
            stage = "disable physical output";
            output_off_requested = true;
            err = update_checked(REG_MODE, OUTPUT_OFF, OUTPUT_OFF);
        }
        if (err == ESP_OK)
            err = wait_status(REG_SYSTEM_STATUS, DISCHARGE_AND_PATHS, 0, 1000, ESP_ERR_TIMEOUT);
        if (err == ESP_OK && c->force_voltage) {
            stage = "configure C1 Source role";
            err = update_checked(REG_C1_ROLE, 0x03, 0x01);
        }
        /* Voltage writes require the force-enable bit. Keep physical outputs
         * off until BOTH bytes and the final control bits have been verified. */
        stage = err == ESP_OK ? "write forced voltage" : stage;
        if (err == ESP_OK) err = update_checked(REG_FORCE, FORCE_VOLTAGE, FORCE_VOLTAGE);
        const uint16_t raw = (uint16_t)(c->voltage_mv / 10);
        if (err == ESP_OK) err = update_checked(REG_VOLTAGE_LOW, 0xff, (uint8_t)raw);
        if (err == ESP_OK) err = update_checked(REG_VOLTAGE_HIGH, 0x0f, (uint8_t)(raw >> 8));
    }
    if (err == ESP_OK && c->force_ibus) {
        stage = "write IBUS limits";
        err = update_checked(REG_FORCE, FORCE_IBUS, FORCE_IBUS);
        if (err == ESP_OK && c->output_limit_ma != before.output_limit_ma)
            err = update_checked(REG_OUTPUT_IBUS, 0xff, (uint8_t)(c->output_limit_ma / 50));
        if (err == ESP_OK && c->charge_limit_ma != before.charge_limit_ma)
            err = update_checked(REG_CHARGE_IBUS, 0xff, (uint8_t)(c->charge_limit_ma / 50));
    }
    /* REG40[7] selects the REG4F power-limit path; it is independent from
     * the VBUS/IBUS force bits and must not be changed by this operation. */
    const uint8_t flags =
                          (c->force_voltage ? FORCE_VOLTAGE : 0) |
                          (c->force_ibus ? FORCE_IBUS : 0);
    if (err == ESP_OK) err = update_checked(REG_FORCE,
        FORCE_VOLTAGE | FORCE_IBUS, flags);
    if (err == ESP_OK && output_off_requested) {
        stage = "verify complete output configuration";
        sw6306_output_config_t actual;
        err = sw6306_power_read_output(&actual);
        if (err == ESP_OK && (actual.force_voltage != c->force_voltage ||
            actual.force_ibus != c->force_ibus || actual.voltage_mv != c->voltage_mv ||
            (c->force_ibus && (actual.output_limit_ma != c->output_limit_ma ||
                              actual.charge_limit_ma != c->charge_limit_ma))))
            err = ESP_ERR_INVALID_RESPONSE;
        if (err == ESP_OK && c->force_voltage) {
            stage = "clear voltage offset and cable compensation";
            err = update_checked(REG_VOLTAGE_COMPENSATION, 0x0F, 0x05);
            stage = err == ESP_OK ? "disable C1 no-load detection" : stage;
            if (err == ESP_OK) err = update_checked(REG_NO_LOAD, 0x08, 0x08);
            stage = err == ESP_OK ? "wait for C1 Source connection (CC/Rd)" : stage;
            if (err == ESP_OK) err = update_checked(REG_MODE, OUTPUT_OFF, 0);
            /* REG28[3] made Type-C sink-only during configuration. Clearing it
             * restores the configured Source role, but cannot simulate Rd.
             * No PD contract is required here; a real Type-C attachment is. */
            if (err == ESP_OK)
                err = wait_status(REG_PORT_STATUS, 0xF0, 0x80, 3000, ESP_ERR_NOT_FOUND);
            if (err == ESP_ERR_NOT_FOUND) {
                uint8_t typec = 0, connected = 0;
                const esp_err_t cc_err = sw6306_register_read(REG_TYPEC_STATUS, &typec);
                const esp_err_t port_err = sw6306_register_read(REG_PORT_STATUS, &connected);
                ESP_LOGE(TAG, "C1 has no Source attachment: REG19=0x%02X (%s), REG1D=0x%02X (%s). CC must detect a sink/Rd; Source mode alone cannot start VBUS",
                         (unsigned)typec, esp_err_to_name(cc_err),
                         (unsigned)connected, esp_err_to_name(port_err));
            }
            stage = err == ESP_OK ? "start internal VBUS via C1" : stage;
            /* Self-clearing event register: no read/modify/write or readback. */
            if (err == ESP_OK) err = sw6306_register_write(REG_PORT_EVENT, 0x40);
            if (err == ESP_OK)
                err = wait_status(REG_SYSTEM_STATUS, CHARGING | DISCHARGE_AND_PATHS,
                                  0x18, 1000, ESP_ERR_TIMEOUT);
        }
        /* Disabled means physical output stays off (REG28[3]=1), rather than
         * handing a live internal rail back to protocol voltage selection. */
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Apply failed at %s: %s", stage, esp_err_to_name(err));
        if (output_off_requested) {
            /* Do not energize a partially configured output. If I2C itself is
             * broken, report that physical shutdown could not be confirmed. */
            const esp_err_t stop_err = update_checked(REG_MODE, OUTPUT_OFF, OUTPUT_OFF);
            ESP_LOGE(TAG, "Output-off latch after failure: %s; reload before retry",
                     esp_err_to_name(stop_err));
        }
        log_output_registers();
    } else {
        log_output_registers();
        ESP_LOGI(TAG, "Internal VBUS %s, target=%ld mV, trigger=%s (register/path check passed)",
                 c->force_voltage ? "ON" : "OFF", (long)c->voltage_mv,
                 c->force_voltage ? "C1" : "none");
    }
    (void)sw6306_register_access_end();
    return err;
}

esp_err_t sw6306_power_read_buck(sw6306_buck_config_t *out)
{
    if (out == NULL) return ESP_ERR_INVALID_ARG;
    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;
    uint8_t r0 = 0, r1 = 0, r2 = 0;
    err = sw6306_register_read(REG_BUCK0, &r0);
    if (err == ESP_OK) err = sw6306_register_read(REG_BUCK1, &r1);
    if (err == ESP_OK) err = sw6306_register_read(REG_BUCK2, &r2);
    if (err == ESP_OK) *out = (sw6306_buck_config_t) {
        .frequency_khz = SW6306_FREQUENCIES_KHZ[r0 >> 6],
        .peak_limit_ma = SW6306_PEAK_LIMITS_MA[(r0 >> 4) & 3],
        .die_temperature_c = SW6306_DIE_TEMPERATURES_C[(r0 >> 2) & 3],
        .m2_rdson_uohm = SW6306_M2_RDSON_UOHM[r1 >> 6],
        .force_pwm = (r2 & 0x40) != 0,
    };
    (void)sw6306_register_access_end();
    return err;
}

esp_err_t sw6306_power_write_buck(const sw6306_buck_config_t *c)
{
    if (!sw6306_buck_valid(c)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;
    uint8_t r0 = (uint8_t)((sw6306_option_index(SW6306_FREQUENCIES_KHZ, c->frequency_khz) << 6) |
                         (sw6306_option_index(SW6306_PEAK_LIMITS_MA, c->peak_limit_ma) << 4) |
                         (sw6306_option_index(SW6306_DIE_TEMPERATURES_C, c->die_temperature_c) << 2));
    err = update_checked(REG_BUCK0, 0xfc, r0);
    if (err == ESP_OK) err = update_checked(REG_BUCK1, 0xc0,
        (uint8_t)(sw6306_option_index(SW6306_M2_RDSON_UOHM, c->m2_rdson_uohm) << 6));
    if (err == ESP_OK) err = update_checked(REG_BUCK2, 0x40, c->force_pwm ? 0x40 : 0);
    (void)sw6306_register_access_end();
    return err;
}
