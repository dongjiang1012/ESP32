#include "sw6306_config_model.h"

#include <stddef.h>

void sw6306_config_model_init(sw6306_config_model_t *model)
{
    if (model == NULL) {
        return;
    }

    *model = (sw6306_config_model_t) {
        .c1_role = SW6306_C1_ROLE_SOURCE,
        .c1_role_valid = false,
        .c1_role_dirty = false,
    };
}

void sw6306_config_model_set_c1_role(sw6306_config_model_t *model,
                                     sw6306_c1_role_t role,
                                     bool mark_dirty)
{
    if (model == NULL || (unsigned)role >= SW6306_C1_ROLE_RESERVED) {
        return;
    }

    model->c1_role = role;
    model->c1_role_valid = true;
    model->c1_role_dirty = mark_dirty;
    if (!mark_dirty) model->c1_role_actual = role;
}

bool sw6306_config_model_is_ready(const sw6306_config_model_t *model)
{
    return model != NULL && model->c1_role_valid &&
           (unsigned)model->c1_role < SW6306_C1_ROLE_RESERVED;
}
