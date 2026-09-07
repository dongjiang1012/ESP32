#include "sw6306_power_types.h"
#include <stddef.h>

const sw6306_numeric_spec_t SW6306_VOLTAGE_SPEC = {"输出电压", "V", 3300, 27300, 10, 3};
const sw6306_numeric_spec_t SW6306_OUTPUT_CURRENT_SPEC = {"输出 IBUS 限流", "A", 200, 7000, 50, 3};
const sw6306_numeric_spec_t SW6306_CHARGE_CURRENT_SPEC = {"充电 IBUS 限流", "A", 200, 7000, 50, 3};
const int32_t SW6306_FREQUENCIES_KHZ[4] = {300, 200, 400, 500};
const int32_t SW6306_PEAK_LIMITS_MA[4] = {12000, 14000, 16000, 18000};
const int32_t SW6306_DIE_TEMPERATURES_C[4] = {120, 130, 140, 150};
const int32_t SW6306_M2_RDSON_UOHM[4] = {2500, 5000, 7500, 10000};

int sw6306_option_index(const int32_t options[4], int32_t value)
{
    for (int i = 0; i < 4; ++i) if (options[i] == value) return i;
    return -1;
}

bool sw6306_numeric_valid(const sw6306_numeric_spec_t *s, int32_t v)
{
    return s != NULL && s->step > 0 && v >= s->minimum && v <= s->maximum &&
           (v - s->minimum) % s->step == 0;
}

bool sw6306_output_valid(const sw6306_output_config_t *c)
{
    return c != NULL && sw6306_numeric_valid(&SW6306_VOLTAGE_SPEC, c->voltage_mv) &&
           sw6306_numeric_valid(&SW6306_OUTPUT_CURRENT_SPEC, c->output_limit_ma) &&
           sw6306_numeric_valid(&SW6306_CHARGE_CURRENT_SPEC, c->charge_limit_ma);
}

bool sw6306_buck_valid(const sw6306_buck_config_t *c)
{
    return c != NULL && sw6306_option_index(SW6306_FREQUENCIES_KHZ, c->frequency_khz) >= 0 &&
           sw6306_option_index(SW6306_PEAK_LIMITS_MA, c->peak_limit_ma) >= 0 &&
           sw6306_option_index(SW6306_DIE_TEMPERATURES_C, c->die_temperature_c) >= 0 &&
           sw6306_option_index(SW6306_M2_RDSON_UOHM, c->m2_rdson_uohm) >= 0;
}

bool sw6306_output_equal(const sw6306_output_config_t *a, const sw6306_output_config_t *b)
{
    return a->force_voltage == b->force_voltage && a->force_ibus == b->force_ibus &&
           a->voltage_mv == b->voltage_mv && a->output_limit_ma == b->output_limit_ma &&
           a->charge_limit_ma == b->charge_limit_ma;
}

bool sw6306_buck_equal(const sw6306_buck_config_t *a, const sw6306_buck_config_t *b)
{
    return a->frequency_khz == b->frequency_khz && a->peak_limit_ma == b->peak_limit_ma &&
           a->die_temperature_c == b->die_temperature_c && a->m2_rdson_uohm == b->m2_rdson_uohm &&
           a->force_pwm == b->force_pwm;
}
