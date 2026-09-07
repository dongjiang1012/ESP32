#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read-only fault/event snapshot from the SW6306V low register page.
 * REG0x15 is write-one-to-clear; this reader never acknowledges or clears it. */
typedef struct {
    uint8_t event_flags;       /* REG0x15: event indications */
    uint8_t discharge_faults;  /* REG0x2A: discharge fault history */
    uint8_t charge_faults;     /* REG0x2B: charge fault history */
    uint8_t other_faults;      /* REG0x2C: 62368/DPDM/CC and key event */
} sw6306_fault_status_t;

esp_err_t sw6306_fault_status_read(sw6306_fault_status_t *out);

#ifdef __cplusplus
}
#endif
