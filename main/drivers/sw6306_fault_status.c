#include "sw6306_fault_status.h"

#include "sw6306_register_access.h"

#define REG_EVENT_FLAGS       0x015
#define REG_DISCHARGE_FAULTS  0x02A
#define REG_CHARGE_FAULTS     0x02B
#define REG_OTHER_FAULTS      0x02C

esp_err_t sw6306_fault_status_read(sw6306_fault_status_t *out)
{
    if (out == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t err = sw6306_register_access_begin();
    if (err != ESP_OK) return err;

    sw6306_fault_status_t value = {0};
    err = sw6306_register_read(REG_EVENT_FLAGS, &value.event_flags);
    if (err == ESP_OK) err = sw6306_register_read(REG_DISCHARGE_FAULTS, &value.discharge_faults);
    if (err == ESP_OK) err = sw6306_register_read(REG_CHARGE_FAULTS, &value.charge_faults);
    if (err == ESP_OK) err = sw6306_register_read(REG_OTHER_FAULTS, &value.other_faults);
    (void)sw6306_register_access_end();

    if (err == ESP_OK) *out = value;
    return err;
}
