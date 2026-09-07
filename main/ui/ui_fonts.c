#include "ui_fonts.h"

lv_font_t ui_font_body;
lv_font_t ui_font_large;

void ui_fonts_init(void)
{
    /* Copy only the descriptors; the glyph arrays are shared. The generated
     * glyph bounds are verified against these UI line boxes by the font check. */
    ui_font_body = ui_font_cn_16;
    ui_font_large = ui_font_cn_24;
    ui_font_body.line_height = 20;
    ui_font_body.base_line = 4;
    ui_font_large.line_height = 28;
    ui_font_large.base_line = 5;
}
