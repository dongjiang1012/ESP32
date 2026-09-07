#pragma once
#include "esp_err.h"

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a phone-like pull-down brightness panel to a screen.
 * The initial brightness is set to 50 percent.
 */
void ui_brightness_panel_create(lv_obj_t *screen);
int ui_brightness_panel_get_percent(void);
esp_err_t ui_brightness_panel_set_percent(int percent);

#ifdef __cplusplus
}
#endif
