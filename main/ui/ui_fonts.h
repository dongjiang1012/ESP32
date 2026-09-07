#pragma once
#include "lvgl.h"

/* Noto Sans SC regular weight, generated from the current UI text set. */
LV_FONT_DECLARE(ui_font_cn_16);
LV_FONT_DECLARE(ui_font_cn_24);

/* UI copies normalize line metrics for the 16 px and 24 px layouts while
 * retaining the converter-generated glyph maps and fallback fonts. */
extern lv_font_t ui_font_body;
extern lv_font_t ui_font_large;
void ui_fonts_init(void);

#define UI_FONT_BODY   (&ui_font_body)
#define UI_FONT_MEDIUM UI_FONT_LARGE
#define UI_FONT_LARGE  (&ui_font_large)
