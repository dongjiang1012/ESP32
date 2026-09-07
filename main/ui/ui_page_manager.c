#include "ui_page_manager.h"
#include "ui_common.h"
#include <stdint.h>

static lv_obj_t *s_pages[UI_PAGE_COUNT];
static lv_obj_t *s_buttons[UI_PAGE_COUNT];
static ui_page_id_t s_current;
static void (*s_on_show)(ui_page_id_t);

static void navigate_cb(lv_event_t *event)
{
    ui_page_manager_show((ui_page_id_t)(uintptr_t)lv_event_get_user_data(event));
}

void ui_page_manager_create(lv_obj_t *screen, void (*on_show)(ui_page_id_t))
{
    s_on_show = on_show;
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(screen, UI_FONT_BODY, 0);
    lv_obj_set_style_text_font(lv_layer_top(), UI_FONT_BODY, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *brand = ui_label(screen, "电源控制中心", 24, 27, 228, UI_ACCENT);
    lv_obj_set_style_text_font(brand, UI_FONT_MEDIUM, 0);
    static const char *const names[] = {"实时监控", "输出控制", "升降压配置", "系统设置", "异常分析"};
    for (unsigned i = 0; i < UI_PAGE_COUNT; ++i) {
        s_buttons[i] = ui_button(screen, names[i], 238 + (int)i * 153, 16, 145,
                                 navigate_cb, (void *)(uintptr_t)i);
        s_pages[i] = lv_obj_create(screen);
        lv_obj_remove_style_all(s_pages[i]);
        lv_obj_set_pos(s_pages[i], 0, 76);
        lv_obj_set_size(s_pages[i], lv_pct(100), 524);
        lv_obj_remove_flag(s_pages[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_page_manager_root(ui_page_id_t page)
{
    return (unsigned)page < UI_PAGE_COUNT ? s_pages[page] : NULL;
}

void ui_page_manager_show(ui_page_id_t page)
{
    if ((unsigned)page >= UI_PAGE_COUNT) return;
    for (unsigned i = 0; i < UI_PAGE_COUNT; ++i) {
        if (i == (unsigned)page) lv_obj_remove_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        ui_set_checked(s_buttons[i], i == (unsigned)page);
    }
    s_current = page;
    if (s_on_show != NULL) s_on_show(page);
}

ui_page_id_t ui_page_manager_current(void) { return s_current; }
