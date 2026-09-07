#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Shared domain values. No register addresses or LVGL dependencies. */
typedef struct {
    bool force_voltage;
    bool force_ibus; /* Shared by charging and discharging on this chip. */
    int32_t voltage_mv;
    int32_t output_limit_ma;
    int32_t charge_limit_ma;
} sw6306_output_config_t;

typedef struct {
    int32_t frequency_khz;
    int32_t peak_limit_ma; /* One shared charge/discharge setting. */
    int32_t die_temperature_c;
    int32_t m2_rdson_uohm;
    bool force_pwm;
} sw6306_buck_config_t;

typedef struct {
    const char *title;
    const char *unit;
    int32_t minimum;
    int32_t maximum;
    int32_t step;
    uint8_t decimals; /* Stored integer / 10^decimals = displayed value. */
} sw6306_numeric_spec_t;

extern const sw6306_numeric_spec_t SW6306_VOLTAGE_SPEC;
extern const sw6306_numeric_spec_t SW6306_OUTPUT_CURRENT_SPEC;
extern const sw6306_numeric_spec_t SW6306_CHARGE_CURRENT_SPEC;

bool sw6306_numeric_valid(const sw6306_numeric_spec_t *spec, int32_t value);
bool sw6306_output_valid(const sw6306_output_config_t *config);
bool sw6306_buck_valid(const sw6306_buck_config_t *config);
bool sw6306_output_equal(const sw6306_output_config_t *a, const sw6306_output_config_t *b);
bool sw6306_buck_equal(const sw6306_buck_config_t *a, const sw6306_buck_config_t *b);

/* Register enumeration order, which is NOT sorted for frequency. */
extern const int32_t SW6306_FREQUENCIES_KHZ[4];
extern const int32_t SW6306_PEAK_LIMITS_MA[4];
extern const int32_t SW6306_DIE_TEMPERATURES_C[4];
extern const int32_t SW6306_M2_RDSON_UOHM[4];
int sw6306_option_index(const int32_t options[4], int32_t value);
