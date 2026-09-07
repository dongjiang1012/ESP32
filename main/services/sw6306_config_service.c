#include "sw6306_config_service.h"
#include <stddef.h>

#include "sw6306_startup_config.h"
#include "sw6306_typec_mode.h"
#include "sw6306_power_config.h"
#include "sw6306_register_access.h"

esp_err_t sw6306_config_service_load_output(sw6306_config_model_t *m)
{
    if (m == NULL) return ESP_ERR_INVALID_ARG;
    m->output_valid = false;
    sw6306_output_config_t actual;
    esp_err_t err = sw6306_power_read_output(&actual);
    if (err == ESP_OK) {
        m->output = m->output_actual = actual;
        m->output_valid = true;
        m->output_dirty = false;
    }
    return err;
}

esp_err_t sw6306_config_service_apply_output(sw6306_config_model_t *m)
{
    if (m == NULL || !m->output_valid || !sw6306_output_valid(&m->output)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;
    m->output_valid = false;
    err = sw6306_power_write_output(&m->output);
    /* Enabling output also configures C1. Refresh its actual value even if
     * a later output stage failed, without discarding an unsaved role draft. */
    sw6306_c1_role_t role = SW6306_C1_ROLE_RESERVED;
    m->c1_role_valid = sw6306_typec_mode_read_c1_role(&role) == ESP_OK &&
                       (unsigned)role < SW6306_C1_ROLE_RESERVED;
    if (m->c1_role_valid) {
        m->c1_role_actual = role;
        if (!m->c1_role_dirty) m->c1_role = role;
        m->c1_role_dirty = m->c1_role != role;
    }
    sw6306_output_config_t actual;
    if (err == ESP_OK) err = sw6306_power_read_output(&actual);
    if (err == ESP_OK && (actual.force_voltage != m->output.force_voltage ||
        actual.force_ibus != m->output.force_ibus || actual.voltage_mv != m->output.voltage_mv ||
        (m->output.force_ibus && (actual.output_limit_ma != m->output.output_limit_ma ||
                                 actual.charge_limit_ma != m->output.charge_limit_ma)))) {
        err = ESP_ERR_INVALID_RESPONSE;
    }
    if (err == ESP_OK) {
        m->output = m->output_actual = actual;
        m->output_valid = true;
        m->output_dirty = false;
    }
    (void)sw6306_register_access_end();
    return err;
}

esp_err_t sw6306_config_service_load_buck(sw6306_config_model_t *m)
{
    if (m == NULL) return ESP_ERR_INVALID_ARG;
    m->buck_valid = false;
    sw6306_buck_config_t actual;
    esp_err_t err = sw6306_power_read_buck(&actual);
    if (err == ESP_OK) {
        m->buck = m->buck_actual = actual;
        m->buck_valid = true;
        m->buck_dirty = false;
    }
    return err;
}

esp_err_t sw6306_config_service_apply_buck(sw6306_config_model_t *m)
{
    if (m == NULL || !m->buck_valid || !sw6306_buck_valid(&m->buck)) return ESP_ERR_INVALID_ARG;
    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;
    m->buck_valid = false;
    err = sw6306_power_write_buck(&m->buck);
    sw6306_buck_config_t actual;
    if (err == ESP_OK) err = sw6306_power_read_buck(&actual);
    if (err == ESP_OK && !sw6306_buck_equal(&actual, &m->buck)) err = ESP_ERR_INVALID_RESPONSE;
    if (err == ESP_OK) {
        m->buck = m->buck_actual = actual;
        m->buck_valid = true;
        m->buck_dirty = false;
    }
    (void)sw6306_register_access_end();
    return err;
}

esp_err_t sw6306_config_service_load(sw6306_config_model_t *model)
{
    if (model == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    model->c1_role_valid = false;
    sw6306_c1_role_t role = SW6306_C1_ROLE_RESERVED;
    const esp_err_t ret = sw6306_typec_mode_read_c1_role(&role);
    if (ret != ESP_OK) {
        return ret;
    }

    if ((unsigned)role >= SW6306_C1_ROLE_RESERVED) return ESP_ERR_INVALID_RESPONSE;
    sw6306_config_model_set_c1_role(model, role, false);
    return ESP_OK;
}

esp_err_t sw6306_config_service_apply(sw6306_config_model_t *model)
{
    if (!sw6306_config_model_is_ready(model)) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = sw6306_register_access_begin();
    if (ret != ESP_OK) return ret;
    model->c1_role_valid = false;
    ret = sw6306_typec_mode_set_c1_role(model->c1_role);
    sw6306_c1_role_t readback = SW6306_C1_ROLE_RESERVED;
    if (ret == ESP_OK) ret = sw6306_typec_mode_read_c1_role(&readback);
    if (ret == ESP_OK && readback != model->c1_role) ret = ESP_ERR_INVALID_RESPONSE;
    if (ret == ESP_OK) sw6306_config_model_set_c1_role(model, readback, false);
    (void)sw6306_register_access_end();
    return ret;
}

esp_err_t sw6306_config_service_toggle_c1_role(
    sw6306_config_model_t *model)
{
    if (model == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = sw6306_config_service_load(model);
    if (ret != ESP_OK) {
        return ret;
    }

    model->c1_role = model->c1_role == SW6306_C1_ROLE_SINK
                         ? SW6306_C1_ROLE_SOURCE
                         : SW6306_C1_ROLE_SINK;
    model->c1_role_dirty = true;
    return sw6306_config_service_apply(model);
}

esp_err_t sw6306_config_service_apply_startup(void)
{
    return sw6306_startup_config_apply();
}
